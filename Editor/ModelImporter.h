#pragma once

void ImportModel_OBJ(const std::string& fileName, wi::scene::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, wi::scene::Scene& scene);
void ExportModel_GLTF(const std::string& filename, wi::scene::Scene& scene);
void ImportModel_FBX(const std::string& fileName, wi::scene::Scene& scene);
