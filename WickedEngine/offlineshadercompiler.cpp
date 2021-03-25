#include "WickedEngine.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <sstream>

std::mutex locker;
std::vector<std::string> shaders[wiGraphics::SHADERSTAGE_COUNT];
struct Target
{
	wiGraphics::SHADERFORMAT format;
	std::string dir;
};
std::vector<Target> targets;
std::unordered_map<std::string, wiShaderCompiler::CompilerOutput> results;
bool rebuild = false;
bool shaderdump_enabled = false;

int main(int argc, char* argv[])
{
	std::cout << "[Wicked Engine Offline Shader Compiler]" << std::endl;
	std::cout << "Available command arguments:" << std::endl;
	std::cout << "\thlsl5 : \tCompile shaders to hlsl5 (dx11) format (using d3dcompiler)" << std::endl;
	std::cout << "\thlsl6 : \tCompile shaders to hlsl6 (dx12) format (using dxcompiler)" << std::endl;
	std::cout << "\tspirv : \tCompile shaders to spirv (vulkan) format (using dxcompiler)" << std::endl;
	std::cout << "\trebuild : \tAll shaders will be rebuilt, regardless if they are outdated or not" << std::endl;
	std::cout << "\tshaderdump : \tShaders will be saved to wiShaderDump.h C++ header file (rebuild is assumed)" << std::endl;
	std::cout << "Command arguments used: ";

	wiStartupArguments::Parse(argc, argv);

	if (wiStartupArguments::HasArgument("hlsl5"))
	{
		targets.push_back({ wiGraphics::SHADERFORMAT_HLSL5, "shaders/hlsl5/" });
		std::cout << "hlsl5 ";
	}
	if (wiStartupArguments::HasArgument("hlsl6"))
	{
		targets.push_back({ wiGraphics::SHADERFORMAT_HLSL6, "shaders/hlsl6/" });
		std::cout << "hlsl6 ";
	}
	if (wiStartupArguments::HasArgument("spirv"))
	{
		targets.push_back({ wiGraphics::SHADERFORMAT_SPIRV, "shaders/spirv/" });
		std::cout << "spirv ";
	}

	if (wiStartupArguments::HasArgument("shaderdump"))
	{
		shaderdump_enabled = true;
		rebuild = true;
		std::cout << "shaderdump ";
	}

	if (wiStartupArguments::HasArgument("rebuild"))
	{
		rebuild = true;
		std::cout << "rebuild ";
	}

	std::cout << std::endl;

	if (targets.empty())
	{
		targets = {
			{ wiGraphics::SHADERFORMAT_HLSL5, "shaders/hlsl5/" },
			{ wiGraphics::SHADERFORMAT_HLSL6, "shaders/hlsl6/" },
			{ wiGraphics::SHADERFORMAT_SPIRV, "shaders/spirv/" },
		};
		std::cout << "No shader formats were specified, assuming command arguments: hlsl5 spirv hlsl6" << std::endl;
	}

	shaders[wiGraphics::CS] = {
		"hairparticle_simulateCS.hlsl"								,
		"hairparticle_finishUpdateCS.hlsl"							,
		"emittedparticle_simulateCS.hlsl"							,
		"generateMIPChainCubeCS_float4.hlsl"						,
		"generateMIPChainCubeCS_unorm4.hlsl"						,
		"generateMIPChainCubeArrayCS_float4.hlsl"					,
		"generateMIPChainCubeArrayCS_unorm4.hlsl"					,
		"generateMIPChain3DCS_float4.hlsl"							,
		"generateMIPChain3DCS_unorm4.hlsl"							,
		"generateMIPChain2DCS_float4.hlsl"							,
		"generateMIPChain2DCS_unorm4.hlsl"							,
		"blur_gaussian_float4CS.hlsl"								,
		"bloomseparateCS.hlsl"										,
		"depthoffield_mainCS.hlsl"									,
		"depthoffield_neighborhoodMaxCOCCS.hlsl"					,
		"depthoffield_prepassCS.hlsl"								,
		"depthoffield_upsampleCS.hlsl"								,
		"depthoffield_tileMaxCOC_verticalCS.hlsl"					,
		"depthoffield_tileMaxCOC_horizontalCS.hlsl"					,
		"voxelRadianceSecondaryBounceCS.hlsl"						,
		"voxelSceneCopyClearCS.hlsl"								,
		"voxelClearOnlyNormalCS.hlsl"								,
		"upsample_bilateral_float1CS.hlsl"							,
		"upsample_bilateral_float4CS.hlsl"							,
		"upsample_bilateral_unorm1CS.hlsl"							,
		"upsample_bilateral_unorm4CS.hlsl"							,
		"temporalaaCS.hlsl"											,
		"tileFrustumsCS.hlsl"										,
		"tonemapCS.hlsl"											,
		"ssr_resolveCS.hlsl"										,
		"ssr_temporalCS.hlsl"										,
		"ssaoCS.hlsl"												,
		"ssr_medianCS.hlsl"											,
		"ssr_raytraceCS.hlsl"										,
		"sharpenCS.hlsl"											,
		"skinningCS.hlsl"											,
		"skinningCS_LDS.hlsl"										,
		"resolveMSAADepthStencilCS.hlsl"							,
		"raytrace_shadeCS.hlsl"										,
		"raytrace_tilesortCS.hlsl"									,
		"raytrace_kickjobsCS.hlsl"									,
		"raytrace_launchCS.hlsl"									,
		"paint_textureCS.hlsl"										,
		"raytrace_closesthitCS.hlsl"								,
		"oceanUpdateDisplacementMapCS.hlsl"							,
		"oceanUpdateGradientFoldingCS.hlsl"							,
		"oceanSimulatorCS.hlsl"										,
		"msao_interleaveCS.hlsl"									,
		"msao_preparedepthbuffers1CS.hlsl"							,
		"msao_preparedepthbuffers2CS.hlsl"							,
		"msao_blurupsampleCS.hlsl"									,
		"msao_blurupsampleCS_blendout.hlsl"							,
		"msao_blurupsampleCS_premin.hlsl"							,
		"msao_blurupsampleCS_premin_blendout.hlsl"					,
		"msaoCS.hlsl"												,
		"motionblur_neighborhoodMaxVelocityCS.hlsl"					,
		"motionblur_tileMaxVelocity_horizontalCS.hlsl"				,
		"motionblur_tileMaxVelocity_verticalCS.hlsl"				,
		"luminancePass2CS.hlsl"										,
		"motionblur_kickjobsCS.hlsl"								,
		"motionblurCS.hlsl"											,
		"motionblurCS_cheap.hlsl"									,
		"motionblurCS_earlyexit.hlsl"								,
		"lineardepthCS.hlsl"										,
		"luminancePass1CS.hlsl"										,
		"lightShaftsCS.hlsl"										,
		"lightCullingCS_ADVANCED_DEBUG.hlsl"						,
		"lightCullingCS_DEBUG.hlsl"									,
		"lightCullingCS.hlsl"										,
		"lightCullingCS_ADVANCED.hlsl"								,
		"hbaoCS.hlsl"												,
		"gpusortlib_sortInnerCS.hlsl"								,
		"gpusortlib_sortStepCS.hlsl"								,
		"gpusortlib_kickoffSortCS.hlsl"								,
		"gpusortlib_sortCS.hlsl"									,
		"fxaaCS.hlsl"												,
		"filterEnvMapCS.hlsl"										,
		"fft_512x512_c2c_CS.hlsl"									,
		"fft_512x512_c2c_v2_CS.hlsl"								,
		"emittedparticle_sphpartitionCS.hlsl"						,
		"emittedparticle_sphpartitionoffsetsCS.hlsl"				,
		"emittedparticle_sphpartitionoffsetsresetCS.hlsl"			,
		"emittedparticle_simulateCS_SORTING.hlsl"					,
		"emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS.hlsl"	,
		"emittedparticle_sphdensityCS.hlsl"							,
		"emittedparticle_sphforceCS.hlsl"							,
		"emittedparticle_kickoffUpdateCS.hlsl"						,
		"emittedparticle_simulateCS_DEPTHCOLLISIONS.hlsl"			,
		"emittedparticle_emitCS.hlsl"								,
		"emittedparticle_emitCS_FROMMESH.hlsl"						,
		"emittedparticle_emitCS_volume.hlsl"						,
		"emittedparticle_finishUpdateCS.hlsl"						,
		"downsample4xCS.hlsl"										,
		"depthoffield_prepassCS_earlyexit.hlsl"						,
		"depthoffield_mainCS_cheap.hlsl"							,
		"depthoffield_mainCS_earlyexit.hlsl"						,
		"depthoffield_postfilterCS.hlsl"							,
		"depthoffield_kickjobsCS.hlsl"								,
		"copytexture2D_float4_borderexpandCS.hlsl"					,
		"copytexture2D_unorm4_borderexpandCS.hlsl"					,
		"copytexture2D_unorm4CS.hlsl"								,
		"copytexture2D_float4CS.hlsl"								,
		"chromatic_aberrationCS.hlsl"								,
		"bvh_hierarchyCS.hlsl"										,
		"bvh_primitivesCS.hlsl"										,
		"bvh_propagateaabbCS.hlsl"									,
		"blur_gaussian_wide_unorm1CS.hlsl"							,
		"blur_gaussian_wide_unorm4CS.hlsl"							,
		"blur_gaussian_unorm1CS.hlsl"								,
		"blur_gaussian_unorm4CS.hlsl"								,
		"blur_gaussian_wide_float1CS.hlsl"							,
		"blur_gaussian_wide_float3CS.hlsl"							,
		"blur_gaussian_wide_float4CS.hlsl"							,
		"blur_bilateral_wide_unorm4CS.hlsl"							,
		"blur_gaussian_float1CS.hlsl"								,
		"blur_gaussian_float3CS.hlsl"								,
		"blur_bilateral_unorm4CS.hlsl"								,
		"blur_bilateral_wide_float1CS.hlsl"							,
		"blur_bilateral_wide_float3CS.hlsl"							,
		"blur_bilateral_wide_float4CS.hlsl"							,
		"blur_bilateral_wide_unorm1CS.hlsl"							,
		"blur_bilateral_float1CS.hlsl"								,
		"blur_bilateral_float3CS.hlsl"								,
		"blur_bilateral_float4CS.hlsl"								,
		"blur_bilateral_unorm1CS.hlsl"								,
		"bloomcombineCS.hlsl"										,
		"voxelSceneCopyClearCS_TemporalSmoothing.hlsl"				,
		"normalsfromdepthCS.hlsl"									,
		"volumetricCloud_shapenoiseCS.hlsl"							,
		"volumetricCloud_detailnoiseCS.hlsl"						,
		"volumetricCloud_curlnoiseCS.hlsl"							,
		"volumetricCloud_weathermapCS.hlsl"							,
		"volumetricCloud_renderCS.hlsl"								,
		"volumetricCloud_reprojectCS.hlsl"							,
		"shadingRateClassificationCS.hlsl"							,
		"shadingRateClassificationCS_DEBUG.hlsl"					,
		"skyAtmosphere_transmittanceLutCS.hlsl"						,
		"skyAtmosphere_skyViewLutCS.hlsl"							,
		"skyAtmosphere_multiScatteredLuminanceLutCS.hlsl"			,
		"skyAtmosphere_skyLuminanceLutCS.hlsl"						,
		"rtao_denoise_blurCS.hlsl"									,
		"rtao_denoise_temporalCS.hlsl"								,
		"rtshadow_denoise_temporalCS.hlsl"							,
		"upsample_bilateral_uint4CS.hlsl"							,
		"screenspaceshadowCS.hlsl"									,
	};

	shaders[wiGraphics::PS] = {
		"emittedparticlePS_soft.hlsl"					,
		"screenPS.hlsl"									,
		"imagePS_separatenormalmap.hlsl"				,
		"imagePS_separatenormalmap_bicubic.hlsl"		,
		"imagePS_backgroundblur_masked.hlsl"			,
		"imagePS_masked.hlsl"							,
		"imagePS_backgroundblur.hlsl"					,
		"imagePS.hlsl"									,
		"emittedparticlePS_soft_lighting.hlsl"			,
		"oceanSurfacePS.hlsl"							,
		"hairparticlePS.hlsl"							,
		"impostorPS.hlsl"								,
		"volumetricLight_SpotPS.hlsl"					,
		"volumetricLight_PointPS.hlsl"					,
		"volumetricLight_DirectionalPS.hlsl"			,
		"voxelPS.hlsl"									,
		"vertexcolorPS.hlsl"							,
		"upsample_bilateralPS.hlsl"						,
		"sunPS.hlsl"									,
		"skyPS_dynamic.hlsl"							,
		"skyPS_static.hlsl"								,
		"skyPS_velocity.hlsl"							,
		"shadowPS_transparent.hlsl"						,
		"shadowPS_water.hlsl"							,
		"shadowPS_alphatest.hlsl"						,
		"renderlightmapPS.hlsl"							,
		"raytrace_debugbvhPS.hlsl"						,
		"outlinePS.hlsl"								,
		"oceanSurfaceSimplePS.hlsl"						,
		"objectPS_transparent_pom.hlsl"					,
		"objectPS_water.hlsl"							,
		"objectPS_voxelizer.hlsl"						,
		"objectPS_voxelizer_terrain.hlsl"				,
		"objectPS_transparent.hlsl"						,
		"objectPS_transparent_planarreflection.hlsl"	,
		"objectPS_planarreflection.hlsl"				,
		"objectPS_pom.hlsl"								,
		"objectPS_terrain.hlsl"							,
		"objectPS.hlsl"									,
		"objectPS_hologram.hlsl"						,
		"objectPS_paintradius.hlsl"						,
		"objectPS_simplest.hlsl"						,
		"objectPS_debug.hlsl"							,
		"objectPS_prepass.hlsl"							,
		"objectPS_prepass_alphatest.hlsl"				,
		"objectPS_anisotropic.hlsl"						,
		"objectPS_transparent_anisotropic.hlsl"			,
		"objectPS_cloth.hlsl"							,
		"objectPS_transparent_cloth.hlsl"				,
		"objectPS_clearcoat.hlsl"						,
		"objectPS_transparent_clearcoat.hlsl"			,
		"objectPS_cloth_clearcoat.hlsl"					,
		"objectPS_transparent_cloth_clearcoat.hlsl"		,
		"objectPS_cartoon.hlsl"							,
		"objectPS_transparent_cartoon.hlsl"				,
		"objectPS_unlit.hlsl"							,
		"objectPS_transparent_unlit.hlsl"				,
		"lightVisualizerPS.hlsl"						,
		"lensFlarePS.hlsl"								,
		"impostorPS_wire.hlsl"							,
		"impostorPS_simple.hlsl"						,
		"impostorPS_prepass.hlsl"						,
		"hairparticlePS_simplest.hlsl"					,
		"hairparticlePS_prepass.hlsl"					,
		"forceFieldVisualizerPS.hlsl"					,
		"fontPS.hlsl"									,
		"envMap_skyPS_static.hlsl"						,
		"envMap_skyPS_dynamic.hlsl"						,
		"envMapPS.hlsl"									,
		"envMapPS_terrain.hlsl"							,
		"emittedparticlePS_soft_distortion.hlsl"		,
		"downsampleDepthBuffer4xPS.hlsl"				,
		"emittedparticlePS_simplest.hlsl"				,
		"cubeMapPS.hlsl"								,
		"circlePS.hlsl"									,
		"captureImpostorPS_normal.hlsl"					,
		"captureImpostorPS_surface.hlsl"				,
		"captureImpostorPS_albedo.hlsl"					,
	};

	shaders[wiGraphics::VS] = {
		"hairparticleVS.hlsl"							,
		"emittedparticleVS.hlsl"						,
		"imageVS.hlsl"									,
		"fontVS.hlsl"									,
		"voxelVS.hlsl"									,
		"vertexcolorVS.hlsl"							,
		"volumetriclight_directionalVS.hlsl"			,
		"volumetriclight_pointVS.hlsl"					,
		"volumetriclight_spotVS.hlsl"					,
		"vSpotLightVS.hlsl"								,
		"vPointLightVS.hlsl"							,
		"sphereVS.hlsl"									,
		"skyVS.hlsl"									,
		"shadowVS_transparent.hlsl"						,
		"shadowVS.hlsl"									,
		"shadowVS_alphatest.hlsl"						,
		"screenVS.hlsl"									,
		"renderlightmapVS.hlsl"							,
		"raytrace_screenVS.hlsl"						,
		"oceanSurfaceVS.hlsl"							,
		"objectVS_simple.hlsl"							,
		"objectVS_voxelizer.hlsl"						,
		"objectVS_common.hlsl"							,
		"objectVS_common_tessellation.hlsl"				,
		"objectVS_prepass.hlsl"							,
		"objectVS_prepass_alphatest.hlsl"				,
		"objectVS_prepass_tessellation.hlsl"			,
		"objectVS_prepass_alphatest_tessellation.hlsl"	,
		"objectVS_debug.hlsl"							,
		"lensFlareVS.hlsl"								,
		"impostorVS.hlsl"								,
		"forceFieldPointVisualizerVS.hlsl"				,
		"forceFieldPlaneVisualizerVS.hlsl"				,
		"envMap_skyVS.hlsl"								,
		"envMapVS.hlsl"									,
		"envMap_skyVS_emulation.hlsl"					,
		"envMapVS_emulation.hlsl"						,
		"cubeShadowVS.hlsl"								,
		"cubeShadowVS_alphatest.hlsl"					,
		"cubeShadowVS_emulation.hlsl"					,
		"cubeShadowVS_alphatest_emulation.hlsl"			,
		"cubeShadowVS_transparent.hlsl"					,
		"cubeShadowVS_transparent_emulation.hlsl"		,
		"cubeVS.hlsl",
	};

	shaders[wiGraphics::GS] = {
		"envMap_skyGS_emulation.hlsl"				,
		"envMapGS_emulation.hlsl"					,
		"cubeShadowGS_emulation.hlsl"				,
		"cubeShadowGS_alphatest_emulation.hlsl"		,
		"cubeShadowGS_transparent_emulation.hlsl"	,
		"voxelGS.hlsl"								,
		"objectGS_voxelizer.hlsl"					,
		"lensFlareGS.hlsl"							,
	};

	shaders[wiGraphics::DS] = {
		"objectDS.hlsl",
		"objectDS_prepass.hlsl",
		"objectDS_prepass_alphatest.hlsl",
	};

	shaders[wiGraphics::HS] = {
		"objectHS.hlsl",
		"objectHS_prepass.hlsl",
		"objectHS_prepass_alphatest.hlsl",
	};

	shaders[wiGraphics::AS] = {

	};

	shaders[wiGraphics::MS] = {
		"emittedparticleMS.hlsl",
	};

	shaders[wiGraphics::LIB] = {
		"rtaoLIB.hlsl",
		"rtreflectionLIB.hlsl",
		"rtshadowLIB.hlsl",
	};

	wiShaderCompiler::Initialize();
	wiJobSystem::Initialize();
	wiJobSystem::context ctx;

	std::string SHADERSOURCEPATH = wiRenderer::GetShaderSourcePath();
	wiHelper::MakePathAbsolute(SHADERSOURCEPATH);

	std::cout << "[Wicked Engine Offline Shader Compiler] Searching for outdated shaders..." << std::endl;
	wiTimer timer;

	for (auto& target : targets)
	{
		std::string SHADERPATH = target.dir;
		wiHelper::DirectoryCreate(SHADERPATH);

		for (int i = 0; i < wiGraphics::SHADERSTAGE_COUNT; ++i)
		{
			if (target.format == wiGraphics::SHADERFORMAT_HLSL5)
			{
				if (
					i == wiGraphics::MS ||
					i == wiGraphics::AS ||
					i == wiGraphics::LIB
					)
				{
					// shader stage not applicable to HLSL5
					continue;
				}
			}

			for (auto& shader : shaders[i])
			{
				wiJobSystem::Execute(ctx, [=](wiJobArgs args) {
					std::string shaderbinaryfilename = wiHelper::ReplaceExtension(SHADERPATH + shader, "cso");
					if (!rebuild && !wiShaderCompiler::IsShaderOutdated(shaderbinaryfilename))
					{
						return;
					}

					wiShaderCompiler::CompilerInput input;
					input.format = target.format;
					input.stage = (wiGraphics::SHADERSTAGE)i;
					input.shadersourcefilename = SHADERSOURCEPATH + shader;
					input.include_directories.push_back(SHADERSOURCEPATH);
					
					wiShaderCompiler::CompilerOutput output;
					wiShaderCompiler::Compile(input, output);

					if (output.IsValid())
					{
						wiShaderCompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

						locker.lock();
						if (!output.error_message.empty())
						{
							std::cerr << output.error_message << std::endl;
						}
						std::cout << "shader compiled: " << shaderbinaryfilename << std::endl;
						if (shaderdump_enabled)
						{
							results[shaderbinaryfilename] = output;
						}
						locker.unlock();
					}
					else
					{
						locker.lock();
						std::cerr << "shader compile FAILED: " << shaderbinaryfilename << std::endl << output.error_message;
						locker.unlock();
						std::exit(1);
					}

				});
			}
		}
	}
	wiJobSystem::Wait(ctx);

	std::cout << "[Wicked Engine Offline Shader Compiler] Finished in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds" << std::endl;

	if (shaderdump_enabled)
	{
		std::cout << "[Wicked Engine Offline Shader Compiler] Creating ShaderDump..." << std::endl;
		timer.record();
		std::stringstream ss;
		ss << "namespace wiShaderDump {" << std::endl;
		for (auto& x : results)
		{
			auto& name = x.first;
			auto& output = x.second;

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			ss << "const uint8_t " << name_repl << "[] = {";
			for (size_t i = 0; i < output.shadersize; ++i)
			{
				ss << (uint32_t)output.shaderdata[i] << ",";
			}
			ss << "};" << std::endl;
		}
		ss << "struct ShaderDumpEntry{const uint8_t* data; size_t size;};" << std::endl;
		ss << "const std::unordered_map<std::string, ShaderDumpEntry> shaderdump = {" << std::endl;
		for (auto& x : results)
		{
			auto& name = x.first;
			auto& output = x.second;

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			ss << "std::pair<std::string, ShaderDumpEntry>(\"" << name << "\", {" << name_repl << ",sizeof(" << name_repl << ")})," << std::endl;
		}
		ss << "};" << std::endl; // map end
		ss << "}" << std::endl; // namespace end
		wiHelper::FileWrite("wiShaderDump.h", (uint8_t*)ss.str().c_str(), ss.str().length());
		std::cout << "[Wicked Engine Offline Shader Compiler] ShaderDump written to wiShaderDump.h in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds" << std::endl;
	}

	return 0;
}
