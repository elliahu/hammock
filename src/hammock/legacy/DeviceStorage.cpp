#include "hammock/legacy/DeviceStorage.h"
#include <stdint.h>

namespace Hammock {
    const uint32_t DeviceStorage::INVALID_HANDLE = UINT32_MAX;
}

Hammock::DeviceStorage::DeviceStorage(Device &device) : device{device}, buffers(), descriptorSets(),
                                                     descriptorSetLayouts(),
                                                     texture2Ds(),
                                                     texture3Ds() {
            DescriptorPool::initialize(device,
        1000,
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10000},
        });
    auto& descriptorPool = DescriptorPool::getInstance();
}

Hammock::DeviceStorageResourceHandle<Hammock::LegacyBuffer> Hammock::DeviceStorage::createBuffer(BufferCreateInfo createInfo) {
    auto buffer = std::make_unique<LegacyBuffer>(
        device,
        createInfo.instanceSize,
        createInfo.instanceCount,
        createInfo.usageFlags,
        createInfo.memoryPropertyFlags);

    if (createInfo.map) buffer->map();

    DeviceStorageResourceHandle<LegacyBuffer> handle(static_cast<id_t>(buffers.size()));
    buffers.emplace(handle.id(), std::move(buffer));
    return handle;
}


Hammock::DeviceStorageResourceHandle<Hammock::LegacyBuffer> Hammock::DeviceStorage::createVertexBuffer(const VertexBufferCreateInfo &createInfo) {
    LegacyBuffer stagingBuffer{
        device,
        createInfo.vertexSize,
        createInfo.vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    const DeviceStorageResourceHandle<LegacyBuffer> handle = createBuffer({
        .instanceSize = createInfo.vertexSize,
        .instanceCount = createInfo.vertexCount,
        .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
        .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .map = false
    });

    device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(),
                      createInfo.vertexCount * createInfo.vertexSize);

    return handle;
}

