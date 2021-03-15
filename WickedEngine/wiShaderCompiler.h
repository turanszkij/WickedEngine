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
		const uint8_t* shadersourcedata = nullptr;
		size_t shadersourcesize = 0;
		std::vector<std::string> include_directories;
		std::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		bool success = false;
		std::vector<uint8_t> shaderbinary;
		std::vector<uint8_t> shaderhash;
		std::string error_message;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);
}
