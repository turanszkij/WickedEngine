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
	};
	struct CompilerInput
	{
		wiGraphics::SHADERFORMAT format = wiGraphics::SHADERFORMAT::SHADERFORMAT_NONE;
		uint64_t flags = FLAG_NONE;
		wiGraphics::SHADERSTAGE stage = wiGraphics::SHADERSTAGE_COUNT;
		std::string shadersourcefilename;
		std::vector<std::string> include_directories;
		std::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		bool success = false;
		std::vector<uint8_t> shaderbinary;
		std::vector<uint8_t> shaderhash;
		std::string error_message;
		std::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);
}
