#pragma once
#include <string>

struct wiScene::Scene;

void ImportModel_OBJ(const std::string& fileName, wiScene::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, wiScene::Scene& scene);

