#pragma once

#include "glm/glm.hpp"

#include <vector>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

bool Mesh_LoadFromOBJ(const char* filename, std::vector<Vertex>& outVertices, std::vector<uint16_t>& outIndices);
