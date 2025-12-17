#include "wiShaderCompiler.h"
#include "wiBacklog.h"
#include "wiPlatform.h"
#include "wiHelper.h"
#include "wiArchive.h"
#include "wiUnorderedSet.h"

#include <mutex>

#ifdef PLATFORM_WINDOWS_DESKTOP
#define SHADERCOMPILER_ENABLED
#define SHADERCOMPILER_ENABLED_DXCOMPILER
#define SHADERCOMPILER_ENABLED_D3DCOMPILER
#include <wrl/client.h>
using namespace Microsoft::WRL;
#endif // _WIN32

#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
#define SHADERCOMPILER_ENABLED
#define SHADERCOMPILER_ENABLED_DXCOMPILER
#define __RPC_FAR
#define ComPtr CComPtr
#include "Utility/dxc/WinAdapter.h"
#endif // defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)

#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
#include "Utility/dxc/dxcapi.h"
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER

#ifdef SHADERCOMPILER_ENABLED_D3DCOMPILER
#include <d3dcompiler.h>
#endif // SHADERCOMPILER_ENABLED_D3DCOMPILER

#if __has_include("wiShaderCompiler_XBOX.h")
#include "wiShaderCompiler_XBOX.h"
#define SHADERCOMPILER_XBOX_INCLUDED
#endif // __has_include("wiShaderCompiler_XBOX.h")

#if __has_include("wiShaderCompiler_PS5.h") && !defined(PLATFORM_PS5)
#include "wiShaderCompiler_PS5.h"
#define SHADERCOMPILER_PS5_INCLUDED
#endif // __has_include("wiShaderCompiler_PS5.h") && !PLATFORM_PS5

#ifdef PLATFORM_APPLE
#define SHADERCOMPILER_APPLE_INCLUDED
#include <metal_irconverter/metal_irconverter.h>
#endif // PLATFORM_APPLE

using namespace wi::graphics;

namespace wi::shadercompiler
{

#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
	struct InternalState_DXC
	{
		DxcCreateInstanceProc DxcCreateInstance = nullptr;

		InternalState_DXC(const std::string& modifier = "")
		{
#ifdef _WIN32
			const std::string library = "dxcompiler" + modifier + ".dll";
			HMODULE dxcompiler = wiLoadLibrary(library.c_str());
#elif defined(PLATFORM_LINUX)
			const std::string library = "./libdxcompiler" + modifier + ".so";
			HMODULE dxcompiler = wiLoadLibrary(library.c_str());
#elif defined(PLATFORM_APPLE)
			const std::string library = "./libdxcompiler" + modifier + ".dylib";
			HMODULE dxcompiler = wiLoadLibrary(library.c_str());
#endif
			if (dxcompiler != nullptr)
			{
				DxcCreateInstance = (DxcCreateInstanceProc)wiGetProcAddress(dxcompiler, "DxcCreateInstance");
				if (DxcCreateInstance != nullptr)
				{
					ComPtr<IDxcCompiler3> dxcCompiler;
					HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
					assert(SUCCEEDED(hr));
					ComPtr<IDxcVersionInfo> info;
					hr = dxcCompiler->QueryInterface(IID_PPV_ARGS(&info));
					assert(SUCCEEDED(hr));
					uint32_t minor = 0;
					uint32_t major = 0;
					hr = info->GetVersion(&major, &minor);
					assert(SUCCEEDED(hr));
					wi::backlog::post("wi::shadercompiler: loaded " + library + " (version: " + std::to_string(major) + "." + std::to_string(minor) + ")");
				}
			}
			else
			{
				wi::backlog::post("wi::shadercompiler: could not load library " + library, wi::backlog::LogLevel::Error);
#ifdef PLATFORM_LINUX
				wi::backlog::post(dlerror(), wi::backlog::LogLevel::Error); // print dlopen() error detail: https://linux.die.net/man/3/dlerror
#endif // PLATFORM_LINUX
			}

		}
	};
	inline InternalState_DXC& dxc_compiler()
	{
		static InternalState_DXC internal_state;
		return internal_state;
	}
	inline InternalState_DXC& dxc_compiler_xs()
	{
		static InternalState_DXC internal_state("_xs");
		return internal_state;
	}

