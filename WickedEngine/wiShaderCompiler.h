#pragma once
#include "wiGraphics.h"
#include "wiContainer.h"

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
		wiGraphics::ShaderFormat format = wiGraphics::ShaderFormat::NONE;
		wiGraphics::ShaderStage stage = wiGraphics::ShaderStage::Count;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		wiGraphics::ShaderModel minshadermodel = wiGraphics::ShaderModel::SM_5_0;
		std::string shadersourcefilename;
		std::string entrypoint = "main";
		wiContainer::vector<std::string> include_directories;
		wiContainer::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		const uint8_t* shaderdata = nullptr;
		size_t shadersize = 0;
		wiContainer::vector<uint8_t> shaderhash;
		std::string error_message;
		wiContainer::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);

	void RegisterShader(const std::string& shaderfilename);
	bool CheckRegisteredShadersOutdated();
}
