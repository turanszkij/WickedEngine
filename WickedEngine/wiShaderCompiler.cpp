#include "wiShaderCompiler.h"
#include "wiBackLog.h"
#include "wiPlatform.h"
#include "wiHelper.h"
#include "wiArchive.h"

#include <mutex>
#include <unordered_set>
#include <filesystem>


#ifdef PLATFORM_WINDOWS_DESKTOP
#define SHADERCOMPILER_ENABLED
#define SHADERCOMPILER_ENABLED_DXCOMPILER
#include <atlbase.h> // ComPtr
#endif // _WIN32

#ifdef PLATFORM_LINUX
// TODO: dxcompiler for linux
//#define SHADERCOMPILER_ENABLED_DXCOMPILER
#endif // PLATFORM_LINUX

#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
#include "Utility/dxcapi.h"
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER

namespace wiShaderCompiler
{
#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
	CComPtr<IDxcUtils> dxcUtils;
	CComPtr<IDxcCompiler3> dxcCompiler;
	CComPtr<IDxcIncludeHandler> dxcIncludeHandler;
	DxcCreateInstanceProc DxcCreateInstance = nullptr;
	void Compile_DXCompiler(const CompilerInput& input, CompilerOutput& output)
	{
		if (dxcCompiler == nullptr)
		{
			return;
		}

		std::vector<uint8_t> shadersourcedata;
		if (!wiHelper::FileRead(input.shadersourcefilename, shadersourcedata))
		{
			return;
		}

		// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface

		std::vector<LPCWSTR> args = {
			L"-res-may-alias",
			L"-flegacy-macro-expansion",
			//L"-no-legacy-cbuf-layout",
			//L"-pack-optimized",
			//L"-all-resources-bound",
			//L"-Gis", // Force IEEE strictness
			//L"-Gec", // Enable backward compatibility mode
			//L"-Ges", // Enable strict mode
			//L"-O0", // Optimization Level 0
		};

		if (input.flags & FLAG_DISABLE_OPTIMIZATION)
		{
			args.push_back(L"-Od");
		}

		switch (input.format)
		{
		case wiGraphics::SHADERFORMAT_DXIL:
			args.push_back(L"-D"); args.push_back(L"HLSL6");
			break;
		case wiGraphics::SHADERFORMAT_SPIRV:
			args.push_back(L"-D"); args.push_back(L"SPIRV");
			args.push_back(L"-spirv");
			args.push_back(L"-fspv-target-env=vulkan1.2");
			args.push_back(L"-fvk-use-dx-layout");
			args.push_back(L"-fvk-use-dx-position-w");
			//args.push_back(L"-fvk-b-shift"); args.push_back(L"0"); args.push_back(L"0");
			args.push_back(L"-fvk-t-shift"); args.push_back(L"1000"); args.push_back(L"0");
			args.push_back(L"-fvk-u-shift"); args.push_back(L"2000"); args.push_back(L"0");
			args.push_back(L"-fvk-s-shift"); args.push_back(L"3000"); args.push_back(L"0");
			break;
		default:
			assert(0);
			return;
		}

		args.push_back(L"-T");
		switch (input.stage)
		{
		case wiGraphics::MS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"ms_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"ms_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"ms_6_7");
				break;
			}
			break;
		case wiGraphics::AS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"as_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"as_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"as_6_7");
				break;
			}
			break;
		case wiGraphics::VS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"vs_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"vs_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"vs_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"vs_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"vs_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"vs_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"vs_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"vs_6_7");
				break;
			}
			break;
		case wiGraphics::HS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"hs_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"hs_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"hs_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"hs_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"hs_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"hs_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"hs_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"hs_6_7");
				break;
			}
			break;
		case wiGraphics::DS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"ds_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"ds_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"ds_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"ds_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"ds_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"ds_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"ds_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"ds_6_7");
				break;
			}
			break;
		case wiGraphics::GS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"gs_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"gs_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"gs_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"gs_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"gs_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"gs_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"gs_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"gs_6_7");
				break;
			}
			break;
		case wiGraphics::PS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"ps_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"ps_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"ps_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"ps_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"ps_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"ps_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"ps_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"ps_6_7");
				break;
			}
			break;
		case wiGraphics::CS:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"cs_6_0");
				break;
			case wiGraphics::SHADERMODEL_6_1:
				args.push_back(L"cs_6_1");
				break;
			case wiGraphics::SHADERMODEL_6_2:
				args.push_back(L"cs_6_2");
				break;
			case wiGraphics::SHADERMODEL_6_3:
				args.push_back(L"cs_6_3");
				break;
			case wiGraphics::SHADERMODEL_6_4:
				args.push_back(L"cs_6_4");
				break;
			case wiGraphics::SHADERMODEL_6_5:
				args.push_back(L"cs_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"cs_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"cs_6_7");
				break;
			}
			break;
		case wiGraphics::LIB:
			switch (input.minshadermodel)
			{
			default:
				args.push_back(L"lib_6_5");
				break;
			case wiGraphics::SHADERMODEL_6_6:
				args.push_back(L"lib_6_6");
				break;
			case wiGraphics::SHADERMODEL_6_7:
				args.push_back(L"lib_6_7");
				break;
			}
			break;
		default:
			assert(0);
			return;
		}

		std::vector<std::wstring> wstrings;
		wstrings.reserve(input.defines.size() + input.include_directories.size());

		for (auto& x : input.defines)
		{
			std::wstring& wstr = wstrings.emplace_back();
			wiHelper::StringConvert(x, wstr);
			args.push_back(L"-D");
			args.push_back(wstr.c_str());
		}

		for (auto& x : input.include_directories)
		{
			std::wstring& wstr = wstrings.emplace_back();
			wiHelper::StringConvert(x, wstr);
			args.push_back(L"-I");
			args.push_back(wstr.c_str());
		}

		// Entry point parameter:
		std::wstring wentry;
		wiHelper::StringConvert(input.entrypoint, wentry);
		args.push_back(L"-E");
		args.push_back(wentry.c_str());

		// Add source file name as last parameter. This will be displayed in error messages
		std::wstring wsource;
		wiHelper::StringConvert(wiHelper::GetFileNameFromPath(input.shadersourcefilename), wsource);
		args.push_back(wsource.c_str());

		DxcBuffer Source;
		Source.Ptr = shadersourcedata.data();
		Source.Size = shadersourcedata.size();
		Source.Encoding = DXC_CP_ACP;

		struct IncludeHandler : public IDxcIncludeHandler
		{
			const CompilerInput* input = nullptr;
			CompilerOutput* output = nullptr;

			HRESULT STDMETHODCALLTYPE LoadSource(
				_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
				_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
			) override
			{
				HRESULT hr = dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
				if (SUCCEEDED(hr))
				{
					std::string& filename = output->dependencies.emplace_back();
					wiHelper::StringConvert(pFilename, filename);
				}
				return hr;
			}
			HRESULT STDMETHODCALLTYPE QueryInterface(
				/* [in] */ REFIID riid,
				/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
			{
				return dxcIncludeHandler->QueryInterface(riid, ppvObject);
			}

			ULONG STDMETHODCALLTYPE AddRef(void) override
			{
				return 0;
			}
			ULONG STDMETHODCALLTYPE Release(void) override
			{
				return 0;
			}
		} includehandler;
		includehandler.input = &input;
		includehandler.output = &output;

		CComPtr<IDxcResult> pResults;
		HRESULT hr = dxcCompiler->Compile(
			&Source,                // Source buffer.
			args.data(),            // Array of pointers to arguments.
			(uint32_t)args.size(),	// Number of arguments.
			&includehandler,		// User-provided interface to handle #include directives (optional).
			IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
		);
		assert(SUCCEEDED(hr));

		CComPtr<IDxcBlobUtf8> pErrors = nullptr;
		hr = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
		assert(SUCCEEDED(hr));
		if (pErrors != nullptr && pErrors->GetStringLength() != 0)
		{
			output.error_message = pErrors->GetStringPointer();
		}

		HRESULT hrStatus;
		hr = pResults->GetStatus(&hrStatus);
		assert(SUCCEEDED(hr));
		if (FAILED(hrStatus))
		{
			return;
		}

		CComPtr<IDxcBlob> pShader = nullptr;
		hr = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
		assert(SUCCEEDED(hr));
		if (pShader != nullptr)
		{
			output.dependencies.push_back(input.shadersourcefilename);
			output.shaderdata = (const uint8_t*)pShader->GetBufferPointer();
			output.shadersize = pShader->GetBufferSize();

			// keep the blob alive == keep shader pointer valid!
			auto internal_state = std::make_shared<CComPtr<IDxcBlob>>();
			*internal_state = pShader;
			output.internal_state = internal_state;
		}

		if (input.format == wiGraphics::SHADERFORMAT_DXIL)
		{
			CComPtr<IDxcBlob> pHash = nullptr;
			hr = pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHash), nullptr);
			assert(SUCCEEDED(hr));
			if (pHash != nullptr)
			{
				DxcShaderHash* pHashBuf = (DxcShaderHash*)pHash->GetBufferPointer();
				for (int i = 0; i < _countof(pHashBuf->HashDigest); i++)
				{
					output.shaderhash.push_back(pHashBuf->HashDigest[i]);
				}
			}
		}
	}
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER

	void Initialize()
	{
#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
		HMODULE dxcompiler = wiLoadLibrary("dxcompiler.dll");
		if(dxcompiler != nullptr)
		{
			DxcCreateInstance = (DxcCreateInstanceProc)wiGetProcAddress(dxcompiler, "DxcCreateInstance");
			if (DxcCreateInstance != nullptr)
			{
				HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
				assert(SUCCEEDED(hr));
				hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
				assert(SUCCEEDED(hr));
				hr = dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler);
				assert(SUCCEEDED(hr));
				wiBackLog::post("wiShaderCompiler: loaded dxcompiler.dll");
			}
		}
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER
	}

	void Compile(const CompilerInput& input, CompilerOutput& output)
	{
		output = CompilerOutput();

#ifdef SHADERCOMPILER_ENABLED
		switch (input.format)
		{
		default:
			break;

#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
		case wiGraphics::SHADERFORMAT_DXIL:
		case wiGraphics::SHADERFORMAT_SPIRV:
			Compile_DXCompiler(input, output);
			break;
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER

		}
#endif // SHADERCOMPILER_ENABLED
	}

	static const char* shadermetaextension = "wishadermeta";
	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output)
	{
#ifdef SHADERCOMPILER_ENABLED
		wiHelper::DirectoryCreate(wiHelper::GetDirectoryFromPath(shaderfilename));

		wiArchive dependencyLibrary(wiHelper::ReplaceExtension(shaderfilename, shadermetaextension), false);
		if (dependencyLibrary.IsOpen())
		{
			std::string rootdir = dependencyLibrary.GetSourceDirectory();
			std::vector<std::string> dependencies = output.dependencies;
			for (auto& x : dependencies)
			{
				wiHelper::MakePathRelative(rootdir, x);
			}
			dependencyLibrary << dependencies;
		}

		if (wiHelper::FileWrite(shaderfilename, output.shaderdata, output.shadersize))
		{
			return true;
		}
#endif // SHADERCOMPILER_ENABLED

		return false;
	}
	bool IsShaderOutdated(const std::string& shaderfilename)
	{
#ifdef SHADERCOMPILER_ENABLED
		std::string filepath = shaderfilename;
		wiHelper::MakePathAbsolute(filepath);
		if (!wiHelper::FileExists(filepath))
		{
			return true; // no shader file = outdated shader, apps can attempt to rebuild it
		}
		std::string dependencylibrarypath = wiHelper::ReplaceExtension(shaderfilename, shadermetaextension);
		if (!wiHelper::FileExists(dependencylibrarypath))
		{
			return false; // no metadata file = no dependency, up to date (for example packaged builds)
		}

		const auto tim = std::filesystem::last_write_time(filepath);

		wiArchive dependencyLibrary(dependencylibrarypath);
		if (dependencyLibrary.IsOpen())
		{
			std::string rootdir = dependencyLibrary.GetSourceDirectory();
			std::vector<std::string> dependencies;
			dependencyLibrary >> dependencies;

			for (auto& x : dependencies)
			{
				std::string dependencypath = rootdir + x;
				wiHelper::MakePathAbsolute(dependencypath);
				if (wiHelper::FileExists(dependencypath))
				{
					const auto dep_tim = std::filesystem::last_write_time(dependencypath);

					if (tim < dep_tim)
					{
						return true;
					}
				}
			}
		}
#endif // SHADERCOMPILER_ENABLED

		return false;
	}

	std::mutex locker;
	std::unordered_set<std::string> registered_shaders;
	void RegisterShader(const std::string& shaderfilename)
	{
#ifdef SHADERCOMPILER_ENABLED
		locker.lock();
		registered_shaders.insert(shaderfilename);
		locker.unlock();
#endif // SHADERCOMPILER_ENABLED
	}
	bool CheckRegisteredShadersOutdated()
	{
#ifdef SHADERCOMPILER_ENABLED
		for (auto& x : registered_shaders)
		{
			if (IsShaderOutdated(x))
			{
				return true;
			}
		}
#endif // SHADERCOMPILER_ENABLED
		return false;
	}
}
