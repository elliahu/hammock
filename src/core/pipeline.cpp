#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "pipeline.hpp"

namespace hammock::core {

    // Internal graphics pipeline config struct
    struct GraphicsPipelineInternalConfig {
        GraphicsPipelineInternalConfig() = default;
        GraphicsPipelineInternalConfig(const GraphicsPipelineInternalConfig &) = delete;
        GraphicsPipelineInternalConfig &operator=(const GraphicsPipelineInternalConfig &) = delete;

        vk::PipelineViewportStateCreateInfo viewportInfo;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
        vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<vk::DynamicState> dynamicStateEnables;
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    };

    /// Helper function for constructing color blend attachment state
    inline vk::PipelineColorBlendAttachmentState graphicsPipelineColorBlendAttachmentState(
        vk::ColorComponentFlags colorWriteMask,
        vk::Bool32 blendEnable) {
        vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
        pipelineColorBlendAttachmentState.blendEnable = blendEnable;
        pipelineColorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        pipelineColorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        pipelineColorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
        pipelineColorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        pipelineColorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        pipelineColorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
        pipelineColorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        return pipelineColorBlendAttachmentState;
    }

    // ==================== Pipeline Implementation ====================

    Pipeline::Pipeline(const PipelineCreateInfo &config)
        : type_(config.type), device_(config.device) {
        switch (config.type) {
            case PipelineType::Compute:
                createComputePipeline(config);
                break;
            case PipelineType::Graphics:
                createGraphicsPipeline(config);
                break;
        }
    }

    Pipeline::~Pipeline() {
        device_.device().destroyPipeline(pipeline_, nullptr);
        device_.device().destroyPipelineLayout(pipelineLayout_, nullptr);

        // Clean up type-specific shader modules
        if (type_ == PipelineType::Compute) {
            device_.device().destroyShaderModule(computeShaderModule_, nullptr);
        } else {
            if (vertShaderModule_) device_.device().destroyShaderModule(vertShaderModule_, nullptr);
            if (fragShaderModule_) device_.device().destroyShaderModule(fragShaderModule_, nullptr);
        }
    }

    std::unique_ptr<Pipeline> Pipeline::create(const PipelineCreateInfo &createInfo) {
        return std::make_unique<Pipeline>(createInfo);
    }

    void Pipeline::createShaderModule(const std::vector<char> &code, vk::ShaderModule *shaderModule) const {
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

        if (device_.device().createShaderModule(&createInfo, nullptr, shaderModule) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create shader module");
        }
    }

