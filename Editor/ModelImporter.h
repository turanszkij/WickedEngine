#pragma once
#include <string>

namespace wiSceneComponents
{
	struct Model;
}

wiSceneComponents::Model* ImportModel_WIO(const std::string& fileName);
wiSceneComponents::Model* ImportModel_OBJ(const std::string& fileName);
wiSceneComponents::Model* ImportModel_GLTF(const std::string& fileName);

