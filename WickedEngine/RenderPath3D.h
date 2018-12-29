#pragma once
#include "RenderPath2D.h"
#include "wiRenderer.h"
#include "wiGraphicsDevice.h"

class RenderPath3D :
	public RenderPath2D
{
private:
	float exposure = 1.0f;
	float lightShaftQuality;
	float bloomThreshold;
	float particleDownSample;
	float reflectionQuality;
	float ssaoQuality;
	float ssaoBlur;
	float dofFocus;
	float dofStrength;
	float sharpenFilterAmount;

	bool fxaaEnabled;
	bool ssaoEnabled;
	bool ssrEnabled;
	bool reflectionsEnabled;
	bool shadowsEnabled;
	bool bloomEnabled;
	bool colorGradingEnabled;
	bool emittedParticlesEnabled;
	bool hairParticlesEnabled;
	bool hairParticlesReflectionEnabled;
	bool volumeLightsEnabled;
	bool lightShaftsEnabled;
	bool lensFlareEnabled;
	bool motionBlurEnabled;
	bool sssEnabled;
	bool depthOfFieldEnabled;
	bool stereogramEnabled;
	bool eyeAdaptionEnabled;
	bool tessellationEnabled;
	bool sharpenFilterEnabled;

	wiGraphicsTypes::Texture2D* colorGradingTex = nullptr;

	UINT msaaSampleCount;

protected:
	static wiRenderTarget
		rtReflection
		, rtSSR
		, rtMotionBlur
		, rtSceneCopy
		, rtWaterRipple
		, rtLinearDepth
		, rtParticle
		, rtVolumetricLights
		, rtFinal[2]
		, rtDof[3]
		, rtTemporalAA[2]
		, rtBloom
		;
	static std::vector<wiRenderTarget> rtSun, rtSSAO;
	static wiGraphicsTypes::Texture2D* depthBuffer;
	static wiDepthTarget dtDepthCopy;
	static wiGraphicsTypes::Texture2D* smallDepth;

	virtual void ResizeBuffers() override;

	virtual void RenderFrameSetUp(GRAPHICSTHREAD threadID);
	virtual void RenderReflections(GRAPHICSTHREAD threadID);
	virtual void RenderShadows(GRAPHICSTHREAD threadID);
	virtual void RenderScene(GRAPHICSTHREAD threadID) = 0;
	virtual void RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, GRAPHICSTHREAD threadID);
	virtual void RenderTransparentScene(wiRenderTarget& refractionRT, GRAPHICSTHREAD threadID);
	virtual void RenderComposition(wiRenderTarget& shadedSceneRT, wiRenderTarget& mainRT, GRAPHICSTHREAD threadID);
	virtual void RenderColorGradedComposition();
public:
	virtual wiDepthTarget* GetDepthBuffer() = 0;

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
	inline bool getTessellationEnabled() { return tessellationEnabled && wiRenderer::GetDevice()->CheckCapability(wiGraphicsTypes::GraphicsDevice::GRAPHICSDEVICE_CAPABILITY_TESSELLATION); }
	inline bool getSharpenFilterEnabled() { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }

	inline wiGraphicsTypes::Texture2D* getColorGradingTexture() { return colorGradingTex; }

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

	inline void setColorGradingTexture(wiGraphicsTypes::Texture2D* tex) { colorGradingTex = tex; }

	inline void setMSAASampleCount(UINT value) { msaaSampleCount = value; ResizeBuffers(); }

	void setProperties();

	RenderPath3D();
	virtual ~RenderPath3D();

	void Initialize() override;
	void Load() override;
	void Start() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void Compose() override;
};

