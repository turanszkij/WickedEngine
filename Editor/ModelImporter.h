#pragma once
#include <string>

namespace wi::scene
{
	struct Scene;
}

void ImportModel_OBJ(const std::string& fileName, wi::scene::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, wi::scene::Scene& scene);
void ExportModel_GLTF(const std::string& filename, wi::scene::Scene& scene);
