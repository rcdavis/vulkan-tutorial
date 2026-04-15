#include "Mesh.h"

#include "tiny_obj_loader.h"
#include "Utils/Log.h"

bool Mesh_LoadFromOBJ(const char* filename, std::vector<Vertex>& outVertices, std::vector<uint16_t>& outIndices) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename);

	if (!warn.empty()) {
		LOG_WARN("Warning while loading OBJ file \"{}\": {}", filename, warn);
	}

	if (!err.empty()) {
		LOG_ERROR("Error while loading OBJ file \"{}\": {}", filename, err);
	}

	if (!success) {
		LOG_ERROR("Failed to load OBJ file \"{}\"", filename);
		return false;
	}

	outVertices.clear();
	outIndices.clear();

	for (const auto& shape : shapes) {
		for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
			Vertex vertex;

			const auto& idx = shape.mesh.indices[i];
			vertex.pos = glm::vec3(
				attrib.vertices[3 * idx.vertex_index + 0],
				-attrib.vertices[3 * idx.vertex_index + 1],
				attrib.vertices[3 * idx.vertex_index + 2]
			);

			const auto& normal_idx = shape.mesh.indices[i + 1];
			vertex.normal = glm::vec3(
				attrib.normals[3 * normal_idx.normal_index + 0],
				-attrib.normals[3 * normal_idx.normal_index + 1],
				attrib.normals[3 * normal_idx.normal_index + 2]
			);

			const auto& texcoord_idx = shape.mesh.indices[i + 2];
			vertex.texCoord = glm::vec2(
				attrib.texcoords[2 * texcoord_idx.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * texcoord_idx.texcoord_index + 1]
			);

			outVertices.push_back(vertex);
			outIndices.push_back((uint16_t)outIndices.size());
		}
	}

	return true;
}
