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
	bool stereogramEnabled = false;
	bool eyeAdaptionEnabled = false;
	bool tessellationEnabled = false;
	bool sharpenFilterEnabled = false;
	bool outlineEnabled = false;

	const wiGraphics::Texture2D* colorGradingTex = nullptr;

	UINT msaaSampleCount = 1;

protected:
	wiGraphics::Texture2D rtReflection;
	wiGraphics::Texture2D rtSSR;
	wiGraphics::Texture2D rtMotionBlur;
	wiGraphics::Texture2D rtSceneCopy;
	wiGraphics::Texture2D rtWaterRipple;
	wiGraphics::Texture2D rtLinearDepth;
	wiGraphics::Texture2D rtParticle;
	wiGraphics::Texture2D rtVolumetricLights;
	wiGraphics::Texture2D rtFinal[2];
	wiGraphics::Texture2D rtDof[3];
	wiGraphics::Texture2D rtTemporalAA[2];
	wiGraphics::Texture2D rtBloom;
	wiGraphics::Texture2D rtSSAO[3];
	wiGraphics::Texture2D rtSun[2];
	wiGraphics::Texture2D rtSun_resolved;

	wiGraphics::Texture2D depthBuffer;
	wiGraphics::Texture2D depthCopy;
	wiGraphics::Texture2D smallDepth;
	wiGraphics::Texture2D depthBuffer_reflection;

	void ResizeBuffers() override;

	virtual void RenderFrameSetUp(GRAPHICSTHREAD threadID);
	virtual void RenderReflections(GRAPHICSTHREAD threadID);
	virtual void RenderShadows(GRAPHICSTHREAD threadID);
	virtual void RenderScene(GRAPHICSTHREAD threadID) = 0;
	virtual void RenderSecondaryScene(const wiGraphics::Texture2D& mainRT, const wiGraphics::Texture2D& shadedSceneRT, GRAPHICSTHREAD threadID);
	virtual void RenderTransparentScene(const wiGraphics::Texture2D& refractionRT, GRAPHICSTHREAD threadID);
	virtual void RenderComposition(const wiGraphics::Texture2D& shadedSceneRT, const wiGraphics::Texture2D& mainRT0, const wiGraphics::Texture2D& mainRT1, GRAPHICSTHREAD threadID);
	virtual void RenderColorGradedComposition();
public:
	virtual const wiGraphics::Texture2D* GetDepthBuffer() { return &depthBuffer; }

	inline float getExposure() { return exposure; }
	inline float getLightShaftQuality(){ return lightShaftQuality; }
	inline float getBloomThreshold(){ return bloomThreshold; }
	inline float getParticleDownSample(){ return particleDownSample; }
	inline float getReflectionQuality(){ return reflectionQuality; }
	inline float getSSAOQuality(){ return ssaoQuality; }
	inline float getSSAOBlur(){ return ssaoBlur; }
	inline float getDepthOfFieldFocus(){ return dofFocus; }
	inline float getDepthOfFieldStrength(){ return dofStrength; }
	inline float getSharpenFilterAmount() { return sharpenFilterAmount; }
	inline float getOutlineThreshold() { return outlineThreshold; }
	inline float getOutlineThickness() { return outlineThickness; }
	inline XMFLOAT3 getOutlineColor() { return outlineColor; }
	inline float getSSAORange() { return ssaoRange; }
	inline UINT getSSAOSampleCount() { return ssaoSampleCount; }

	inline bool getSSAOEnabled(){ return ssaoEnabled; }
	inline bool getSSREnabled(){ return ssrEnabled; }
	inline bool getShadowsEnabled(){ return shadowsEnabled; }
	inline bool getReflectionsEnabled(){ return reflectionsEnabled; }
	inline bool getFXAAEnabled(){ return fxaaEnabled; }
	inline bool getBloomEnabled(){ return bloomEnabled; }
	inline bool getColorGradingEnabled(){ return colorGradingEnabled; }
	inline bool getEmittedParticlesEnabled(){ return emittedParticlesEnabled; }
	inline bool getHairParticlesEnabled() { return hairParticlesEnabled; }
	inline bool getHairParticlesReflectionEnabled() { return hairParticlesReflectionEnabled; }
	inline bool getVolumeLightsEnabled(){ return volumeLightsEnabled; }
	inline bool getLightShaftsEnabled(){ return lightShaftsEnabled; }
	inline bool getLensFlareEnabled(){ return lensFlareEnabled; }
	inline bool getMotionBlurEnabled(){ return motionBlurEnabled; }
	inline bool getSSSEnabled(){ return sssEnabled; }
	inline bool getDepthOfFieldEnabled(){ return depthOfFieldEnabled; }
	inline bool getStereogramEnabled() { return stereogramEnabled; }
	inline bool getEyeAdaptionEnabled() { return eyeAdaptionEnabled; }
	inline bool getTessellationEnabled() { return tessellationEnabled && wiRenderer::GetDevice()->CheckCapability(wiGraphics::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION); }
	inline bool getSharpenFilterEnabled() { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
	inline bool getOutlineEnabled() { return outlineEnabled; }

	inline const wiGraphics::Texture2D* getColorGradingTexture() { return colorGradingTex; }

	inline UINT getMSAASampleCount() { return msaaSampleCount; }

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
	inline void setStereogramEnabled(bool value) { stereogramEnabled = value; }
	inline void setEyeAdaptionEnabled(bool value) { eyeAdaptionEnabled = value; }
	inline void setTessellationEnabled(bool value) { tessellationEnabled = value; }
	inline void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
	inline void setOutlineEnabled(bool value) { outlineEnabled = value; }

	inline void setColorGradingTexture(const wiGraphics::Texture2D* tex) { colorGradingTex = tex; }

	inline void setMSAASampleCount(UINT value) { if (msaaSampleCount != value) { msaaSampleCount = value; ResizeBuffers(); } }

	void Initialize() override;
	void Load() override;
	void Start() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void Compose() override;
};