	void Compile_DXCompiler(const CompilerInput& input, CompilerOutput& output)
	{
		InternalState_DXC& compiler_internal = input.format == ShaderFormat::HLSL6_XS ? dxc_compiler_xs() : dxc_compiler();
		if (compiler_internal.DxcCreateInstance == nullptr)
		{
			return;
		}

		ComPtr<IDxcUtils> dxcUtils;
		ComPtr<IDxcCompiler3> dxcCompiler;

		HRESULT hr = compiler_internal.DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));
		hr = compiler_internal.DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
		assert(SUCCEEDED(hr));

		if (dxcCompiler == nullptr)
		{
			return;
		}

		wi::vector<uint8_t> shadersourcedata;
		if (!wi::helper::FileRead(input.shadersourcefilename, shadersourcedata))
		{
			return;
		}

		// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface

		wi::vector<std::wstring> args = {
			//L"-res-may-alias",
			//L"-flegacy-macro-expansion",
			//L"-no-legacy-cbuf-layout",
			//L"-pack-optimized", // this has problem with tessellation shaders: https://github.com/microsoft/DirectXShaderCompiler/issues/3362
			//L"-all-resources-bound",
			//L"-Gis", // Force IEEE strictness
			//L"-Gec", // Enable backward compatibility mode
			//L"-Ges", // Enable strict mode
			//L"-O0", // Optimization Level 0
			//L"-enable-16bit-types",
			L"-Wno-conversion",
		};

		if (has_flag(input.flags, Flags::DISABLE_OPTIMIZATION))
		{
			args.push_back(L"-Od");
		}

		switch (input.format)
		{
		case ShaderFormat::HLSL6:
		case ShaderFormat::HLSL6_XS:
		case ShaderFormat::METAL:
			args.push_back(L"-rootsig-define"); args.push_back(L"WICKED_ENGINE_DEFAULT_ROOTSIGNATURE");
			if (has_flag(input.flags, Flags::STRIP_REFLECTION))
			{
				args.push_back(L"-Qstrip_reflect"); // only valid in HLSL6 compiler
			}
			break;
		case ShaderFormat::SPIRV:
			args.push_back(L"-spirv");
			args.push_back(L"-fspv-target-env=vulkan1.3");
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

		ShaderModel minshadermodel = input.minshadermodel;

		// global minshadermodel override:
		//minshadermodel = std::max(minshadermodel, ShaderModel::SM_6_2);

		if (input.format == ShaderFormat::HLSL6_XS)
		{
			minshadermodel = ShaderModel::SM_6_6;
		}
		
		if (input.format == ShaderFormat::METAL)
		{
			//args.push_back(L"-Od");
			args.push_back(L"-Zi");
			args.push_back(L"-Qembed_debug");
		}

		args.push_back(L"-T");
		switch (input.stage)
		{
		case ShaderStage::MS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"ms_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"ms_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"ms_6_7");
				break;
			}
			break;
		case ShaderStage::AS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"as_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"as_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"as_6_7");
				break;
			}
			break;
		case ShaderStage::VS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"vs_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"vs_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"vs_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"vs_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"vs_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"vs_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"vs_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"vs_6_7");
				break;
			}
			break;
		case ShaderStage::HS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"hs_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"hs_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"hs_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"hs_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"hs_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"hs_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"hs_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"hs_6_7");
				break;
			}
			break;
		case ShaderStage::DS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"ds_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"ds_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"ds_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"ds_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"ds_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"ds_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"ds_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"ds_6_7");
				break;
			}
			break;
		case ShaderStage::GS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"gs_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"gs_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"gs_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"gs_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"gs_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"gs_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"gs_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"gs_6_7");
				break;
			}
			break;
		case ShaderStage::PS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"ps_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"ps_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"ps_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"ps_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"ps_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"ps_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"ps_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"ps_6_7");
				break;
			}
			break;
		case ShaderStage::CS:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"cs_6_0");
				break;
			case ShaderModel::SM_6_1:
				args.push_back(L"cs_6_1");
				break;
			case ShaderModel::SM_6_2:
				args.push_back(L"cs_6_2");
				break;
			case ShaderModel::SM_6_3:
				args.push_back(L"cs_6_3");
				break;
			case ShaderModel::SM_6_4:
				args.push_back(L"cs_6_4");
				break;
			case ShaderModel::SM_6_5:
				args.push_back(L"cs_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"cs_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"cs_6_7");
				break;
			}
			break;
		case ShaderStage::LIB:
			switch (minshadermodel)
			{
			default:
				args.push_back(L"lib_6_5");
				break;
			case ShaderModel::SM_6_6:
				args.push_back(L"lib_6_6");
				break;
			case ShaderModel::SM_6_7:
				args.push_back(L"lib_6_7");
				break;
			}
			break;
		default:
			assert(0);
			return;
		}

		for (auto& x : input.defines)
		{
			args.push_back(L"-D");
			wi::helper::StringConvert(x, args.emplace_back());
		}

		for (auto& x : input.include_directories)
		{
			args.push_back(L"-I");
			wi::helper::StringConvert(x, args.emplace_back());
		}

