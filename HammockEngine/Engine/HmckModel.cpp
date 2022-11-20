#include "HmckModel.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace std
{
	template <>
	struct hash<Hmck::HmckModel::Vertex>
	{
		size_t operator()(Hmck::HmckModel::Vertex const& vertex) const
		{
			size_t seed = 0;
			Hmck::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

Hmck::HmckModel::HmckModel(HmckDevice& device, const HmckModel::Builder& builder) : hmckDevice{ device }
{
	createVertexBuffers(builder.vertices);
	createIndexBuffers(builder.indices);
}

Hmck::HmckModel::~HmckModel(){}


std::unique_ptr<Hmck::HmckModel> Hmck::HmckModel::createModelFromFile(HmckDevice& device, const std::string& filepath, bool calculateTangents)
{
	Builder builder{};
	ModelInfo mInfo = builder.loadModel(filepath);

	if (calculateTangents)
		builder.calculateTangent();

	std::unique_ptr<HmckModel> model = std::make_unique<HmckModel>(device, builder);
	model->modelInfo = mInfo;

	return model;
}

void Hmck::HmckModel::draw(VkCommandBuffer commandBuffer)
{
	if (hasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
	else 
	{
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void Hmck::HmckModel::bind(VkCommandBuffer commandBuffer)
{
	VkBuffer buffers[] = { vertexBuffer->getBuffer()};
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (hasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer() , 0, VK_INDEX_TYPE_UINT32);
	}
}

void Hmck::HmckModel::createVertexBuffers(const std::vector<Vertex>& vertices)
{
	// copy data to staging memory on device, then copy from staging to v/i memory
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	HmckBuffer stagingBuffer{
		hmckDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};


	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());

	vertexBuffer = std::make_unique<HmckBuffer>(
		hmckDevice,
		vertexSize,
		vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	hmckDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Hmck::HmckModel::createIndexBuffers(const std::vector<uint32_t>& indices)
{
	indexCount = static_cast<uint32_t>(indices.size());

	hasIndexBuffer = indexCount > 0;

	if (!hasIndexBuffer)
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
	uint32_t indexSize = sizeof(indices[0]);

	HmckBuffer stagingBuffer{
		hmckDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	
	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<HmckBuffer>(
		hmckDevice,
		indexSize,
		indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	
	hmckDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

std::vector<VkVertexInputBindingDescription> Hmck::HmckModel::Vertex::getBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription> Hmck::HmckModel::Vertex::getAttributeDescriptions()
{
	return
	{	
		// order is location, binding, format, offset
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
		{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
		{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
	};
}

Hmck::HmckModel::ModelInfo Hmck::HmckModel::Builder::loadModel(const std::string& filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	vertices.clear();
	indices.clear();

	ModelInfo mInfo{};

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto &shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			if (index.vertex_index >= 0)
			{
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.color = {
					attrib.colors[3 * index.vertex_index + 0],
					attrib.colors[3 * index.vertex_index + 1],
					attrib.colors[3 * index.vertex_index + 2]
				};
			}

			if (index.vertex_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0) 
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);

				// fill Info struct
				if (vertex.position.x <= mInfo.x.min)
				{
					mInfo.x.min = vertex.position.x;
				}

				if (vertex.position.x >= mInfo.x.max)
				{
					mInfo.x.max = vertex.position.x;
				}

				if (vertex.position.y <= mInfo.y.min)
				{
					mInfo.y.min = vertex.position.y;
				}

				if (vertex.position.y >= mInfo.y.max)
				{
					mInfo.y.max = vertex.position.y;
				}

				if (vertex.position.z <= mInfo.z.min)
				{
					mInfo.z.min = vertex.position.z;
				}

				if (vertex.position.z >= mInfo.z.max)
				{
					mInfo.z.max = vertex.position.z;
				}
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return mInfo;
}

void Hmck::HmckModel::Builder::calculateTangent()
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(indices.size()); i += 3)
	{
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		glm::vec3 edge1 = vertices[i1].position - vertices[i0].position;
		glm::vec3 edge2 = vertices[i2].position - vertices[i0].position;

		float deltaU1 = vertices[i1].uv.x - vertices[i0].uv.x;
		float deltaV1 = vertices[i1].uv.y - vertices[i0].uv.y;

		float deltaU2 = vertices[i2].uv.x - vertices[i0].uv.x;
		float deltaV2 = vertices[i2].uv.y - vertices[i0].uv.y;

		float divident = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
		float fc = 1.0f / divident;

		glm::vec3 tangent = glm::vec3(
			(fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
			(fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
			(fc * (deltaV2 * edge1.z - deltaV1 * edge2.z))
		);

		tangent = glm::normalize(tangent);
		
		vertices[i0].tangent = tangent;
		vertices[i1].tangent = tangent;
		vertices[i2].tangent = tangent;
	}
}
