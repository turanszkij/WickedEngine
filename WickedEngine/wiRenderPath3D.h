#pragma once
#include "wiRenderPath2D.h"
#include "wiRenderer.h"
#include "wiGraphicsDevice.h"
#include "wiResourceManager.h"
#include "wiScene.h"

namespace wi
{

	class RenderPath3D :
		public RenderPath2D
	{
	public:
		enum AO
		{
			AO_DISABLED,	// no ambient occlusion
			AO_SSAO,		// simple brute force screen space ambient occlusion
			AO_HBAO,		// horizon based screen space ambient occlusion
			AO_MSAO,		// multi scale screen space ambient occlusion
			AO_RTAO,		// ray traced ambient occlusion
			// Don't alter order! (bound to lua manually)
		};
		enum class FSR2_Preset
		{
			// Guidelines: https://github.com/GPUOpen-Effects/FidelityFX-FSR2#scaling-modes
			Quality,
			Balanced,
			Performance,
			Ultra_Performance,
		};
	private:
		float exposure = 1.0f;
		float brightness = 0.0f;
		float contrast = 1.0f;
		float saturation = 1.0f;
		float bloomThreshold = 1.0f;
		float motionBlurStrength = 100.0f;
		float dofStrength = 10.0f;
		float sharpenFilterAmount = 0.28f;
		float outlineThreshold = 0.2f;
		float outlineThickness = 1.0f;
		XMFLOAT4 outlineColor = XMFLOAT4(0, 0, 0, 1);
		float aoRange = 1.0f;
		uint32_t aoSampleCount = 16;
		float aoPower = 1.0f;
		float chromaticAberrationAmount = 2.0f;
		uint32_t screenSpaceShadowSampleCount = 16;
		float screenSpaceShadowRange = 1;
		float eyeadaptionKey = 0.115f;
		float eyeadaptionRate = 1;
		float fsrSharpness = 1.0f;
		float fsr2Sharpness = 0.5f;
		float lightShaftsStrength = 0.2f;
		float raytracedDiffuseRange = 10;
		float raytracedReflectionsRange = 10000.0f;
		float reflectionRoughnessCutoff = 0.6f;
		wi::renderer::Tonemap tonemap = wi::renderer::Tonemap::ACES;

		AO ao = AO_DISABLED;
		bool fxaaEnabled = false;
		bool ssrEnabled = false;
		bool raytracedReflectionsEnabled = false;
		bool raytracedDiffuseEnabled = false;
		bool reflectionsEnabled = true;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool colorGradingEnabled = true;
		bool volumeLightsEnabled = true;
		bool lightShaftsEnabled = false;
		bool lensFlareEnabled = true;
		bool motionBlurEnabled = false;
		bool depthOfFieldEnabled = true;
		bool eyeAdaptionEnabled = false;
		bool sharpenFilterEnabled = false;
		bool outlineEnabled = false;
		bool chromaticAberrationEnabled = false;
		bool ditherEnabled = true;
		bool occlusionCullingEnabled = true;
		bool sceneUpdateEnabled = true;
		bool fsrEnabled = false;
		bool fsr2Enabled = false;
		bool vxgiResolveFullResolution = false;

		uint32_t msaaSampleCount = 1;

	public:
		wi::graphics::Texture rtMain;
		wi::graphics::Texture rtMain_render; // can be MSAA
		wi::graphics::Texture rtPrimitiveID;
		wi::graphics::Texture rtPrimitiveID_render; // can be MSAA
		wi::graphics::Texture rtVelocity; // optional R16G16_FLOAT
		wi::graphics::Texture rtReflection; // contains the scene rendered for planar reflections
		wi::graphics::Texture rtRaytracedDiffuse; // raytraced diffuse screen space texture
		wi::graphics::Texture rtSSR; // standard screen-space reflection results
		wi::graphics::Texture rtSceneCopy; // contains the rendered scene that can be fed into transparent pass for distortion effect
		wi::graphics::Texture rtSceneCopy_tmp; // temporary for gaussian mipchain
		wi::graphics::Texture rtWaterRipple; // water ripple sprite normal maps are rendered into this
		wi::graphics::Texture rtParticleDistortion; // contains distortive particles
		wi::graphics::Texture rtParticleDistortion_Resolved; // contains distortive particles
		wi::graphics::Texture rtVolumetricLights[2]; // contains the volumetric light results
		wi::graphics::Texture rtBloom; // contains the bright parts of the image + mipchain
		wi::graphics::Texture rtBloom_tmp; // temporary for bloom downsampling
		wi::graphics::Texture rtAO; // full res AO
		wi::graphics::Texture rtShadow; // raytraced shadows mask
		wi::graphics::Texture rtSun[2]; // 0: sun render target used for lightshafts (can be MSAA), 1: radial blurred lightshafts
		wi::graphics::Texture rtSun_resolved; // sun render target, but the resolved version if MSAA is enabled
		wi::graphics::Texture rtGUIBlurredBackground[3];	// downsampled, gaussian blurred scene for GUI
		wi::graphics::Texture rtShadingRate; // UINT8 shading rate per tile
		wi::graphics::Texture rtFSR[2]; // FSR upscaling result (full resolution LDR)
		wi::graphics::Texture rtOutlineSource; // linear depth but only the regions which have outline stencil

		wi::graphics::Texture rtPostprocess; // ping-pong with main scene RT in post-process chain

		wi::graphics::Texture depthBuffer_Main; // used for depth-testing, can be MSAA
		wi::graphics::Texture depthBuffer_Copy; // used for shader resource, single sample
		wi::graphics::Texture depthBuffer_Copy1; // used for disocclusion check
		wi::graphics::Texture depthBuffer_Reflection; // used for reflection, single sample
		wi::graphics::Texture rtLinearDepth; // linear depth result + mipchain (max filter)

		wi::graphics::Texture debugUAV; // debug UAV can be used by some shaders...
		wi::renderer::TiledLightResources tiledLightResources;
		wi::renderer::TiledLightResources tiledLightResources_planarReflection;
		wi::renderer::LuminanceResources luminanceResources;
		wi::renderer::SSAOResources ssaoResources;
		wi::renderer::MSAOResources msaoResources;
		wi::renderer::RTAOResources rtaoResources;
		wi::renderer::RTDiffuseResources rtdiffuseResources;
		wi::renderer::RTReflectionResources rtreflectionResources;
		wi::renderer::SSRResources ssrResources;
		wi::renderer::RTShadowResources rtshadowResources;
		wi::renderer::ScreenSpaceShadowResources screenspaceshadowResources;
		wi::renderer::DepthOfFieldResources depthoffieldResources;
		wi::renderer::MotionBlurResources motionblurResources;
		wi::renderer::AerialPerspectiveResources aerialperspectiveResources;
		wi::renderer::AerialPerspectiveResources aerialperspectiveResources_reflection;
		wi::renderer::VolumetricCloudResources volumetriccloudResources;
		wi::renderer::VolumetricCloudResources volumetriccloudResources_reflection;
		wi::renderer::BloomResources bloomResources;
		wi::renderer::SurfelGIResources surfelGIResources;
		wi::renderer::TemporalAAResources temporalAAResources;
		wi::renderer::VisibilityResources visibilityResources;
		wi::renderer::FSR2Resources fsr2Resources;
		wi::renderer::VXGIResources vxgiResources;

		wi::graphics::CommandList video_cmd;
		wi::vector<wi::video::VideoInstance*> video_instances_tmp;

		mutable const wi::graphics::Texture* lastPostprocessRT = &rtPostprocess;
		// Post-processes are ping-ponged, this function helps to obtain the last postprocess render target that was written
		const wi::graphics::Texture* GetLastPostprocessRT() const
		{
			return lastPostprocessRT;
		}

		virtual void RenderAO(wi::graphics::CommandList cmd) const;
		virtual void RenderSSR(wi::graphics::CommandList cmd) const;
		virtual void RenderOutline(wi::graphics::CommandList cmd) const;
		virtual void RenderLightShafts(wi::graphics::CommandList cmd) const;
		virtual void RenderVolumetrics(wi::graphics::CommandList cmd) const;
		virtual void RenderSceneMIPChain(wi::graphics::CommandList cmd) const;
		virtual void RenderTransparents(wi::graphics::CommandList cmd) const;
		virtual void RenderPostprocessChain(wi::graphics::CommandList cmd) const;

		void DeleteGPUResources();
		void ResizeBuffers() override;

		wi::scene::CameraComponent* camera = &wi::scene::GetCamera();
		wi::scene::CameraComponent camera_previous;
		wi::scene::CameraComponent camera_reflection;
		wi::scene::CameraComponent camera_reflection_previous;

		wi::scene::Scene* scene = &wi::scene::GetScene();
		wi::renderer::Visibility visibility_main;
		wi::renderer::Visibility visibility_reflection;

		FrameCB frameCB = {};

		bool visibility_shading_in_compute = false;

		// Crop parameters in logical coordinates:
		float crop_left = 0;
		float crop_top = 0;
		float crop_right = 0;
		float crop_bottom = 0;
		wi::graphics::Rect GetScissorNativeResolution() const
		{
			wi::graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left));
			scissor.top = int(LogicalToPhysical(crop_top));
			scissor.right = int(GetPhysicalWidth() - LogicalToPhysical(crop_right));
			scissor.bottom = int(GetPhysicalHeight() - LogicalToPhysical(crop_bottom));
			return scissor;
		}
		wi::graphics::Rect GetScissorInternalResolution() const
		{
			wi::graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left) * resolutionScale);
			scissor.top = int(LogicalToPhysical(crop_top) * resolutionScale);
			scissor.right = int(GetInternalResolution().x - LogicalToPhysical(crop_right) * resolutionScale);
			scissor.bottom = int(GetInternalResolution().y - LogicalToPhysical(crop_bottom) * resolutionScale);
			return scissor;
		}

		const wi::graphics::Texture* GetDepthStencil() const override { return &depthBuffer_Main; }
		const wi::graphics::Texture* GetGUIBlurredBackground() const override { return &rtGUIBlurredBackground[2]; }

		constexpr float getExposure() const { return exposure; }
		constexpr float getBrightness() const { return brightness; }
		constexpr float getContrast() const { return contrast; }
		constexpr float getSaturation() const { return saturation; }
		constexpr float getBloomThreshold() const { return bloomThreshold; }
		constexpr float getMotionBlurStrength() const { return motionBlurStrength; }
		constexpr float getDepthOfFieldStrength() const { return dofStrength; }
		constexpr float getSharpenFilterAmount() const { return sharpenFilterAmount; }
		constexpr float getOutlineThreshold() const { return outlineThreshold; }
		constexpr float getOutlineThickness() const { return outlineThickness; }
		constexpr XMFLOAT4 getOutlineColor() const { return outlineColor; }
		constexpr float getAORange() const { return aoRange; }
		constexpr uint32_t getAOSampleCount() const { return aoSampleCount; }
		constexpr float getAOPower() const { return aoPower; }
		constexpr float getChromaticAberrationAmount() const { return chromaticAberrationAmount; }
		constexpr uint32_t getScreenSpaceShadowSampleCount() const { return screenSpaceShadowSampleCount; }
		constexpr float getScreenSpaceShadowRange() const { return screenSpaceShadowRange; }
		constexpr float getEyeAdaptionKey() const { return eyeadaptionKey; }
		constexpr float getEyeAdaptionRate() const { return eyeadaptionRate; }
		constexpr float getFSRSharpness() const { return fsrSharpness; }
		constexpr float getFSR2Sharpness() const { return fsr2Sharpness; }
		constexpr float getLightShaftsStrength() const { return lightShaftsStrength; }
		constexpr float getRaytracedDiffuseRange() const { return raytracedDiffuseRange; }
		constexpr float getRaytracedReflectionsRange() const { return raytracedReflectionsRange; }
		constexpr float getReflectionRoughnessCutoff() const { return reflectionRoughnessCutoff; }
		constexpr wi::renderer::Tonemap getTonemap() const { return tonemap; }

		constexpr bool getAOEnabled() const { return ao != AO_DISABLED; }
		constexpr AO getAO() const { return ao; }
		constexpr bool getSSREnabled() const { return ssrEnabled; }
		constexpr bool getRaytracedDiffuseEnabled() const { return raytracedDiffuseEnabled; }
		constexpr bool getRaytracedReflectionEnabled() const { return raytracedReflectionsEnabled; }
		constexpr bool getShadowsEnabled() const { return shadowsEnabled; }
		constexpr bool getReflectionsEnabled() const { return reflectionsEnabled; }
		constexpr bool getFXAAEnabled() const { return fxaaEnabled; }
		constexpr bool getBloomEnabled() const { return bloomEnabled; }
		constexpr bool getColorGradingEnabled() const { return colorGradingEnabled; }
		constexpr bool getVolumeLightsEnabled() const { return volumeLightsEnabled; }
		constexpr bool getLightShaftsEnabled() const { return lightShaftsEnabled; }
		constexpr bool getLensFlareEnabled() const { return lensFlareEnabled; }
		constexpr bool getMotionBlurEnabled() const { return motionBlurEnabled; }
		constexpr bool getDepthOfFieldEnabled() const { return depthOfFieldEnabled; }
		constexpr bool getEyeAdaptionEnabled() const { return eyeAdaptionEnabled; }
		constexpr bool getSharpenFilterEnabled() const { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getOutlineEnabled() const { return outlineEnabled; }
		constexpr bool getChromaticAberrationEnabled() const { return chromaticAberrationEnabled; }
		constexpr bool getDitherEnabled() const { return ditherEnabled; }
		constexpr bool getOcclusionCullingEnabled() const { return occlusionCullingEnabled; }
		constexpr bool getSceneUpdateEnabled() const { return sceneUpdateEnabled; }
		constexpr bool getFSREnabled() const { return fsrEnabled; }
		constexpr bool getFSR2Enabled() const { return fsr2Enabled; }
		constexpr bool getVXGIResolveFullResolutionEnabled() const { return vxgiResolveFullResolution; }

		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		constexpr void setExposure(float value) { exposure = value; }
		constexpr void setBrightness(float value) { brightness = value; }
		constexpr void setContrast(float value) { contrast = value; }
		constexpr void setSaturation(float value) { saturation = value; }
		constexpr void setBloomThreshold(float value) { bloomThreshold = value; }
		constexpr void setMotionBlurStrength(float value) { motionBlurStrength = value; }
		constexpr void setDepthOfFieldStrength(float value) { dofStrength = value; }
		constexpr void setSharpenFilterAmount(float value) { sharpenFilterAmount = value; }
		constexpr void setOutlineThreshold(float value) { outlineThreshold = value; }
		constexpr void setOutlineThickness(float value) { outlineThickness = value; }
		constexpr void setOutlineColor(const XMFLOAT4& value) { outlineColor = value; }
		constexpr void setAORange(float value) { aoRange = value; }
		constexpr void setAOSampleCount(uint32_t value) { aoSampleCount = value; }
		constexpr void setAOPower(float value) { aoPower = value; }
		constexpr void setChromaticAberrationAmount(float value) { chromaticAberrationAmount = value; }
		constexpr void setScreenSpaceShadowSampleCount(uint32_t value) { screenSpaceShadowSampleCount = value; }
		constexpr void setScreenSpaceShadowRange(float value) { screenSpaceShadowRange = value; }
		constexpr void setEyeAdaptionKey(float value) { eyeadaptionKey = value; }
		constexpr void setEyeAdaptionRate(float value) { eyeadaptionRate = value; }
		constexpr void setFSRSharpness(float value) { fsrSharpness = value; }
		constexpr void setFSR2Sharpness(float value) { fsr2Sharpness = value; }
		constexpr void setLightShaftsStrength(float value) { lightShaftsStrength = value; }
		constexpr void setRaytracedDiffuseRange(float value) { raytracedDiffuseRange = value; }
		constexpr void setRaytracedReflectionsRange(float value) { raytracedReflectionsRange = value; }
		constexpr void setReflectionRoughnessCutoff(float value) { reflectionRoughnessCutoff = value; }
		constexpr void setTonemap(wi::renderer::Tonemap value) { tonemap = value; }

		void setAO(AO value);
		void setSSREnabled(bool value);
		void setRaytracedReflectionsEnabled(bool value);
		void setRaytracedDiffuseEnabled(bool value);
		void setMotionBlurEnabled(bool value);
		void setDepthOfFieldEnabled(bool value);
		void setEyeAdaptionEnabled(bool value);
		void setReflectionsEnabled(bool value);
		void setBloomEnabled(bool value);
		void setVolumeLightsEnabled(bool value);
		void setLightShaftsEnabled(bool value);
		void setOutlineEnabled(bool value);
		constexpr void setShadowsEnabled(bool value) { shadowsEnabled = value; }
		constexpr void setFXAAEnabled(bool value) { fxaaEnabled = value; }
		constexpr void setColorGradingEnabled(bool value) { colorGradingEnabled = value; }
		constexpr void setLensFlareEnabled(bool value) { lensFlareEnabled = value; }
		constexpr void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
		constexpr void setChromaticAberrationEnabled(bool value) { chromaticAberrationEnabled = value; }
		constexpr void setDitherEnabled(bool value) { ditherEnabled = value; }
		constexpr void setOcclusionCullingEnabled(bool value) { occlusionCullingEnabled = value; }
		constexpr void setSceneUpdateEnabled(bool value) { sceneUpdateEnabled = value; }
		void setFSREnabled(bool value);
		void setFSR2Enabled(bool value);
		void setFSR2Preset(FSR2_Preset preset); // this will modify resolution scaling and sampler lod bias
		void setVXGIResolveFullResolutionEnabled(bool value) { vxgiResolveFullResolution = value; }

		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }

		struct CustomPostprocess
		{
			std::string name = "CustomPostprocess";
			wi::graphics::Shader computeshader;
			XMFLOAT4 params0;
			XMFLOAT4 params1;
			enum class Stage
			{
				BeforeTonemap, // Before tonemap and bloom in HDR color space
				AfterTonemap // After tonemap, in display color space
			} stage = Stage::AfterTonemap;
		};
		wi::vector<CustomPostprocess> custom_post_processes;

		void PreUpdate() override;
		void Update(float dt) override;
		void Render() const override;
		void Compose(wi::graphics::CommandList cmd) const override;

		void Stop() override;
		void Start() override;
	};

}
