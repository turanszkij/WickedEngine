#pragma once
#include "wiGraphics.h"
#include "wiVector.h"

#include <string>

namespace wi::shadercompiler
{

	enum class Flags
	{
		NONE = 0,
		DISABLE_OPTIMIZATION = 1 << 0,
		STRIP_REFLECTION = 1 << 1,
	};
	struct CompilerInput
	{
		Flags flags = Flags::NONE;
		wi::graphics::ShaderFormat format = wi::graphics::ShaderFormat::NONE;
		wi::graphics::ShaderStage stage = wi::graphics::ShaderStage::Count;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		wi::graphics::ShaderModel minshadermodel = wi::graphics::ShaderModel::SM_5_0;
		std::string shadersourcefilename;
		std::string entrypoint = "main";
		wi::vector<std::string> include_directories;
		wi::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		const uint8_t* shaderdata = nullptr;
		size_t shadersize = 0;
		wi::vector<uint8_t> shaderhash;
		std::string error_message;
		wi::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);

	void RegisterShader(const std::string& shaderfilename);
	size_t GetRegisteredShaderCount();
	bool CheckRegisteredShadersOutdated();
}

template<>
struct enable_bitmask_operators<wi::shadercompiler::Flags> {
	static const bool enable = true;
};
