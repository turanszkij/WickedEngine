#pragma once
#include "RenderPath2D.h"
#include "wiRenderer.h"
#include "wiGraphicsDevice.h"

class RenderPath3D :
	public RenderPath2D
{
private:
	float exposure = 1.0f;
	float lightShaftQuality = 0.4f;
	float bloomThreshold = 1.0f;
	float particleDownSample = 1.0f;
	float reflectionQuality = 0.5f;
	float ssaoQuality = 0.5f;
	float ssaoBlur = 2.3f;
	float dofFocus = 10.0f;
	float dofStrength = 2.2f;
	float sharpenFilterAmount = 0.28f;
	float outlineThreshold = 0.2f;
	float outlineThickness = 1.0f;
	XMFLOAT3 outlineColor = XMFLOAT3(0, 0, 0);
	float ssaoRange = 1.0f;
	UINT ssaoSampleCount = 16;

	bool fxaaEnabled = false;
	bool ssaoEnabled = false;
	bool ssrEnabled = false;
	bool reflectionsEnabled = true;
	bool shadowsEnabled = true;
	bool bloomEnabled = true;
	bool colorGradingEnabled = false;
	bool emittedParticlesEnabled = true;
	bool hairParticlesEnabled = true;
	bool hairParticlesReflectionEnabled = false;
	bool volumeLightsEnabled = true;
	bool lightShaftsEnabled = false;
	bool lensFlareEnabled = false;
	bool motionBlurEnabled = false;
	bool sssEnabled = true;
	bool depthOfFieldEnabled = false;
	bool eyeAdaptionEnabled = false;
	bool tessellationEnabled = false;
	bool sharpenFilterEnabled = false;
	bool outlineEnabled = false;

	const wiGraphics::Texture2D* colorGradingTex = nullptr;

	UINT msaaSampleCount = 1;

protected:
	wiGraphics::Texture2D rtReflection; // conains the scene rendered for planar reflections
	wiGraphics::Texture2D rtSSR; // screen-space reflection results
	wiGraphics::Texture2D rtSceneCopy; // contains the rendered scene that can be fed into transparent pass for distortion effect
	wiGraphics::Texture2D rtWaterRipple; // water ripple sprite normal maps are rendered into this
	wiGraphics::Texture2D rtParticle; // contains off-screen particles
	wiGraphics::Texture2D rtVolumetricLights; // contains the volumetric light results
	wiGraphics::Texture2D rtDof[2]; // depth of field blurred out-of focus part
	wiGraphics::Texture2D rtTemporalAA[2]; // temporal AA history buffer
	wiGraphics::Texture2D rtBloom; // contains the bright parts of the image + mipchain
	wiGraphics::Texture2D rtSSAO[2]; // ping-pong when rendering and blurring SSAO
	wiGraphics::Texture2D rtSun[2]; // 0: sun render target used for lightshafts (can be MSAA), 1: radial blurred lightshafts
	wiGraphics::Texture2D rtSun_resolved; // sun render target, but the resolved version if MSAA is enabled

	wiGraphics::Texture2D rtPostprocess_HDR; // ping-pong with main scene RT in HDR post-process chain
	wiGraphics::Texture2D rtPostprocess_LDR[2]; // ping-pong with itself in LDR post-process chain

	wiGraphics::Texture2D depthBuffer; // used for depth-testing, can be MSAA
	wiGraphics::Texture2D depthBuffer_Copy; // used for shader resource, single sample
	wiGraphics::Texture2D rtLinearDepth; // linear depth result
	wiGraphics::Texture2D smallDepth; // downsampled depth buffer
	wiGraphics::Texture2D depthBuffer_reflection; // depth-test for reflection rendering

	// Post-processes are ping-ponged, this function helps to obtain the last postprocess render target that was written
	const wiGraphics::Texture2D* GetLastPostprocessRT() const
	{
		int ldr_postprocess_count = 0;
		ldr_postprocess_count += sharpenFilterEnabled ? 1 : 0;
		ldr_postprocess_count += colorGradingEnabled ? 1 : 0;
		ldr_postprocess_count += fxaaEnabled ? 1 : 0;
		int rt_index = ldr_postprocess_count % 2;
		return &rtPostprocess_LDR[rt_index];
	}

	void ResizeBuffers() override;

	virtual void RenderFrameSetUp(GRAPHICSTHREAD threadID) const;
	virtual void RenderReflections(GRAPHICSTHREAD threadID) const;
	virtual void RenderShadows(GRAPHICSTHREAD threadID) const;

	virtual void RenderLinearDepth(GRAPHICSTHREAD threadID) const;
	virtual void RenderSSAO(GRAPHICSTHREAD threadID) const;
	virtual void RenderSSR(const wiGraphics::Texture2D& srcSceneRT, GRAPHICSTHREAD threadID) const;
	virtual void DownsampleDepthBuffer(GRAPHICSTHREAD threadID) const;
	virtual void RenderOutline(const wiGraphics::Texture2D& dstSceneRT, GRAPHICSTHREAD threadID) const;
	virtual void RenderLightShafts(GRAPHICSTHREAD threadID) const;
	virtual void RenderVolumetrics(GRAPHICSTHREAD threadID) const;
	virtual void RenderParticles(bool isDistrortionPass, GRAPHICSTHREAD threadID) const;
	virtual void RenderRefractionSource(const wiGraphics::Texture2D& srcSceneRT, GRAPHICSTHREAD threadID) const;
	virtual void RenderTransparents(const wiGraphics::Texture2D& dstSceneRT, RENDERPASS renderPass, GRAPHICSTHREAD threadID) const;
	virtual void TemporalAAResolve(const wiGraphics::Texture2D& srcdstSceneRT, const wiGraphics::Texture2D& srcGbuffer1, GRAPHICSTHREAD threadID) const;
	virtual void RenderBloom(const wiGraphics::Texture2D& srcdstSceneRT, GRAPHICSTHREAD threadID) const;
	virtual void RenderPostprocessChain(const wiGraphics::Texture2D& srcSceneRT, const wiGraphics::Texture2D& srcGbuffer1, GRAPHICSTHREAD threadID) const;
	
public:
	const wiGraphics::Texture2D* GetDepthStencil() const override { return &depthBuffer; }

	inline float getExposure() const { return exposure; }
	inline float getLightShaftQuality() const { return lightShaftQuality; }
	inline float getBloomThreshold() const { return bloomThreshold; }
	inline float getParticleDownSample() const { return particleDownSample; }
	inline float getReflectionQuality() const { return reflectionQuality; }
	inline float getSSAOQuality() const { return ssaoQuality; }
	inline float getSSAOBlur() const { return ssaoBlur; }
	inline float getDepthOfFieldFocus() const { return dofFocus; }
	inline float getDepthOfFieldStrength() const { return dofStrength; }
	inline float getSharpenFilterAmount() const { return sharpenFilterAmount; }
	inline float getOutlineThreshold() const { return outlineThreshold; }
	inline float getOutlineThickness() const { return outlineThickness; }
	inline XMFLOAT3 getOutlineColor() const { return outlineColor; }
	inline float getSSAORange() const { return ssaoRange; }
	inline UINT getSSAOSampleCount() const { return ssaoSampleCount; }

	inline bool getSSAOEnabled() const { return ssaoEnabled; }
	inline bool getSSREnabled() const { return ssrEnabled; }
	inline bool getShadowsEnabled() const { return shadowsEnabled; }
	inline bool getReflectionsEnabled() const { return reflectionsEnabled; }
	inline bool getFXAAEnabled() const { return fxaaEnabled; }
	inline bool getBloomEnabled() const { return bloomEnabled; }
	inline bool getColorGradingEnabled() const { return colorGradingEnabled; }
	inline bool getEmittedParticlesEnabled() const { return emittedParticlesEnabled; }
	inline bool getHairParticlesEnabled() const { return hairParticlesEnabled; }
	inline bool getHairParticlesReflectionEnabled() const { return hairParticlesReflectionEnabled; }
	inline bool getVolumeLightsEnabled() const { return volumeLightsEnabled; }
	inline bool getLightShaftsEnabled() const { return lightShaftsEnabled; }
	inline bool getLensFlareEnabled() const { return lensFlareEnabled; }
	inline bool getMotionBlurEnabled() const { return motionBlurEnabled; }
	inline bool getSSSEnabled() const { return sssEnabled; }
	inline bool getDepthOfFieldEnabled() const { return depthOfFieldEnabled; }
	inline bool getEyeAdaptionEnabled() const { return eyeAdaptionEnabled; }
	inline bool getTessellationEnabled() const { return tessellationEnabled && wiRenderer::GetDevice()->CheckCapability(wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION); }
	inline bool getSharpenFilterEnabled() const { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
	inline bool getOutlineEnabled() const { return outlineEnabled; }

	inline const wiGraphics::Texture2D* getColorGradingTexture() const { return colorGradingTex; }

	inline UINT getMSAASampleCount() const { return msaaSampleCount; }

	inline void setExposure(float value) { exposure = value; }
	inline void setLightShaftQuality(float value){ lightShaftQuality = value; }
	inline void setBloomThreshold(float value){ bloomThreshold = value; }
	inline void setParticleDownSample(float value){ particleDownSample = value; }
	inline void setReflectionQuality(float value){ reflectionQuality = value; }
	inline void setSSAOQuality(float value){ ssaoQuality = value; }
	inline void setSSAOBlur(float value){ ssaoBlur = value; }
	inline void setDepthOfFieldFocus(float value){ dofFocus = value; }
	inline void setDepthOfFieldStrength(float value){ dofStrength = value; }
	inline void setSharpenFilterAmount(float value) { sharpenFilterAmount = value; }
	inline void setOutlineThreshold(float value) { outlineThreshold = value; }
	inline void setOutlineThickness(float value) { outlineThickness = value; }
	inline void setOutlineColor(const XMFLOAT3& value) { outlineColor = value; }
	inline void setSSAORange(float value) { ssaoRange = value; }
	inline void setSSAOSampleCount(UINT value) { ssaoSampleCount = value; }

	inline void setSSAOEnabled(bool value){ ssaoEnabled = value; }
	inline void setSSREnabled(bool value){ ssrEnabled = value; }
	inline void setShadowsEnabled(bool value){ shadowsEnabled = value; }
	inline void setReflectionsEnabled(bool value){ reflectionsEnabled = value; }
	inline void setFXAAEnabled(bool value){ fxaaEnabled = value; }
	inline void setBloomEnabled(bool value){ bloomEnabled = value; }
	inline void setColorGradingEnabled(bool value){ colorGradingEnabled = value; }
	inline void setEmitterParticlesEnabled(bool value){ emittedParticlesEnabled = value; }
	inline void setHairParticlesEnabled(bool value) { hairParticlesEnabled = value; }
	inline void setHairParticlesReflectionEnabled(bool value) { hairParticlesReflectionEnabled = value; }
	inline void setVolumeLightsEnabled(bool value){ volumeLightsEnabled = value; }
	inline void setLightShaftsEnabled(bool value){ lightShaftsEnabled = value; }
	inline void setLensFlareEnabled(bool value){ lensFlareEnabled = value; }
	inline void setMotionBlurEnabled(bool value){ motionBlurEnabled = value; }
	inline void setSSSEnabled(bool value){ sssEnabled = value; }
	inline void setDepthOfFieldEnabled(bool value){ depthOfFieldEnabled = value; }
	inline void setEyeAdaptionEnabled(bool value) { eyeAdaptionEnabled = value; }
	inline void setTessellationEnabled(bool value) { tessellationEnabled = value; }
	inline void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
	inline void setOutlineEnabled(bool value) { outlineEnabled = value; }

	inline void setColorGradingTexture(const wiGraphics::Texture2D* tex) { colorGradingTex = tex; }

	virtual void setMSAASampleCount(UINT value) { if (msaaSampleCount != value) { msaaSampleCount = value; ResizeBuffers(); } }

	void Update(float dt) override;
	void Render() const override = 0;
	void Compose() const override;
};

