#pragma once
#include <string>

struct wiSceneSystem::Scene;

void ImportModel_OBJ(const std::string& fileName, wiSceneSystem::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, wiSceneSystem::Scene& scene);

