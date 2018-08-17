#pragma once
#include <string>

struct Model;

Model* ImportModel_WIO(const std::string& fileName);
Model* ImportModel_OBJ(const std::string& fileName);
Model* ImportModel_GLTF(const std::string& fileName);