    void Pipeline::createComputePipeline(const PipelineCreateInfo &config) {
        // Create a pipeline layout using the provided descriptor set layouts and push constant ranges.
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = config.pushConstantRanges.empty()
                                                     ? nullptr
                                                     : config.pushConstantRanges.data();

        if (device_.device().createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create compute pipeline layout");
        }

        // Create the compute shader module.
        createShaderModule(config.computeShader.byteCode, &computeShaderModule_);

        // Set up the compute shader stage.
        vk::PipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
        shaderStageInfo.module = computeShaderModule_;
        shaderStageInfo.pName = config.computeShader.entry.c_str();

        // Create the compute pipeline.
        vk::ComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.basePipelineIndex = -1;

        if (device_.device().createComputePipelines({}, 1, &pipelineInfo, nullptr, &pipeline_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create compute pipeline");
        }
    }

    void Pipeline::createGraphicsPipeline(const PipelineCreateInfo &config) {
        // Create pipeline layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = config.descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = config.pushConstantRanges.data();

        if (device_.device().createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout_) !=
            vk::Result::eSuccess) {
            throw std::runtime_error("failed to create pipeline layout");
        }

        // Setup default graphics configuration
        GraphicsPipelineInternalConfig configInfo{};
        Pipeline::defaultGraphicsRenderPipelineConfig(configInfo);

        // Override with custom blend attachment states if provided
        if (!config.blendAttachmentStates.empty()) {
            configInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(config.blendAttachmentStates.size());
            configInfo.colorBlendInfo.pAttachments = config.blendAttachmentStates.data();
        }

        // Setup depth and rasterization state
        configInfo.depthStencilInfo.depthTestEnable = config.depthTest;
        configInfo.depthStencilInfo.depthWriteEnable = config.depthTest;
        configInfo.rasterizationInfo.cullMode = config.cullMode;
        configInfo.rasterizationInfo.frontFace = config.frontFace;

        // Setup dynamic state
        configInfo.dynamicStateEnables = config.dynamicStateEnables;
        configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
        configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        configInfo.dynamicStateInfo.flags = {};

        // Setup shader stages
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        shaderStages.resize(2);

        createShaderModule(config.vertexShader.byteCode, &vertShaderModule_);
        createShaderModule(config.fragmentShader.byteCode, &fragShaderModule_);

        shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
        shaderStages[0].module = vertShaderModule_;
        shaderStages[0].pName = config.vertexShader.entry.c_str();
        shaderStages[0].flags = {};
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
        shaderStages[1].module = fragShaderModule_;
        shaderStages[1].pName = config.fragmentShader.entry.c_str();
        shaderStages[1].flags = {};
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        // Setup vertex input state
        auto &bindingDescriptions = config.vertexInputBindingDescriptions;
        auto &attributeDescriptions = config.vertexInputAttributeDescriptions;
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

        // Create graphics pipeline
        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
        pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
        pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = config.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = nullptr;

        // Setup dynamic rendering if no render pass is provided
        vk::PipelineRenderingCreateInfoKHR pipelineDynamicRenderingCreateInfo{};
        if (config.renderPass == nullptr) {
            pipelineDynamicRenderingCreateInfo.colorAttachmentCount = static_cast<std::uint32_t>(config.colorAttachmentFormats.size());
            pipelineDynamicRenderingCreateInfo.pColorAttachmentFormats = config.colorAttachmentFormats.data();
            pipelineDynamicRenderingCreateInfo.depthAttachmentFormat = config.depthAttachmentFormat;
            pipelineDynamicRenderingCreateInfo.stencilAttachmentFormat = config.stencilAttachmentFormat;
            pipelineInfo.pNext = &pipelineDynamicRenderingCreateInfo;
        }

        if (device_.device().createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &pipeline_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create graphics pipeline");
        }
    }




    void Pipeline::defaultGraphicsRenderPipelineConfig(GraphicsPipelineInternalConfig &configInfo) {
        configInfo.inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
        configInfo.inputAssemblyInfo.primitiveRestartEnable = vk::False;

        configInfo.viewportInfo.viewportCount = 1;
        configInfo.viewportInfo.pViewports = nullptr;
        configInfo.viewportInfo.scissorCount = 1;
        configInfo.viewportInfo.pScissors = nullptr;

        configInfo.rasterizationInfo.depthClampEnable = vk::False;
        configInfo.rasterizationInfo.rasterizerDiscardEnable = vk::False;
        configInfo.rasterizationInfo.polygonMode = vk::PolygonMode::eFill;
        configInfo.rasterizationInfo.lineWidth = 1.0f;
        configInfo.rasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
        configInfo.rasterizationInfo.frontFace = vk::FrontFace::eClockwise;
        configInfo.rasterizationInfo.depthBiasEnable = true;

        configInfo.multisampleInfo.sampleShadingEnable = false;
        configInfo.multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
        configInfo.multisampleInfo.minSampleShading = 1.0f;
        configInfo.multisampleInfo.pSampleMask = nullptr;
        configInfo.multisampleInfo.alphaToCoverageEnable = false;
        configInfo.multisampleInfo.alphaToOneEnable = false;

        configInfo.colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        configInfo.colorBlendAttachment.blendEnable = false;
        configInfo.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        configInfo.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        configInfo.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        configInfo.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        configInfo.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        configInfo.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        configInfo.colorBlendInfo.logicOpEnable = false;
        configInfo.colorBlendInfo.logicOp = vk::LogicOp::eCopy;
        configInfo.colorBlendInfo.attachmentCount = 1;
        configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
        configInfo.colorBlendInfo.blendConstants[0] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[1] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[2] = 0.0f;
        configInfo.colorBlendInfo.blendConstants[3] = 0.0f;

        configInfo.depthStencilInfo.depthTestEnable = true;
        configInfo.depthStencilInfo.depthWriteEnable = true;
        configInfo.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
        configInfo.depthStencilInfo.depthBoundsTestEnable = false;
        configInfo.depthStencilInfo.minDepthBounds = 0.0f;
        configInfo.depthStencilInfo.maxDepthBounds = 1.0f;
        configInfo.depthStencilInfo.stencilTestEnable = false;
        configInfo.depthStencilInfo.front = vk::StencilOpState{};
        configInfo.depthStencilInfo.back = vk::StencilOpState{};

        configInfo.dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
        configInfo.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        configInfo.dynamicStateInfo.flags = {};
    }

