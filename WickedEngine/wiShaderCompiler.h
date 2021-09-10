#pragma once
#include "wiGraphics.h"

#include <vector>
#include <string>

namespace wiShaderCompiler
{
	void Initialize();

	enum FLAGS
	{
		FLAG_NONE = 0,
		FLAG_DISABLE_OPTIMIZATION = 1 << 0,
	};
	struct CompilerInput
	{
		uint64_t flags = FLAG_NONE;
		wiGraphics::SHADERFORMAT format = wiGraphics::SHADERFORMAT::SHADERFORMAT_DXIL;
		wiGraphics::SHADERSTAGE stage = wiGraphics::SHADERSTAGE_COUNT;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		wiGraphics::SHADERMODEL minshadermodel = wiGraphics::SHADERMODEL_6_0;
		std::string shadersourcefilename;
		std::string entrypoint = "main";
		std::vector<std::string> include_directories;
		std::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		const uint8_t* shaderdata = nullptr;
		size_t shadersize = 0;
		std::vector<uint8_t> shaderhash;
		std::string error_message;
		std::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);

	void RegisterShader(const std::string& shaderfilename);
	bool CheckRegisteredShadersOutdated();
}
