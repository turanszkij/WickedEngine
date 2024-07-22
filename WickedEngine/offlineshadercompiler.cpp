#include "WickedEngine.h"

#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <cstdlib>

std::mutex locker;
struct ShaderEntry
{
	std::string name;
	wi::graphics::ShaderStage stage = wi::graphics::ShaderStage::Count;
	wi::graphics::ShaderModel minshadermodel = wi::graphics::ShaderModel::SM_5_0;
	struct Permutation
	{
		wi::vector<std::string> defines;
	};
	wi::vector<Permutation> permutations;
};
wi::vector<ShaderEntry> shaders = {
	{"hairparticle_simulateCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS", wi::graphics::ShaderStage::CS},
	{"generateMIPChainCubeCS_float4", wi::graphics::ShaderStage::CS},
	{"generateMIPChainCubeCS_unorm4", wi::graphics::ShaderStage::CS},
	{"generateMIPChainCubeArrayCS_float4", wi::graphics::ShaderStage::CS},
	{"generateMIPChainCubeArrayCS_unorm4", wi::graphics::ShaderStage::CS},
	{"generateMIPChain3DCS_float4", wi::graphics::ShaderStage::CS},
	{"generateMIPChain3DCS_unorm4", wi::graphics::ShaderStage::CS},
	{"generateMIPChain2DCS_float4", wi::graphics::ShaderStage::CS},
	{"generateMIPChain2DCS_unorm4", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC1", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC3", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC4", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC5", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H", wi::graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H_cubemap", wi::graphics::ShaderStage::CS},
	{"blur_gaussian_float4CS", wi::graphics::ShaderStage::CS},
	{"bloomseparateCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_mainCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_neighborhoodMaxCOCCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_prepassCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_upsampleCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_verticalCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_horizontalCS", wi::graphics::ShaderStage::CS},
	{"vxgi_offsetprevCS", wi::graphics::ShaderStage::CS},
	{"vxgi_temporalCS", wi::graphics::ShaderStage::CS},
	{"vxgi_sdf_jumpfloodCS", wi::graphics::ShaderStage::CS},
	{"vxgi_resolve_diffuseCS", wi::graphics::ShaderStage::CS},
	{"vxgi_resolve_specularCS", wi::graphics::ShaderStage::CS},
	{"upsample_bilateral_float1CS", wi::graphics::ShaderStage::CS},
	{"upsample_bilateral_float4CS", wi::graphics::ShaderStage::CS},
	{"upsample_bilateral_unorm1CS", wi::graphics::ShaderStage::CS},
	{"upsample_bilateral_unorm4CS", wi::graphics::ShaderStage::CS},
	{"temporalaaCS", wi::graphics::ShaderStage::CS},
	{"tileFrustumsCS", wi::graphics::ShaderStage::CS},
	{"tonemapCS", wi::graphics::ShaderStage::CS},
	{"underwaterCS", wi::graphics::ShaderStage::CS},
	{"fsr_upscalingCS", wi::graphics::ShaderStage::CS},
	{"fsr_sharpenCS", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_autogen_reactive_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_compute_luminance_pyramid_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_prepare_input_color_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_reconstruct_previous_depth_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_depth_clip_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_lock_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_accumulate_pass", wi::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_rcas_pass", wi::graphics::ShaderStage::CS},
	{"ssaoCS", wi::graphics::ShaderStage::CS},
	{"ssgi_deinterleaveCS", wi::graphics::ShaderStage::CS},
	{"ssgiCS", wi::graphics::ShaderStage::CS},
	{"ssgi_upsampleCS", wi::graphics::ShaderStage::CS},
	{"rtdiffuseCS", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5},
	{"rtdiffuse_spatialCS", wi::graphics::ShaderStage::CS},
	{"rtdiffuse_temporalCS", wi::graphics::ShaderStage::CS},
	{"rtdiffuse_upsampleCS", wi::graphics::ShaderStage::CS},
	{"rtreflectionCS", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5},
	{"ssr_tileMaxRoughness_horizontalCS", wi::graphics::ShaderStage::CS},
	{"ssr_tileMaxRoughness_verticalCS", wi::graphics::ShaderStage::CS},
	{"ssr_depthHierarchyCS", wi::graphics::ShaderStage::CS},
	{"ssr_resolveCS", wi::graphics::ShaderStage::CS},
	{"ssr_temporalCS", wi::graphics::ShaderStage::CS},
	{"ssr_upsampleCS", wi::graphics::ShaderStage::CS},
	{"ssr_raytraceCS", wi::graphics::ShaderStage::CS},
	{"ssr_raytraceCS_cheap", wi::graphics::ShaderStage::CS},
	{"ssr_raytraceCS_earlyexit", wi::graphics::ShaderStage::CS},
	{"sharpenCS", wi::graphics::ShaderStage::CS},
	{"skinningCS", wi::graphics::ShaderStage::CS},
	{"resolveMSAADepthStencilCS", wi::graphics::ShaderStage::CS},
	{"raytraceCS", wi::graphics::ShaderStage::CS},
	{"raytraceCS_rtapi", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5},
	{"paint_textureCS", wi::graphics::ShaderStage::CS},
	{"oceanUpdateDisplacementMapCS", wi::graphics::ShaderStage::CS},
	{"oceanUpdateGradientFoldingCS", wi::graphics::ShaderStage::CS},
	{"oceanSimulatorCS", wi::graphics::ShaderStage::CS},
	{"msao_interleaveCS", wi::graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers1CS", wi::graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers2CS", wi::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS", wi::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_blendout", wi::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin", wi::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin_blendout", wi::graphics::ShaderStage::CS},
	{"msaoCS", wi::graphics::ShaderStage::CS},
	{"motionblur_neighborhoodMaxVelocityCS", wi::graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_horizontalCS", wi::graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_verticalCS", wi::graphics::ShaderStage::CS},
	{"luminancePass2CS", wi::graphics::ShaderStage::CS},
	{"motionblurCS", wi::graphics::ShaderStage::CS},
	{"motionblurCS_cheap", wi::graphics::ShaderStage::CS},
	{"motionblurCS_earlyexit", wi::graphics::ShaderStage::CS},
	{"luminancePass1CS", wi::graphics::ShaderStage::CS},
	{"lightShaftsCS", wi::graphics::ShaderStage::CS},
	{"lightCullingCS_ADVANCED_DEBUG", wi::graphics::ShaderStage::CS},
	{"lightCullingCS_DEBUG", wi::graphics::ShaderStage::CS},
	{"lightCullingCS", wi::graphics::ShaderStage::CS},
	{"lightCullingCS_ADVANCED", wi::graphics::ShaderStage::CS},
	{"hbaoCS", wi::graphics::ShaderStage::CS},
	{"gpusortlib_sortInnerCS", wi::graphics::ShaderStage::CS},
	{"gpusortlib_sortStepCS", wi::graphics::ShaderStage::CS},
	{"gpusortlib_kickoffSortCS", wi::graphics::ShaderStage::CS},
	{"gpusortlib_sortCS", wi::graphics::ShaderStage::CS},
	{"fxaaCS", wi::graphics::ShaderStage::CS},
	{"filterEnvMapCS", wi::graphics::ShaderStage::CS},
	{"fft_512x512_c2c_CS", wi::graphics::ShaderStage::CS},
	{"fft_512x512_c2c_v2_CS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_sphpartitionCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_sphcellallocationCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_sphbinningCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING", wi::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_sphdensityCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_sphforceCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_kickoffUpdateCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_DEPTHCOLLISIONS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS", wi::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_FROMMESH", wi::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_volume", wi::graphics::ShaderStage::CS},
	{"emittedparticle_finishUpdateCS", wi::graphics::ShaderStage::CS},
	{"downsample4xCS", wi::graphics::ShaderStage::CS},
	{"lineardepthCS", wi::graphics::ShaderStage::CS},
	{"depthoffield_prepassCS_earlyexit", wi::graphics::ShaderStage::CS},
	{"depthoffield_mainCS_cheap", wi::graphics::ShaderStage::CS},
	{"depthoffield_mainCS_earlyexit", wi::graphics::ShaderStage::CS },
	{"depthoffield_postfilterCS", wi::graphics::ShaderStage::CS },
	{"copytexture2D_float4_borderexpandCS", wi::graphics::ShaderStage::CS },
	{"copytexture2D_unorm4_borderexpandCS", wi::graphics::ShaderStage::CS },
	{"copytexture2D_unorm4CS", wi::graphics::ShaderStage::CS },
	{"copytexture2D_float4CS", wi::graphics::ShaderStage::CS },
	{"chromatic_aberrationCS", wi::graphics::ShaderStage::CS },
	{"bvh_hierarchyCS", wi::graphics::ShaderStage::CS },
	{"bvh_primitivesCS", wi::graphics::ShaderStage::CS },
	{"bvh_propagateaabbCS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_unorm1CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_unorm4CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_unorm1CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_unorm4CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float1CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float3CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float4CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_unorm4CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_float1CS", wi::graphics::ShaderStage::CS },
	{"blur_gaussian_float3CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_unorm4CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float1CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float3CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float4CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_unorm1CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_float1CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_float3CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_float4CS", wi::graphics::ShaderStage::CS },
	{"blur_bilateral_unorm1CS", wi::graphics::ShaderStage::CS },
	{"normalsfromdepthCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_curlnoiseCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_detailnoiseCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS_capture", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS_capture_MSAA", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_reprojectCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_shadow_filterCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_shadow_renderCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_shapenoiseCS", wi::graphics::ShaderStage::CS },
	{"volumetricCloud_upsamplePS", wi::graphics::ShaderStage::PS },
	{"volumetricCloud_weathermapCS", wi::graphics::ShaderStage::CS },
	{"shadingRateClassificationCS", wi::graphics::ShaderStage::CS },
	{"shadingRateClassificationCS_DEBUG", wi::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS", wi::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS_capture", wi::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS_capture_MSAA", wi::graphics::ShaderStage::CS },
	{"skyAtmosphere_cameraVolumeLutCS", wi::graphics::ShaderStage::CS },
	{"skyAtmosphere_transmittanceLutCS", wi::graphics::ShaderStage::CS },
	{"skyAtmosphere_skyViewLutCS", wi::graphics::ShaderStage::CS },
	{"skyAtmosphere_multiScatteredLuminanceLutCS", wi::graphics::ShaderStage::CS },
	{"skyAtmosphere_skyLuminanceLutCS", wi::graphics::ShaderStage::CS },
	{"screenspaceshadowCS", wi::graphics::ShaderStage::CS },
	{"rtshadowCS", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5 },
	{"rtshadow_denoise_tileclassificationCS", wi::graphics::ShaderStage::CS },
	{"rtshadow_denoise_filterCS", wi::graphics::ShaderStage::CS },
	{"rtshadow_denoise_temporalCS", wi::graphics::ShaderStage::CS },
	{"rtshadow_upsampleCS", wi::graphics::ShaderStage::CS },
	{"rtaoCS", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5 },
	{"rtao_denoise_tileclassificationCS", wi::graphics::ShaderStage::CS },
	{"rtao_denoise_filterCS", wi::graphics::ShaderStage::CS },
	{"visibility_resolveCS", wi::graphics::ShaderStage::CS },
	{"visibility_resolveCS_MSAA", wi::graphics::ShaderStage::CS },
	{"visibility_velocityCS", wi::graphics::ShaderStage::CS },
	{"visibility_skyCS", wi::graphics::ShaderStage::CS },
	{"surfel_coverageCS", wi::graphics::ShaderStage::CS },
	{"surfel_indirectprepareCS", wi::graphics::ShaderStage::CS },
	{"surfel_updateCS", wi::graphics::ShaderStage::CS },
	{"surfel_gridoffsetsCS", wi::graphics::ShaderStage::CS },
	{"surfel_binningCS", wi::graphics::ShaderStage::CS },
	{"surfel_raytraceCS_rtapi", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5 },
	{"surfel_raytraceCS", wi::graphics::ShaderStage::CS },
	{"surfel_integrateCS", wi::graphics::ShaderStage::CS },
	{"ddgi_rayallocationCS", wi::graphics::ShaderStage::CS },
	{"ddgi_indirectprepareCS", wi::graphics::ShaderStage::CS },
	{"ddgi_raytraceCS", wi::graphics::ShaderStage::CS },
	{"ddgi_raytraceCS_rtapi", wi::graphics::ShaderStage::CS, wi::graphics::ShaderModel::SM_6_5 },
	{"ddgi_updateCS", wi::graphics::ShaderStage::CS },
	{"ddgi_updateCS_depth", wi::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS", wi::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_normalmap", wi::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_surfacemap", wi::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_emissivemap", wi::graphics::ShaderStage::CS },
	{"meshlet_prepareCS", wi::graphics::ShaderStage::CS },
	{"impostor_prepareCS", wi::graphics::ShaderStage::CS },
	{"virtualTextureTileRequestsCS", wi::graphics::ShaderStage::CS },
	{"virtualTextureTileAllocateCS", wi::graphics::ShaderStage::CS },
	{"virtualTextureResidencyUpdateCS", wi::graphics::ShaderStage::CS },
	{"windCS", wi::graphics::ShaderStage::CS },
	{"yuv_to_rgbCS", wi::graphics::ShaderStage::CS },
	{"wetmap_updateCS", wi::graphics::ShaderStage::CS },
	{"causticsCS", wi::graphics::ShaderStage::CS },


	{"emittedparticlePS_soft", wi::graphics::ShaderStage::PS },
	{"imagePS", wi::graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_lighting", wi::graphics::ShaderStage::PS },
	{"oceanSurfacePS", wi::graphics::ShaderStage::PS },
	{"hairparticlePS", wi::graphics::ShaderStage::PS },
	{"hairparticlePS_simple", wi::graphics::ShaderStage::PS },
	{"hairparticlePS_prepass", wi::graphics::ShaderStage::PS },
	{"hairparticlePS_prepass_depthonly", wi::graphics::ShaderStage::PS },
	{"hairparticlePS_shadow", wi::graphics::ShaderStage::PS },
	{"volumetricLight_SpotPS", wi::graphics::ShaderStage::PS },
	{"volumetricLight_PointPS", wi::graphics::ShaderStage::PS },
	{"volumetricLight_DirectionalPS", wi::graphics::ShaderStage::PS },
	{"voxelPS", wi::graphics::ShaderStage::PS },
	{"vertexcolorPS", wi::graphics::ShaderStage::PS },
	{"upsample_bilateralPS", wi::graphics::ShaderStage::PS },
	{"sunPS", wi::graphics::ShaderStage::PS },
	{"skyPS_dynamic", wi::graphics::ShaderStage::PS },
	{"skyPS_static", wi::graphics::ShaderStage::PS },
	{"shadowPS_transparent", wi::graphics::ShaderStage::PS },
	{"shadowPS_water", wi::graphics::ShaderStage::PS },
	{"shadowPS_alphatest", wi::graphics::ShaderStage::PS },
	{"renderlightmapPS", wi::graphics::ShaderStage::PS },
	{"renderlightmapPS_rtapi", wi::graphics::ShaderStage::PS, wi::graphics::ShaderModel::SM_6_5 },
	{"raytrace_debugbvhPS", wi::graphics::ShaderStage::PS },
	{"outlinePS", wi::graphics::ShaderStage::PS },
	{"oceanSurfaceSimplePS", wi::graphics::ShaderStage::PS },
	{"objectPS_voxelizer", wi::graphics::ShaderStage::PS },
	{"objectPS_hologram", wi::graphics::ShaderStage::PS },
	{"objectPS_paintradius", wi::graphics::ShaderStage::PS },
	{"objectPS_simple", wi::graphics::ShaderStage::PS },
	{"objectPS_debug", wi::graphics::ShaderStage::PS },
	{"objectPS_prepass", wi::graphics::ShaderStage::PS },
	{"objectPS_prepass_alphatest", wi::graphics::ShaderStage::PS },
	{"objectPS_prepass_depthonly", wi::graphics::ShaderStage::PS },
	{"objectPS_prepass_depthonly_alphatest", wi::graphics::ShaderStage::PS },
	{"lightVisualizerPS", wi::graphics::ShaderStage::PS },
	{"lensFlarePS", wi::graphics::ShaderStage::PS },
	{"impostorPS", wi::graphics::ShaderStage::PS },
	{"impostorPS_simple", wi::graphics::ShaderStage::PS },
	{"impostorPS_prepass", wi::graphics::ShaderStage::PS },
	{"impostorPS_prepass_depthonly", wi::graphics::ShaderStage::PS },
	{"forceFieldVisualizerPS", wi::graphics::ShaderStage::PS },
	{"fontPS", wi::graphics::ShaderStage::PS },
	{"envMap_skyPS_static", wi::graphics::ShaderStage::PS },
	{"envMap_skyPS_dynamic", wi::graphics::ShaderStage::PS },
	{"envMapPS", wi::graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_distortion", wi::graphics::ShaderStage::PS },
	{"downsampleDepthBuffer4xPS", wi::graphics::ShaderStage::PS },
	{"emittedparticlePS_simple", wi::graphics::ShaderStage::PS },
	{"cubeMapPS", wi::graphics::ShaderStage::PS },
	{"circlePS", wi::graphics::ShaderStage::PS },
	{"captureImpostorPS", wi::graphics::ShaderStage::PS },
	{"ddgi_debugPS", wi::graphics::ShaderStage::PS },
	{"copyDepthPS", wi::graphics::ShaderStage::PS },
	{"copyStencilBitPS", wi::graphics::ShaderStage::PS },
	{"trailPS", wi::graphics::ShaderStage::PS },


	{"hairparticleVS", wi::graphics::ShaderStage::VS },
	{"emittedparticleVS", wi::graphics::ShaderStage::VS },
	{"imageVS", wi::graphics::ShaderStage::VS },
	{"fontVS", wi::graphics::ShaderStage::VS },
	{"voxelVS", wi::graphics::ShaderStage::VS },
	{"vertexcolorVS", wi::graphics::ShaderStage::VS },
	{"volumetriclight_directionalVS", wi::graphics::ShaderStage::VS },
	{"volumetriclight_pointVS", wi::graphics::ShaderStage::VS },
	{"volumetriclight_spotVS", wi::graphics::ShaderStage::VS },
	{"vSpotLightVS", wi::graphics::ShaderStage::VS },
	{"vPointLightVS", wi::graphics::ShaderStage::VS },
	{"sphereVS", wi::graphics::ShaderStage::VS },
	{"skyVS", wi::graphics::ShaderStage::VS },
	{"postprocessVS", wi::graphics::ShaderStage::VS },
	{"renderlightmapVS", wi::graphics::ShaderStage::VS },
	{"raytrace_screenVS", wi::graphics::ShaderStage::VS },
	{"oceanSurfaceVS", wi::graphics::ShaderStage::VS },
	{"objectVS_debug", wi::graphics::ShaderStage::VS },
	{"objectVS_voxelizer", wi::graphics::ShaderStage::VS },
	{"lensFlareVS", wi::graphics::ShaderStage::VS },
	{"impostorVS", wi::graphics::ShaderStage::VS },
	{"forceFieldPointVisualizerVS", wi::graphics::ShaderStage::VS },
	{"forceFieldPlaneVisualizerVS", wi::graphics::ShaderStage::VS },
	{"envMap_skyVS", wi::graphics::ShaderStage::VS },
	{"envMapVS", wi::graphics::ShaderStage::VS },
	{"envMap_skyVS_emulation", wi::graphics::ShaderStage::VS },
	{"envMapVS_emulation", wi::graphics::ShaderStage::VS },
	{"occludeeVS", wi::graphics::ShaderStage::VS },
	{"ddgi_debugVS", wi::graphics::ShaderStage::VS },
	{"envMap_skyGS_emulation", wi::graphics::ShaderStage::GS },
	{"envMapGS_emulation", wi::graphics::ShaderStage::GS },
	{"shadowGS_emulation", wi::graphics::ShaderStage::GS },
	{"shadowGS_alphatest_emulation", wi::graphics::ShaderStage::GS },
	{"shadowGS_transparent_emulation", wi::graphics::ShaderStage::GS },
	{"objectGS_primitiveID_emulation", wi::graphics::ShaderStage::GS },
	{"objectGS_primitiveID_emulation_alphatest", wi::graphics::ShaderStage::GS },
	{"voxelGS", wi::graphics::ShaderStage::GS },
	{"objectGS_voxelizer", wi::graphics::ShaderStage::GS },
	{"objectVS_simple", wi::graphics::ShaderStage::VS },
	{"objectVS_common", wi::graphics::ShaderStage::VS },
	{"objectVS_common_tessellation", wi::graphics::ShaderStage::VS },
	{"objectVS_prepass", wi::graphics::ShaderStage::VS },
	{"objectVS_prepass_alphatest", wi::graphics::ShaderStage::VS },
	{"objectVS_prepass_tessellation", wi::graphics::ShaderStage::VS },
	{"objectVS_prepass_alphatest_tessellation", wi::graphics::ShaderStage::VS },
	{"objectVS_simple_tessellation", wi::graphics::ShaderStage::VS },
	{"shadowVS", wi::graphics::ShaderStage::VS },
	{"shadowVS_alphatest", wi::graphics::ShaderStage::VS },
	{"shadowVS_emulation", wi::graphics::ShaderStage::VS },
	{"shadowVS_alphatest_emulation", wi::graphics::ShaderStage::VS },
	{"shadowVS_transparent", wi::graphics::ShaderStage::VS },
	{"shadowVS_transparent_emulation", wi::graphics::ShaderStage::VS },
	{"screenVS", wi::graphics::ShaderStage::VS },
	{"trailVS", wi::graphics::ShaderStage::VS },



	{"objectDS", wi::graphics::ShaderStage::DS },
	{"objectDS_prepass", wi::graphics::ShaderStage::DS },
	{"objectDS_prepass_alphatest", wi::graphics::ShaderStage::DS },
	{"objectDS_simple", wi::graphics::ShaderStage::DS },


	{"objectHS", wi::graphics::ShaderStage::HS },
	{"objectHS_prepass", wi::graphics::ShaderStage::HS },
	{"objectHS_prepass_alphatest", wi::graphics::ShaderStage::HS },
	{"objectHS_simple", wi::graphics::ShaderStage::HS },

	{"emittedparticleMS", wi::graphics::ShaderStage::MS },


	//{"rtreflectionLIB", wi::graphics::ShaderStage::LIB },
};

struct Target
{
	wi::graphics::ShaderFormat format;
	std::string dir;
};
wi::vector<Target> targets;
wi::unordered_map<std::string, wi::shadercompiler::CompilerOutput> results;
bool rebuild = false;
bool shaderdump_enabled = false;

using namespace wi::graphics;

int main(int argc, char* argv[])
{
	wi::shadercompiler::Flags compile_flags = wi::shadercompiler::Flags::NONE;
	std::cout << "[Wicked Engine Offline Shader Compiler]\n";
	std::cout << "Available command arguments:\n";
	std::cout << "\thlsl5 : \t\tCompile shaders to hlsl5 (dx11) format (using d3dcompiler)\n";
	std::cout << "\thlsl6 : \t\tCompile shaders to hlsl6 (dx12) format (using dxcompiler)\n";
	std::cout << "\tspirv : \t\tCompile shaders to spirv (vulkan) format (using dxcompiler)\n";
	std::cout << "\thlsl6_xs : \t\tCompile shaders to hlsl6 Xbox Series native (dx12) format (requires Xbox SDK)\n";
	std::cout << "\tps5 : \t\t\tCompile shaders to PlayStation 5 native format (requires PlayStation 5 SDK)\n";
	std::cout << "\trebuild : \t\tAll shaders will be rebuilt, regardless if they are outdated or not\n";
	std::cout << "\tdisable_optimization : \tShaders will be compiled without optimizations\n";
	std::cout << "\tstrip_reflection : \tReflection will be stripped from shader binary to reduce file size\n";
	std::cout << "\tshaderdump : \t\tShaders will be saved to wiShaderDump.h C++ header file (rebuild is assumed)\n";
	std::cout << "Command arguments used: ";

	wi::arguments::Parse(argc, argv);

	if (wi::arguments::HasArgument("hlsl5"))
	{
		targets.push_back({ ShaderFormat::HLSL5, "shaders/hlsl5/" });
		std::cout << "hlsl5 ";
	}
	if (wi::arguments::HasArgument("hlsl6"))
	{
		targets.push_back({ ShaderFormat::HLSL6, "shaders/hlsl6/" });
		std::cout << "hlsl6 ";
	}
	if (wi::arguments::HasArgument("spirv"))
	{
		targets.push_back({ ShaderFormat::SPIRV, "shaders/spirv/" });
		std::cout << "spirv ";
	}
	if (wi::arguments::HasArgument("hlsl6_xs"))
	{
		targets.push_back({ ShaderFormat::HLSL6_XS, "shaders/hlsl6_xs/" });
		std::cout << "hlsl6_xs ";
	}
	if (wi::arguments::HasArgument("ps5"))
	{
		targets.push_back({ ShaderFormat::PS5, "shaders/ps5/" });
		std::cout << "ps5 ";
	}

	if (wi::arguments::HasArgument("shaderdump"))
	{
		shaderdump_enabled = true;
		rebuild = true;
		std::cout << "shaderdump ";
	}

	if (wi::arguments::HasArgument("rebuild"))
	{
		rebuild = true;
		std::cout << "rebuild ";
	}

	if (wi::arguments::HasArgument("disable_optimization"))
	{
		compile_flags |= wi::shadercompiler::Flags::DISABLE_OPTIMIZATION;
		std::cout << "disable_optimization ";
	}

	if (wi::arguments::HasArgument("strip_reflection"))
	{
		compile_flags |= wi::shadercompiler::Flags::STRIP_REFLECTION;
		std::cout << "strip_reflection ";
	}

	std::cout << "\n";

	if (targets.empty())
	{
		targets = {
			//{ ShaderFormat::HLSL5, "shaders/hlsl5/" },
			{ ShaderFormat::HLSL6, "shaders/hlsl6/" },
			{ ShaderFormat::SPIRV, "shaders/spirv/" },
		};
		std::cout << "No shader formats were specified, assuming command arguments: spirv hlsl6\n";
	}

	// permutations for objectPS:
	shaders.push_back({ "objectPS", wi::graphics::ShaderStage::PS });
	for (auto& x : wi::scene::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;

		// same but with TRANSPARENT:
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("TRANSPARENT");
	}

	// permutations for visibility_surfaceCS:
	shaders.push_back({ "visibility_surfaceCS", wi::graphics::ShaderStage::CS });
	for (auto& x : wi::scene::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
	}

	// permutations for visibility_surfaceCS REDUCED:
	shaders.push_back({ "visibility_surfaceCS", wi::graphics::ShaderStage::CS });
	for (auto& x : wi::scene::MaterialComponent::shaderTypeDefines)
	{
		auto defines = x;
		defines.push_back("REDUCED");
		shaders.back().permutations.emplace_back().defines = defines;
	}

	// permutations for visibility_shadeCS:
	shaders.push_back({ "visibility_shadeCS", wi::graphics::ShaderStage::CS });
	for (auto& x : wi::scene::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
	}

	// permutations for ssgiCS:
	shaders.push_back({ "ssgiCS", wi::graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "WIDE" };
	// permutations for ssgi_upsampleCS:
	shaders.push_back({ "ssgi_upsampleCS", wi::graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "WIDE" };

	wi::jobsystem::Initialize();
	wi::jobsystem::context ctx;

	std::string SHADERSOURCEPATH = wi::renderer::GetShaderSourcePath();
	wi::helper::MakePathAbsolute(SHADERSOURCEPATH);

	std::cout << "[Wicked Engine Offline Shader Compiler] Searching for outdated shaders...\n";
	wi::Timer timer;
	static int errors = 0;

	for (auto& target : targets)
	{
		std::string SHADERPATH = target.dir;
		wi::helper::DirectoryCreate(SHADERPATH);

		for (auto& shader : shaders)
		{
			if (target.format == ShaderFormat::HLSL5)
			{
				if (
					shader.stage == ShaderStage::MS ||
					shader.stage == ShaderStage::AS ||
					shader.stage == ShaderStage::LIB
					)
				{
					// shader stage not applicable to HLSL5
					continue;
				}
			}
			wi::vector<ShaderEntry::Permutation> permutations = shader.permutations;
			if (permutations.empty())
			{
				permutations.emplace_back();
			}

			for (auto permutation : permutations)
			{
				wi::jobsystem::Execute(ctx, [=](wi::jobsystem::JobArgs args) {
					std::string shaderbinaryfilename = SHADERPATH + shader.name;
					for (auto& def : permutation.defines)
					{
						shaderbinaryfilename += "_" + def;
					}
					shaderbinaryfilename += ".cso";
					if (!rebuild && !wi::shadercompiler::IsShaderOutdated(shaderbinaryfilename))
					{
						return;
					}

					wi::shadercompiler::CompilerInput input;
					input.flags = compile_flags;
					input.format = target.format;
					input.stage = shader.stage;
					input.shadersourcefilename = SHADERSOURCEPATH + shader.name + ".hlsl";
					input.include_directories.push_back(SHADERSOURCEPATH);
					input.include_directories.push_back(SHADERSOURCEPATH + wi::helper::GetDirectoryFromPath(shader.name));
					input.minshadermodel = shader.minshadermodel;
					input.defines = permutation.defines;

					if (input.minshadermodel > ShaderModel::SM_5_0 && target.format == ShaderFormat::HLSL5)
					{
						// if shader format cannot support shader model, then we cancel the task without returning error
						return;
					}
					if (target.format == ShaderFormat::PS5 && (input.minshadermodel >= ShaderModel::SM_6_5 || input.stage == ShaderStage::MS))
					{
						// TODO PS5 raytracing, mesh shader
						return;
					}

					wi::shadercompiler::CompilerOutput output;
					wi::shadercompiler::Compile(input, output);

					if (output.IsValid())
					{
						wi::shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

						locker.lock();
						if (!output.error_message.empty())
						{
							std::cerr << output.error_message << "\n";
						}
						std::cout << "shader compiled: " << shaderbinaryfilename << "\n";
						if (shaderdump_enabled)
						{
							results[shaderbinaryfilename] = output;
						}
						locker.unlock();
					}
					else
					{
						locker.lock();
						std::cerr << "shader compile FAILED: " << shaderbinaryfilename << "\n" << output.error_message;
						errors++;
						locker.unlock();
					}

				});
			}
		}
	}
	wi::jobsystem::Wait(ctx);

	std::cout << "[Wicked Engine Offline Shader Compiler] Finished in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds with " << errors << " errors\n";

	if (shaderdump_enabled)
	{
		std::cout << "[Wicked Engine Offline Shader Compiler] Creating ShaderDump...\n";
		timer.record();
		std::string ss;
		ss += "namespace wiShaderDump {\n";
		for (auto& x : results)
		{
			auto& name = x.first;
			auto& output = x.second;

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			std::replace(name_repl.begin(), name_repl.end(), '-', '_');
			ss += "const uint8_t " + name_repl + "[] = {";
			for (size_t i = 0; i < output.shadersize; ++i)
			{
				ss += std::to_string((uint32_t)output.shaderdata[i]) + ",";
			}
			ss += "};\n";
		}
		ss += "struct ShaderDumpEntry{const uint8_t* data; size_t size;};\n";
		ss += "const wi::unordered_map<std::string, ShaderDumpEntry> shaderdump = {\n";
		for (auto& x : results)
		{
			auto& name = x.first;
			auto& output = x.second;

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			std::replace(name_repl.begin(), name_repl.end(), '-', '_');
			ss += "{\"" + name + "\", {" + name_repl + ",sizeof(" + name_repl + ")}},\n";
		}
		ss += "};\n"; // map end
		ss += "}\n"; // namespace end
		wi::helper::FileWrite("wiShaderDump.h", (uint8_t*)ss.c_str(), ss.length());
		std::cout << "[Wicked Engine Offline Shader Compiler] ShaderDump written to wiShaderDump.h in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds\n";
	}

	wi::jobsystem::ShutDown();

	return errors;
}