    // ==================== PipelineBuilder Implementation ====================

    PipelineBuilder::PipelineBuilder(Device &device) : createInfo_{PipelineType::Graphics, device} {
    }

    PipelineBuilder &PipelineBuilder::asComputePipeline() {
        createInfo_.type = PipelineType::Compute;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::asGraphicsPipeline() {
        createInfo_.type = PipelineType::Graphics;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addDescriptorSetLayout(
        ResourceRef<DescriptorSetLayout> descriptorSetLayout) {
        createInfo_.descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addPushConstantRange(vk::PushConstantRange pushConstantRange) {
        createInfo_.pushConstantRanges.push_back(pushConstantRange);
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setComputeShader(const std::vector<char> &bytecode,
                                                       const std::string &entry) {
        createInfo_.computeShader.byteCode = bytecode;
        createInfo_.computeShader.entry = entry;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setVertexShader(const std::vector<char> &byteCode,
                                                      const std::string &entry) {
        createInfo_.vertexShader.byteCode = byteCode;
        createInfo_.vertexShader.entry = entry;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setFragmentShader(const std::vector<char> &byteCode,
                                                        const std::string &entry) {
        createInfo_.fragmentShader.byteCode = byteCode;
        createInfo_.fragmentShader.entry = entry;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setDepthTest(vk::Bool32 depthTest, vk::CompareOp compareOp) {
        createInfo_.depthTest = depthTest;
        createInfo_.depthTestCompareOp = compareOp;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setCullMode(vk::CullModeFlags cullMode, vk::FrontFace frontFace) {
        createInfo_.cullMode = cullMode;
        createInfo_.frontFace = frontFace;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addBlendAttachmentState(vk::ColorComponentFlags colorWriteMask,
                                                              vk::Bool32 blendEnable) {
        createInfo_.blendAttachmentStates.push_back(
            graphicsPipelineColorBlendAttachmentState(colorWriteMask, blendEnable));
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setVertexInputAttributeDescriptions(
        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions) {
        createInfo_.vertexInputAttributeDescriptions = std::move(vertexInputAttributeDescriptions);
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addVertexInputAttributeDescription(std::uint32_t location,
                                                                         std::uint32_t binding,
                                                                         vk::Format format,
                                                                         std::uint32_t offset) {
        createInfo_.vertexInputAttributeDescriptions.push_back({location, binding, format, offset});
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setVertexInputBindingDescriptions(
        std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions) {
        createInfo_.vertexInputBindingDescriptions = std::move(vertexInputBindingDescriptions);
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addVertexInputBindingDescription(std::uint32_t binding,
                                                                       std::uint32_t stride,
                                                                       vk::VertexInputRate inputRate) {
        createInfo_.vertexInputBindingDescriptions.push_back({binding, stride, inputRate});
        return *this;
    }

    PipelineBuilder &PipelineBuilder::addColorAttachmentFormat(vk::Format format) {
        createInfo_.colorAttachmentFormats.push_back(format);
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setDepthStencilAttachmentFormat(vk::Format depth,
                                                                      vk::Format stencil) {
        createInfo_.depthAttachmentFormat = depth;
        createInfo_.stencilAttachmentFormat = stencil;
        return *this;
    }

    PipelineBuilder &PipelineBuilder::setRenderPass(vk::RenderPass renderPass) {
        createInfo_.renderPass = renderPass;
        return *this;
    }

    std::unique_ptr<Pipeline> PipelineBuilder::build() {
        return std::make_unique<Pipeline>(createInfo_);
    }

    PipelineCreateInfo PipelineBuilder::buildCreateInfo() { return createInfo_; }
}  // namespace hammock::core
