#include "wiShaderCompiler.h"
#include "wiBackLog.h"
#include "wiPlatform.h"
#include "wiHelper.h"
#include "wiArchive.h"

#ifdef _WIN32

#include <mutex>
#include <unordered_set>
#include <filesystem>
#include <atlbase.h> // ComPtr

#include "Utility/dxcapi.h"

namespace wiShaderCompiler
{
	CComPtr<IDxcUtils> dxcUtils;
	CComPtr<IDxcCompiler3> dxcCompiler;
	CComPtr<IDxcIncludeHandler> dxcIncludeHandler;

	void Initialize()
	{
		DxcCreateInstanceProc DxcCreateInstance = nullptr;

#ifdef _WIN32
#ifdef PLATFORM_UWP
		return; // Shader compiling not supported on this platform!
#else
		HMODULE dxcompiler = LoadLibrary(L"dxcompiler.dll");
		if (dxcompiler == NULL)
			return;
		DxcCreateInstance = (DxcCreateInstanceProc)GetProcAddress(dxcompiler, "DxcCreateInstance");
		assert(DxcCreateInstance != nullptr);
#endif // PLATFORM_UWP
#else
		wiBackLog::post("wiShaderCompiler not implemented!");
		return;
#endif // _WIN32

		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
		assert(SUCCEEDED(hr));
		dxcUtils->CreateDefaultIncludeHandler(&dxcIncludeHandler);

		wiBackLog::post("wiShaderCompiler Intialized");
	}

	void Compile(const CompilerInput& input, CompilerOutput& output)
	{
		output = CompilerOutput();

		if (dxcCompiler == nullptr)
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
		};

		switch (input.format)
		{
		case wiGraphics::SHADERFORMAT_HLSL6:
			args.push_back(L"-D");
			args.push_back(L"HLSL6");
			break;
		case wiGraphics::SHADERFORMAT_SPIRV:
			args.push_back(L"-D");
			args.push_back(L"SPIRV");
			args.push_back(L"-spirv");
			args.push_back(L"-fspv-target-env=vulkan1.2");
			args.push_back(L"-fvk-use-dx-layout");
			//args.push_back(L"-fvk-b-shift"); args.push_back(L"0"); args.push_back(L"0");
			args.push_back(L"-fvk-t-shift"); args.push_back(L"1000"); args.push_back(L"0");
			args.push_back(L"-fvk-u-shift"); args.push_back(L"2000"); args.push_back(L"0");
			args.push_back(L"-fvk-s-shift"); args.push_back(L"3000"); args.push_back(L"0");
			args.push_back(L"-Vd"); // DISABLE VALIDATION: There is currently a validation bug with raytracing RayTCurrent()!!!
			break;
		default:
			assert(0);
			return;
		}

		args.push_back(L"-T");
		switch (input.stage)
		{
		case wiGraphics::MS:
			args.push_back(L"ms_6_5");
			break;
		case wiGraphics::AS:
			args.push_back(L"as_6_5");
			break;
		case wiGraphics::VS:
			args.push_back(L"vs_6_5");
			break;
		case wiGraphics::HS:
			args.push_back(L"hs_6_5");
			break;
		case wiGraphics::DS:
			args.push_back(L"ds_6_5");
			break;
		case wiGraphics::GS:
			args.push_back(L"gs_6_5");
			break;
		case wiGraphics::PS:
			args.push_back(L"ps_6_5");
			break;
		case wiGraphics::CS:
			args.push_back(L"cs_6_5");
			break;
		case wiGraphics::LIB:
			args.push_back(L"lib_6_5");
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

		std::vector<uint8_t> shadersourcedata;
		if (!wiHelper::FileRead(input.shadersourcefilename, shadersourcedata))
		{
			return;
		}

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
				HRESULT hr =  dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
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

		if (input.format == wiGraphics::SHADERFORMAT_HLSL6)
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

	static const char* shadermetaextension = "wishadermeta";
	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output)
	{
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

		return wiHelper::FileWrite(shaderfilename, output.shaderdata, output.shadersize);
	}
	bool IsShaderOutdated(const std::string& shaderfilename)
	{
		std::filesystem::path filepath = std::filesystem::absolute(shaderfilename);
		if (!std::filesystem::exists(filepath))
		{
			return false;
		}
		std::filesystem::path dependencylibrarypath = wiHelper::ReplaceExtension(shaderfilename, shadermetaextension);
		if (!std::filesystem::exists(dependencylibrarypath))
		{
			return false;
		}

		const auto tim = std::filesystem::last_write_time(filepath);

		wiArchive dependencyLibrary(dependencylibrarypath.string());
		if (dependencyLibrary.IsOpen())
		{
			std::string rootdir = dependencyLibrary.GetSourceDirectory();
			std::vector<std::string> dependencies;
			dependencyLibrary >> dependencies;

			for (auto& x : dependencies)
			{
				std::filesystem::path dependencypath = std::filesystem::absolute(rootdir + x);
				if (std::filesystem::exists(dependencypath))
				{
					const auto dep_tim = std::filesystem::last_write_time(dependencypath);

					if (tim < dep_tim)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	std::mutex locker;
	std::unordered_set<std::string> registered_shaders;
	void RegisterShader(const std::string& shaderfilename)
	{
		locker.lock();
		registered_shaders.insert(shaderfilename);
		locker.unlock();
	}
	bool CheckRegisteredShadersOutdated()
	{
		for (auto& x : registered_shaders)
		{
			if (IsShaderOutdated(x))
			{
				return true;
			}
		}
		return false;
	}
}

#else

namespace wiShaderCompiler
{
	void Initialize() {}
	void Compile(const CompilerInput& input, CompilerOutput& output) {}
	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output) { return false; }
	bool IsShaderOutdated(const std::string& shaderfilename) { return false; }
	void RegisterShader(const std::string& shaderfilename) {}
	bool CheckRegisteredShadersOutdated() { return false; }
}

#endif // WIN32
