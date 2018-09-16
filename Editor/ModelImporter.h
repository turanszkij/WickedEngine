#pragma once
#include "wiECS.h"

#include <string>

wiECS::Entity ImportModel_OBJ(const std::string& fileName);
wiECS::Entity ImportModel_GLTF(const std::string& fileName);

