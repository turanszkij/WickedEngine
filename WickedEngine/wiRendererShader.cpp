#include "wiRenderer.h"
#include "wiShaderCompiler.h"
#include "wiBacklog.h"

using namespace wi::graphics;

namespace wi::renderer
{

#if __has_include("wiShaderDump.h")
	// In this case, wiShaderDump.h contains precompiled shader binary data
#include "wiShaderDump.h"
#define SHADERDUMP_ENABLED
	size_t GetShaderDumpCount()
	{
		return wiShaderDump::shaderdump.size();
	}
#else
	// In this case, shaders can only be loaded from file
	size_t GetShaderDumpCount()
	{
		return 0;
	}
#endif // SHADERDUMP

#ifdef SHADERDUMP_ENABLED
	// Note: when using Shader Dump, use relative directory, because the dump will contain relative names too
	std::string SHADERPATH = "shaders/";
	std::string SHADERSOURCEPATH = "../WickedEngine/shaders/";
#else
	// Note: when NOT using Shader Dump, use absolute directory, to avoid the case when something (eg. file dialog) overrides working directory
	std::string SHADERPATH = wi::helper::GetCurrentPath() + "/shaders/";
	std::string SHADERSOURCEPATH = SHADER_INTEROP_PATH;
#endif // SHADERDUMP_ENABLED

	const std::string& GetShaderPath()
	{
		return SHADERPATH;
	}
	void SetShaderPath(const std::string& path)
	{
		SHADERPATH = path;
	}
	const std::string& GetShaderSourcePath()
	{
		return SHADERSOURCEPATH;
	}
	void SetShaderSourcePath(const std::string& path)
	{
		SHADERSOURCEPATH = path;
	}
	
	static std::atomic<size_t> SHADER_ERRORS{ 0 };
	static std::atomic<size_t> SHADER_MISSING{ 0 };
	size_t GetShaderErrorCount()
	{
		return SHADER_ERRORS.load();
	}
	size_t GetShaderMissingCount()
	{
		return SHADER_MISSING.load();
	}
	void ResetShaderErrorCount()
	{
		SHADER_ERRORS.store(0);
		SHADER_MISSING.store(0);
	}

	bool LoadShader(
		ShaderStage stage,
		Shader& shader,
		const std::string& filename,
		ShaderModel minshadermodel,
		const wi::vector<std::string>& permutation_defines
	)
	{
		GraphicsDevice* device = GetDevice();

		std::string shaderbinaryfilename = SHADERPATH + filename;

		if (!permutation_defines.empty())
		{
			std::string ext = wi::helper::GetExtensionFromFileName(shaderbinaryfilename);
			shaderbinaryfilename = wi::helper::RemoveExtension(shaderbinaryfilename);
			for (auto& def : permutation_defines)
			{
				shaderbinaryfilename += "_" + def;
			}
			shaderbinaryfilename += "." + ext;
		}

		if (device != nullptr)
		{
#ifdef SHADERDUMP_ENABLED
			// Loading shader from precompiled dump:
			auto it = wiShaderDump::shaderdump.find(shaderbinaryfilename);
			if (it != wiShaderDump::shaderdump.end())
			{
				return device->CreateShader(stage, it->second.data, it->second.size, &shader);
			}
			else
			{
				wi::backlog::post("shader dump doesn't contain shader: " + shaderbinaryfilename, wi::backlog::LogLevel::Error);
			}
#endif // SHADERDUMP_ENABLED
		}

		wi::shadercompiler::RegisterShader(shaderbinaryfilename);

		if (wi::shadercompiler::IsShaderOutdated(shaderbinaryfilename))
		{
			wi::shadercompiler::CompilerInput input;
			input.format = device->GetShaderFormat();
			input.stage = stage;
			input.minshadermodel = minshadermodel;
			input.defines = permutation_defines;

			std::string sourcedir = SHADERSOURCEPATH;
			wi::helper::MakePathAbsolute(sourcedir);
			input.include_directories.push_back(sourcedir);
			input.include_directories.push_back(sourcedir + wi::helper::GetDirectoryFromPath(filename));
			input.shadersourcefilename = wi::helper::ReplaceExtension(sourcedir + filename, "hlsl");

			wi::shadercompiler::CompilerOutput output;
			wi::shadercompiler::Compile(input, output);

			if (output.IsValid())
			{
				wi::shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

				if (!output.error_message.empty())
				{
					wi::backlog::post(output.error_message, wi::backlog::LogLevel::Warning);
				}
				wi::backlog::post("shader compiled: " + shaderbinaryfilename);
				return device->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
			}
			else
			{
				wi::backlog::post("shader compile FAILED: " + shaderbinaryfilename + "\n" + output.error_message, wi::backlog::LogLevel::Error);
				SHADER_ERRORS.fetch_add(1);
			}
		}

		if (device != nullptr)
		{
			wi::vector<uint8_t> buffer;
			if (wi::helper::FileRead(shaderbinaryfilename, buffer))
			{
				bool success = device->CreateShader(stage, buffer.data(), buffer.size(), &shader);
				if (success)
				{
					device->SetName(&shader, shaderbinaryfilename.c_str());
				}
				return success;
			}
			else
			{
				SHADER_MISSING.fetch_add(1);
			}
		}

		return false;
	}

}
