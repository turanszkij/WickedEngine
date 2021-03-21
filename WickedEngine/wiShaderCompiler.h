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
		wiGraphics::SHADERFORMAT format = wiGraphics::SHADERFORMAT::SHADERFORMAT_NONE;
		wiGraphics::SHADERSTAGE stage = wiGraphics::SHADERSTAGE_COUNT;
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