Hammock::DeviceStorageResourceHandle<Hammock::LegacyBuffer> Hammock::DeviceStorage::createIndexBuffer(const IndexBufferCreateInfo &createInfo) {
    LegacyBuffer stagingBuffer{
        device,
        createInfo.indexSize,
        createInfo.indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    stagingBuffer.map();
    stagingBuffer.writeToBuffer(createInfo.data);

    const DeviceStorageResourceHandle<LegacyBuffer> handle = createBuffer({
        .instanceSize = createInfo.indexSize,
        .instanceCount = createInfo.indexCount,
        .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usageFlags,
        .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .map = false
    });

    device.copyBuffer(stagingBuffer.getBuffer(), getBuffer(handle)->getBuffer(),
                      createInfo.indexCount * createInfo.indexSize);

    return handle;
}

Hammock::DeviceStorageResourceHandle<Hammock::DescriptorSetLayout> Hammock::DeviceStorage::createDescriptorSetLayout(
    const DescriptorSetLayoutCreateInfo &createInfo) {
    auto descriptorSetLayoutBuilder = DescriptorSetLayout::Builder(device);

    for (auto &binding: createInfo.bindings) {
        descriptorSetLayoutBuilder.addBinding(binding.binding, binding.descriptorType, binding.stageFlags,
                                              binding.count, binding.bindingFlags);
    }
    auto descriptorSetLayout = descriptorSetLayoutBuilder.build();

    DeviceStorageResourceHandle<DescriptorSetLayout> handle(static_cast<id_t>(descriptorSetLayouts.size()));
    descriptorSetLayouts.emplace(handle.id(), std::move(descriptorSetLayout));
    return handle;
}

Hammock::DeviceStorageResourceHandle<VkDescriptorSet_T *> Hammock::DeviceStorage::createDescriptorSet(
    const DescriptorSetCreateInfo &createInfo) {
    VkDescriptorSet descriptorSet;

    auto writer = DescriptorWriter(getDescriptorSetLayout(createInfo.descriptorSetLayout), *descriptorPool);

    for (auto &buffer: createInfo.bufferWrites) {
        writer.writeBuffer(buffer.binding, &buffer.bufferInfo);
    }

    for (auto &bufferArray: createInfo.bufferArrayWrites) {
        writer.writeBufferArray(bufferArray.binding, bufferArray.bufferInfos);
    }

    for (auto &image: createInfo.imageWrites) {
        writer.writeImage(image.binding, &image.imageInfo);
    }

    for (auto &imageArray: createInfo.imageArrayWrites) {
        writer.writeImageArray(imageArray.binding, imageArray.imageInfos);
    }

    for (auto &accelerationStructure: createInfo.accelerationStructureWrites) {
        writer.writeAccelerationStructure(accelerationStructure.binding,
                                          &accelerationStructure.accelerationStructureInfo);
    }

    if (writer.build(descriptorSet)) {
        DeviceStorageResourceHandle<VkDescriptorSet> handle(static_cast<id_t>(descriptorSets.size()));
        descriptorSets.emplace(handle.id(), descriptorSet);
        return handle;
    }

    throw std::runtime_error("Faild to create descriptor!");
}

Hammock::DeviceStorageResourceHandle<Hammock::Texture2D> Hammock::DeviceStorage::createTexture2D(
    const Texture2DCreateFromBufferInfo &createInfo) {
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(device);

    texture->loadFromBuffer(
        createInfo.buffer,
        createInfo.instanceSize,
        createInfo.width, createInfo.height, createInfo.channels,
        device,
        createInfo.format,
        createInfo.imageLayout,
        createInfo.samplerInfo.maxLod);

    if (createInfo.samplerInfo.createSampler) {
        texture->createSampler(device,
                               createInfo.samplerInfo.filter,
                               createInfo.samplerInfo.addressMode,
                               createInfo.samplerInfo.borderColor,
                               createInfo.samplerInfo.mipmapMode,
                               createInfo.samplerInfo.maxLod);
    }

    if (createInfo.samplerInfo.maxLod > 1) {
        texture->generateMipMaps(device, createInfo.samplerInfo.maxLod);
    }

    texture->updateDescriptor();
    DeviceStorageResourceHandle<Texture2D> handle(static_cast<id_t>(texture2Ds.size()));
    texture2Ds.emplace(handle.id(), std::move(texture));
    return handle;
}

Hammock::DeviceStorageResourceHandle<Hammock::Texture2D> Hammock::DeviceStorage::createEmptyTexture2D() {
    std::unique_ptr<Texture2D> texture = std::make_unique<Texture2D>(device);
    DeviceStorageResourceHandle<Texture2D> handle(static_cast<id_t>(texture2Ds.size()));
    texture2Ds.emplace(handle.id(), std::move(texture));
    return handle;
}


Hammock::DeviceStorageResourceHandle<Hammock::Texture3D> Hammock::DeviceStorage::createTexture3D(
    const Texture3DCreateFromBufferInfo &createInfo) {
    std::unique_ptr<Texture3D> texture = std::make_unique<Texture3D>(device);
    texture->loadFromBuffer(
        device,
        createInfo.buffer,
        createInfo.instanceSize,
        createInfo.width, createInfo.height, createInfo.channels, createInfo.depth,
        createInfo.format,
        createInfo.imageLayout
    );
    if (createInfo.samplerInfo.createSampler) {
        texture->createSampler(device, createInfo.samplerInfo.filter, createInfo.samplerInfo.addressMode);
    }
    texture->updateDescriptor();
    DeviceStorageResourceHandle<Texture3D> handle(static_cast<id_t>(texture3Ds.size()));
    texture3Ds.emplace(handle.id(), std::move(texture));
    return handle;
}

Hammock::DescriptorSetLayout &Hammock::DeviceStorage::getDescriptorSetLayout(DeviceStorageResourceHandle<DescriptorSetLayout> handle) {
    if (descriptorSetLayouts.contains(handle.id())) {
        return *descriptorSetLayouts[handle.id()];
    }

    throw std::runtime_error("Descriptor set layout with provided handle does not exist!");
}

VkDescriptorSet Hammock::DeviceStorage::getDescriptorSet(DeviceStorageResourceHandle<VkDescriptorSet> handle) {
    if (descriptorSets.contains(handle.id())) {
        return descriptorSets[handle.id()];
    }

    throw std::runtime_error("Descriptor with provided handle does not exist!");
}

std::unique_ptr<Hammock::LegacyBuffer> &Hammock::DeviceStorage::getBuffer(DeviceStorageResourceHandle<LegacyBuffer> handle) {
    if (buffers.contains(handle.id())) {
        return buffers[handle.id()];
    }

    throw std::runtime_error("Uniform buffer with provided handle does not exist!");
}

std::unique_ptr<Hammock::Texture2D> &Hammock::DeviceStorage::getTexture2D(DeviceStorageResourceHandle<Texture2D> handle) {
    if (texture2Ds.contains(handle.id())) {
        return texture2Ds[handle.id()];
    }

    throw std::runtime_error("Texture2D with provided handle does not exist!");
}

VkDescriptorImageInfo Hammock::DeviceStorage::getTexture2DDescriptorImageInfo(DeviceStorageResourceHandle<Texture2D> handle) {
    return getTexture2D(handle)->descriptor;
}


std::unique_ptr<Hammock::Texture3D> &Hammock::DeviceStorage::getTexture3D(DeviceStorageResourceHandle<Texture3D> handle) {
    if (texture3Ds.contains(handle.id())) {
        return texture3Ds[handle.id()];
    }

    throw std::runtime_error("Texture3D with provided handle does not exist!");
}

VkDescriptorImageInfo Hammock::DeviceStorage::getTexture3DDescriptorImageInfo(DeviceStorageResourceHandle<Texture3D> handle) {
    return getTexture3D(handle)->descriptor;
}

void Hammock::DeviceStorage::bindDescriptorSet(
    const VkCommandBuffer commandBuffer,
    const VkPipelineBindPoint bindPoint,
    const VkPipelineLayout pipelineLayout,
    const uint32_t firstSet,
    const uint32_t descriptorCount,
    DeviceStorageResourceHandle<VkDescriptorSet> descriptorSet,
    const uint32_t dynamicOffsetCount,
    const uint32_t *pDynamicOffsets) {
    const auto descriptor = getDescriptorSet(descriptorSet);

    vkCmdBindDescriptorSets(
        commandBuffer,
        bindPoint,
        pipelineLayout,
        firstSet, descriptorCount,
        &descriptor,
        dynamicOffsetCount,
        pDynamicOffsets);
}

void Hammock::DeviceStorage::destroyBuffer(DeviceStorageResourceHandle<LegacyBuffer> handle) {
    buffers.erase(handle.id());
}

void Hammock::DeviceStorage::destroyDescriptorSetLayout(DeviceStorageResourceHandle<DescriptorSetLayout> handle) {
    descriptorSetLayouts.erase(handle.id());
}

void Hammock::DeviceStorage::destroyTexture2D(DeviceStorageResourceHandle<Texture2D> handle) {
    texture2Ds.erase(handle.id());
}

void Hammock::DeviceStorage::destroyTexture3D(DeviceStorageResourceHandle<Texture3D> handle) {
    texture3Ds.erase(handle.id());
}

void Hammock::DeviceStorage::bindVertexBuffer(DeviceStorageResourceHandle<LegacyBuffer> handle, const VkCommandBuffer commandBuffer) {
    VkDeviceSize offsets[] = {0};
    VkBuffer buffers[] = {getBuffer(handle)->getBuffer()};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

void Hammock::DeviceStorage::bindVertexBuffer(DeviceStorageResourceHandle<LegacyBuffer> vertexBuffer, DeviceStorageResourceHandle<LegacyBuffer> indexBuffer,
                                           const VkCommandBuffer commandBuffer, VkIndexType indexType) {
    bindVertexBuffer(vertexBuffer, commandBuffer);
    bindIndexBuffer(indexBuffer, commandBuffer);
}

void Hammock::DeviceStorage::bindIndexBuffer(DeviceStorageResourceHandle<LegacyBuffer> handle, const VkCommandBuffer commandBuffer,
                                          const VkIndexType indexType) {
    vkCmdBindIndexBuffer(commandBuffer, getBuffer(handle)->getBuffer(), 0, indexType);
}

void Hammock::DeviceStorage::copyBuffer(DeviceStorageResourceHandle<LegacyBuffer> from, DeviceStorageResourceHandle<LegacyBuffer> to) {
    auto &fromBuffer = getBuffer(from);
    auto &toBuffer = getBuffer(to);

    device.copyBuffer(fromBuffer->getBuffer(), toBuffer->getBuffer(), fromBuffer->getBufferSize());
}