#ifdef SHADERCOMPILER_XBOX_INCLUDED
		if (input.format == ShaderFormat::HLSL6_XS)
		{
			wi::shadercompiler::xbox::AddArguments(input, args);
		}
#endif // SHADERCOMPILER_XBOX_INCLUDED

		// Entry point parameter:
		std::wstring wentry;
		wi::helper::StringConvert(input.entrypoint, wentry);
		args.push_back(L"-E");
		args.push_back(wentry.c_str());

		// Add source file name as last parameter. This will be displayed in error messages
		std::wstring wsource;
		wi::helper::StringConvert(wi::helper::GetFileNameFromPath(input.shadersourcefilename), wsource);
		args.push_back(wsource.c_str());

		DxcBuffer Source;
		Source.Ptr = shadersourcedata.data();
		Source.Size = shadersourcedata.size();
		Source.Encoding = DXC_CP_ACP;

		struct IncludeHandler final : public IDxcIncludeHandler
		{
			const CompilerInput* input = nullptr;
			CompilerOutput* output = nullptr;
			ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

			HRESULT STDMETHODCALLTYPE LoadSource(
				_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
				_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
			) override
			{
				HRESULT hr = dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
				if (SUCCEEDED(hr))
				{
					std::string& filename = output->dependencies.emplace_back();
					wi::helper::StringConvert(pFilename, filename);
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

		hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler.dxcIncludeHandler);
		assert(SUCCEEDED(hr));

		wi::vector<const wchar_t*> args_raw;
		args_raw.reserve(args.size());
		for (auto& x : args)
		{
			args_raw.push_back(x.c_str());
		}

#ifndef _WIN32
		// work around https://github.com/microsoft/DirectXShaderCompiler/issues/7869
		static std::mutex locale_mut;
		static char* prev_locale;
		// we need to use a mutex anyway, so no point in using atomic_int
		static int scope = 0;

		{
			std::scoped_lock lock(locale_mut);
			if (scope++ == 0) {
				prev_locale = strdup(setlocale(LC_ALL, nullptr));
				setlocale(LC_ALL, "en_US.UTF-8");
			}
		}
#endif
		ComPtr<IDxcResult> pResults;
		hr = dxcCompiler->Compile(
			&Source,						// Source buffer.
			args_raw.data(),			// Array of pointers to arguments.
			(uint32_t)args.size(),		// Number of arguments.
			&includehandler,		// User-provided interface to handle #include directives (optional).
			IID_PPV_ARGS(&pResults)	// Compiler output status, buffer, and errors.
		);
#ifndef _WIN32
		{
			std::scoped_lock lock(locale_mut);
			if (--scope == 0) {
				setlocale(LC_ALL, prev_locale);
				free(prev_locale);
			}
		}
#endif
		assert(SUCCEEDED(hr));

		ComPtr<IDxcBlobUtf8> pErrors = nullptr;
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

		ComPtr<IDxcBlob> pShader = nullptr;
		hr = pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr);
		assert(SUCCEEDED(hr));
		if (pShader != nullptr)
		{
			output.dependencies.push_back(input.shadersourcefilename);
			output.shaderdata = (const uint8_t*)pShader->GetBufferPointer();
			output.shadersize = pShader->GetBufferSize();
			
			// keep the blob alive == keep shader pointer valid!
			auto internal_state = wi::allocator::make_shared<ComPtr<IDxcBlob>>();
			*internal_state = pShader;
			output.internal_state = internal_state;
			
#ifdef SHADERCOMPILER_APPLE_INCLUDED
			if (input.format == ShaderFormat::METAL)
			{
				static HMODULE irconverter = wiLoadLibrary("/usr/local/lib/libmetalirconverter.dylib");
				assert(irconverter); // You must install the metal shader converter
				if(irconverter != nullptr)
				{
#define LINK_IR(name) using PFN_##name = decltype(&name); static PFN_##name name = (PFN_##name)wiGetProcAddress(irconverter, #name);
					LINK_IR(IRCompilerCreate)
					LINK_IR(IRCompilerSetMinimumGPUFamily)
					LINK_IR(IRCompilerSetCompatibilityFlags)
					LINK_IR(IRCompilerSetValidationFlags)
					LINK_IR(IRCompilerSetEntryPointName)
					LINK_IR(IRCompilerSetStageInGenerationMode)
					LINK_IR(IRObjectCreateFromDXIL)
					LINK_IR(IRCompilerAllocCompileAndLink)
					LINK_IR(IRErrorDestroy)
					LINK_IR(IRMetalLibBinaryCreate)
					LINK_IR(IRObjectGetMetalLibBinary)
					LINK_IR(IRMetalLibGetBytecodeSize)
					LINK_IR(IRMetalLibGetBytecode)
					LINK_IR(IRMetalLibBinaryDestroy)
					LINK_IR(IRObjectDestroy)
					LINK_IR(IRCompilerDestroy)
					LINK_IR(IRRootSignatureCreateFromDescriptor)
					LINK_IR(IRCompilerSetGlobalRootSignature)
					//LINK_IR(IRRootSignatureDestroy)
					static constexpr uint32_t unbounded = ~0u;
					
					static IRDescriptorRange1 binding_resources[] =
					{
						{ .RangeType = IRDescriptorRangeTypeCBV, .BaseShaderRegister = 3, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = IRDescriptorRangeOffsetAppend, .NumDescriptors = 11, .Flags = IRDescriptorRangeFlagDataStaticWhileSetAtExecute },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = IRDescriptorRangeOffsetAppend, .NumDescriptors = 16, .Flags = IRDescriptorRangeFlagDataStaticWhileSetAtExecute },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = IRDescriptorRangeOffsetAppend, .NumDescriptors = 16, .Flags = IRDescriptorRangeFlagDataStaticWhileSetAtExecute },
					};
					static IRDescriptorRange1 binding_samplers[] =
					{
						{ .RangeType = IRDescriptorRangeTypeSampler, .BaseShaderRegister = 0, .RegisterSpace = 0, .OffsetInDescriptorsFromTableStart = IRDescriptorRangeOffsetAppend, .NumDescriptors = 8, .Flags = IRDescriptorRangeFlagDescriptorsVolatile },
					};
					static IRDescriptorRange1 bindless_samplers[] =
					{
						{ .RangeType = IRDescriptorRangeTypeSampler, .BaseShaderRegister = 0, .RegisterSpace = 1, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
					};
					static IRDescriptorRange1 bindless_resources[] =
					{
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 2, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 3, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 4, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 5, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 6, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 7, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 8, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 9, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 10, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 11, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 12, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 13, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 14, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 15, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 16, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 17, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 18, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 19, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 20, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 21, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 22, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 23, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 24, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 25, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 26, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 27, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 28, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 29, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 30, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 100, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 101, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 102, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 103, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 104, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 105, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 106, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 107, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 108, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 109, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 110, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 111, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 112, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 113, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 114, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeUAV, .BaseShaderRegister = 0, .RegisterSpace = 115, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 200, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 201, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 202, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 203, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 204, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 205, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 206, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 207, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
						{ .RangeType = IRDescriptorRangeTypeSRV, .BaseShaderRegister = 0, .RegisterSpace = 208, .OffsetInDescriptorsFromTableStart = 0, .NumDescriptors = unbounded, .Flags = IRDescriptorRangeFlags(IRDescriptorRangeFlagDescriptorsVolatile | IRDescriptorRangeFlagDataVolatile) },
					};
					static IRRootParameter1 root_parameters[] =
					{
						{
							.ParameterType = IRRootParameterType32BitConstants,
							.Constants = { .ShaderRegister = 999, .Num32BitValues = 16, .RegisterSpace = 0 },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeCBV,
							.Descriptor = { .ShaderRegister = 0, .RegisterSpace = 0, .Flags = IRRootDescriptorFlagNone },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeCBV,
							.Descriptor = { .ShaderRegister = 1, .RegisterSpace = 0, .Flags = IRRootDescriptorFlagNone },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeCBV,
							.Descriptor = { .ShaderRegister = 2, .RegisterSpace = 0, .Flags = IRRootDescriptorFlagNone },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeDescriptorTable,
							.DescriptorTable = { .NumDescriptorRanges = arraysize(binding_resources), .pDescriptorRanges = binding_resources },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeDescriptorTable,
							.DescriptorTable = { .NumDescriptorRanges = arraysize(binding_samplers), .pDescriptorRanges = binding_samplers },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeDescriptorTable,
							.DescriptorTable = { .NumDescriptorRanges = arraysize(bindless_samplers), .pDescriptorRanges = bindless_samplers },
							.ShaderVisibility = IRShaderVisibilityAll
						},
						{
							.ParameterType = IRRootParameterTypeDescriptorTable,
							.DescriptorTable = { .NumDescriptorRanges = arraysize(bindless_resources), .pDescriptorRanges = bindless_resources },
							.ShaderVisibility = IRShaderVisibilityAll
						},
					};
					
					static IRStaticSamplerDescriptor static_samplers[] =
					{
						{ .ShaderRegister = 100, .RegisterSpace = 0, .Filter = IRFilterMinMagMipLinear, .AddressU = IRTextureAddressModeClamp, .AddressV = IRTextureAddressModeClamp, .AddressW = IRTextureAddressModeClamp, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 101, .RegisterSpace = 0, .Filter = IRFilterMinMagMipLinear, .AddressU = IRTextureAddressModeWrap, .AddressV = IRTextureAddressModeWrap, .AddressW = IRTextureAddressModeWrap, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 102, .RegisterSpace = 0, .Filter = IRFilterMinMagMipLinear, .AddressU = IRTextureAddressModeMirror, .AddressV = IRTextureAddressModeMirror, .AddressW = IRTextureAddressModeMirror, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						
						{ .ShaderRegister = 103, .RegisterSpace = 0, .Filter = IRFilterMinMagMipPoint, .AddressU = IRTextureAddressModeClamp, .AddressV = IRTextureAddressModeClamp, .AddressW = IRTextureAddressModeClamp, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 104, .RegisterSpace = 0, .Filter = IRFilterMinMagMipPoint, .AddressU = IRTextureAddressModeWrap, .AddressV = IRTextureAddressModeWrap, .AddressW = IRTextureAddressModeWrap, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 105, .RegisterSpace = 0, .Filter = IRFilterMinMagMipPoint, .AddressU = IRTextureAddressModeMirror, .AddressV = IRTextureAddressModeMirror, .AddressW = IRTextureAddressModeMirror, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						
						{ .ShaderRegister = 106, .RegisterSpace = 0, .Filter = IRFilterAnisotropic, .AddressU = IRTextureAddressModeClamp, .AddressV = IRTextureAddressModeClamp, .AddressW = IRTextureAddressModeClamp, .MipLODBias = 0, .MaxAnisotropy = 16, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 107, .RegisterSpace = 0, .Filter = IRFilterAnisotropic, .AddressU = IRTextureAddressModeWrap, .AddressV = IRTextureAddressModeWrap, .AddressW = IRTextureAddressModeWrap, .MipLODBias = 0, .MaxAnisotropy = 16, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						{ .ShaderRegister = 108, .RegisterSpace = 0, .Filter = IRFilterAnisotropic, .AddressU = IRTextureAddressModeMirror, .AddressV = IRTextureAddressModeMirror, .AddressW = IRTextureAddressModeMirror, .MipLODBias = 0, .MaxAnisotropy = 16, .ComparisonFunc = IRComparisonFunctionNever, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
						
						{ .ShaderRegister = 109, .RegisterSpace = 0, .Filter = IRFilterMinMagMipLinear, .AddressU = IRTextureAddressModeClamp, .AddressV = IRTextureAddressModeClamp, .AddressW = IRTextureAddressModeClamp, .MipLODBias = 0, .MaxAnisotropy = 0, .ComparisonFunc = IRComparisonFunctionGreaterEqual, .BorderColor = IRStaticBorderColorOpaqueBlack, .MinLOD = 0, .MaxLOD = FLT_MAX, .ShaderVisibility = IRShaderVisibilityAll },
					};
					
					static const IRVersionedRootSignatureDescriptor desc = {
						.version = IRRootSignatureVersion_1_1,
						.desc_1_1.Flags = IRRootSignatureFlagAllowInputAssemblerInputLayout,
						.desc_1_1.NumParameters = arraysize(root_parameters),
						.desc_1_1.pParameters = root_parameters,
						.desc_1_1.NumStaticSamplers = arraysize(static_samplers),
						.desc_1_1.pStaticSamplers = static_samplers,
					};
					static IRError* pRootSigError = nullptr;
					static IRRootSignature* pRootSig = IRRootSignatureCreateFromDescriptor(&desc, &pRootSigError);
					if (pRootSig == nullptr)
					{
						assert(0);
						IRErrorDestroy(pRootSigError);
					}
					
					IRCompiler* pCompiler = IRCompilerCreate();
					IRCompilerSetMinimumGPUFamily(pCompiler, IRGPUFamilyMetal3);
					IRCompilerSetCompatibilityFlags(pCompiler, IRCompatibilityFlags(IRCompatibilityFlagBoundsCheck | IRCompatibilityFlagPositionInvariance));
					IRCompilerSetValidationFlags(pCompiler, IRCompilerValidationFlagAll);
					IRCompilerSetEntryPointName(pCompiler, input.entrypoint.c_str());
					IRCompilerSetGlobalRootSignature(pCompiler, pRootSig);
					IRCompilerSetStageInGenerationMode(pCompiler, IRStageInCodeGenerationModeUseMetalVertexFetch);
					IRObject* pDXIL = IRObjectCreateFromDXIL(output.shaderdata, output.shadersize, IRBytecodeOwnershipNone);
					IRError* pError = nullptr;
					IRObject* pOutIR = IRCompilerAllocCompileAndLink(pCompiler, NULL, pDXIL, &pError);
					if (!pOutIR)
					{
						assert(0);
						IRErrorDestroy(pError);
					}
					IRMetalLibBinary* pMetallib = IRMetalLibBinaryCreate();
					IRShaderStage irstage = {};
					switch (input.stage) {
						case ShaderStage::VS:
							irstage = IRShaderStageVertex;
							break;
						case ShaderStage::PS:
							irstage = IRShaderStageFragment;
							break;
						case ShaderStage::GS:
							irstage = IRShaderStageGeometry;
							break;
						case ShaderStage::HS:
							irstage = IRShaderStageHull;
							break;
						case ShaderStage::DS:
							irstage = IRShaderStageDomain;
							break;
						case ShaderStage::MS:
							irstage = IRShaderStageMesh;
							break;
						case ShaderStage::AS:
							irstage = IRShaderStageAmplification;
							break;
						case ShaderStage::CS:
							irstage = IRShaderStageCompute;
							break;
						default:
							assert(0);
							break;
					}
					IRObjectGetMetalLibBinary(pOutIR, irstage, pMetallib);
					size_t metallibSize = IRMetalLibGetBytecodeSize(pMetallib);
					auto internal_state = wi::allocator::make_shared<wi::vector<uint8_t>>(metallibSize); // lifetime storage of pointer
					IRMetalLibGetBytecode(pMetallib, internal_state->data());
					output.internal_state = internal_state;
					output.shaderdata = internal_state->data();
					output.shadersize = internal_state->size();
					IRMetalLibBinaryDestroy(pMetallib);
					IRObjectDestroy(pDXIL);
					IRObjectDestroy(pOutIR);
					IRCompilerDestroy(pCompiler);
					//IRRootSignatureDestroy(pRootSig);
				}
			}
#endif // SHADERCOMPILER_APPLE_INCLUDED
			
		}

		if (input.format == ShaderFormat::HLSL6)
		{
			ComPtr<IDxcBlob> pHash = nullptr;
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

#ifdef SHADERCOMPILER_ENABLED_D3DCOMPILER
	struct InternalState_D3DCompiler
	{
		using PFN_D3DCOMPILE = decltype(&D3DCompile);
		PFN_D3DCOMPILE D3DCompile = nullptr;

		InternalState_D3DCompiler()
		{
			if (D3DCompile != nullptr)
			{
				return; // already initialized
			}

			HMODULE d3dcompiler = wiLoadLibrary("d3dcompiler_47.dll");
			if (d3dcompiler != nullptr)
			{
				D3DCompile = (PFN_D3DCOMPILE)wiGetProcAddress(d3dcompiler, "D3DCompile");
				if (D3DCompile != nullptr)
				{
					wi::backlog::post("wi::shadercompiler: loaded d3dcompiler_47.dll");
				}
			}
		}
	};
	inline InternalState_D3DCompiler& d3d_compiler()
	{
		static InternalState_D3DCompiler internal_state;
		return internal_state;
	}

	void Compile_D3DCompiler(const CompilerInput& input, CompilerOutput& output)
	{
		if (d3d_compiler().D3DCompile == nullptr)
		{
			return;
		}

		if (input.minshadermodel > ShaderModel::SM_5_0)
		{
			output.error_message = "SHADERFORMAT_HLSL5 cannot support specified minshadermodel!";
			return;
		}

		wi::vector<uint8_t> shadersourcedata;
		if (!wi::helper::FileRead(input.shadersourcefilename, shadersourcedata))
		{
			return;
		}

		D3D_SHADER_MACRO defines[] = {
			{"HLSL5", "1"},
			{"DISABLE_WAVE_INTRINSICS", "1"},
			{NULL, NULL},
		};

		const char* target = nullptr;
		switch (input.stage)
		{
		default:
		case ShaderStage::MS:
		case ShaderStage::AS:
		case ShaderStage::LIB:
			// not applicable
			return;
		case ShaderStage::VS:
			target = "vs_5_0";
			break;
		case ShaderStage::HS:
			target = "hs_5_0";
			break;
		case ShaderStage::DS:
			target = "ds_5_0";
			break;
		case ShaderStage::GS:
			target = "gs_5_0";
			break;
		case ShaderStage::PS:
			target = "ps_5_0";
			break;
		case ShaderStage::CS:
			target = "cs_5_0";
			break;
		}

		struct IncludeHandler final : public ID3DInclude
		{
			const CompilerInput* input = nullptr;
			CompilerOutput* output = nullptr;
			wi::vector<wi::vector<uint8_t>> filedatas;

			HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
			{
				for (auto& x : input->include_directories)
				{
					std::string filename = x + pFileName;
					if (!wi::helper::FileExists(filename))
						continue;
					wi::vector<uint8_t>& filedata = filedatas.emplace_back();
					if (wi::helper::FileRead(filename, filedata))
					{
						output->dependencies.push_back(filename);
						*ppData = filedata.data();
						*pBytes = (UINT)filedata.size();
						return S_OK;
					}
				}
				return E_FAIL;
			}

			HRESULT Close(LPCVOID pData) override
			{
				return S_OK;
			}
		} includehandler;
		includehandler.input = &input;
		includehandler.output = &output;

		// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/d3dcompile-constants
		UINT Flags1 = 0;
		if (has_flag(input.flags, Flags::DISABLE_OPTIMIZATION))
		{
			Flags1 |= D3DCOMPILE_SKIP_OPTIMIZATION;
		}


		ComPtr<ID3DBlob> code;
		ComPtr<ID3DBlob> errors;
		HRESULT hr = d3d_compiler().D3DCompile(
			shadersourcedata.data(),
			shadersourcedata.size(),
			input.shadersourcefilename.c_str(),
			defines,
			&includehandler, //D3D_COMPILE_STANDARD_FILE_INCLUDE,
			input.entrypoint.c_str(),
			target,
			Flags1,
			0,
			&code,
			&errors
		);

		if (errors)
		{
			output.error_message = (const char*)errors->GetBufferPointer();
		}

		if (SUCCEEDED(hr))
		{
			output.dependencies.push_back(input.shadersourcefilename);
			output.shaderdata = (const uint8_t*)code->GetBufferPointer();
			output.shadersize = code->GetBufferSize();

			// keep the blob alive == keep shader pointer valid!
			auto internal_state = wi::allocator::make_shared<ComPtr<ID3D10Blob>>();
			*internal_state = code;
			output.internal_state = internal_state;
		}
	}
#endif // SHADERCOMPILER_ENABLED_D3DCOMPILER

	void Compile(const CompilerInput& input, CompilerOutput& output)
	{
		output = CompilerOutput();

#ifdef SHADERCOMPILER_ENABLED
		switch (input.format)
		{
		default:
			break;

#ifdef SHADERCOMPILER_ENABLED_DXCOMPILER
		case ShaderFormat::HLSL6:
		case ShaderFormat::SPIRV:
		case ShaderFormat::HLSL6_XS:
		case ShaderFormat::METAL:
			Compile_DXCompiler(input, output);
			break;
#endif // SHADERCOMPILER_ENABLED_DXCOMPILER

#ifdef SHADERCOMPILER_ENABLED_D3DCOMPILER
		case ShaderFormat::HLSL5:
			Compile_D3DCompiler(input, output);
			break;
#endif // SHADERCOMPILER_ENABLED_D3DCOMPILER

#ifdef SHADERCOMPILER_PS5_INCLUDED
		case ShaderFormat::PS5:
			wi::shadercompiler::ps5::Compile(input, output);
			break;
#endif // SHADERCOMPILER_PS5_INCLUDED

		}
#endif // SHADERCOMPILER_ENABLED
	}

	constexpr const char* shadermetaextension = "wishadermeta";
	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output)
	{
#ifdef SHADERCOMPILER_ENABLED
		wi::helper::DirectoryCreate(wi::helper::GetDirectoryFromPath(shaderfilename));

		wi::Archive dependencyLibrary(wi::helper::ReplaceExtension(shaderfilename, shadermetaextension), false);
		if (dependencyLibrary.IsOpen())
		{
			std::string rootdir = dependencyLibrary.GetSourceDirectory();
			wi::vector<std::string> dependencies = output.dependencies;
			for (auto& x : dependencies)
			{
				wi::helper::MakePathRelative(rootdir, x);
			}
			dependencyLibrary << dependencies;
		}

		if (wi::helper::FileWrite(shaderfilename, output.shaderdata, output.shadersize))
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
		wi::helper::MakePathAbsolute(filepath);
		if (!wi::helper::FileExists(filepath))
		{
			return true; // no shader file = outdated shader, apps can attempt to rebuild it
		}
		std::string dependencylibrarypath = wi::helper::ReplaceExtension(shaderfilename, shadermetaextension);
		if (!wi::helper::FileExists(dependencylibrarypath))
		{
			return false; // no metadata file = no dependency, up to date (for example packaged builds)
		}

		const uint64_t tim = wi::helper::FileTimestamp(filepath);

		wi::Archive dependencyLibrary(dependencylibrarypath);
		if (dependencyLibrary.IsOpen())
		{
			std::string rootdir = dependencyLibrary.GetSourceDirectory();
			wi::vector<std::string> dependencies;
			dependencyLibrary >> dependencies;

			for (auto& x : dependencies)
			{
				std::string dependencypath = rootdir + x;
				wi::helper::MakePathAbsolute(dependencypath);
				if (wi::helper::FileExists(dependencypath))
				{
					const uint64_t dep_tim = wi::helper::FileTimestamp(dependencypath);

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
	wi::unordered_set<std::string> registered_shaders;
	void RegisterShader(const std::string& shaderfilename)
	{
#ifdef SHADERCOMPILER_ENABLED
		std::scoped_lock lock(locker);
		registered_shaders.insert(shaderfilename);
#endif // SHADERCOMPILER_ENABLED
	}
	size_t GetRegisteredShaderCount()
	{
		std::scoped_lock lock(locker);
		return registered_shaders.size();
	}
	bool CheckRegisteredShadersOutdated()
	{
#ifdef SHADERCOMPILER_ENABLED
		std::scoped_lock lock(locker);
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
