#pragma once
#include "Renderable2DComponent.h"
#include "wiTaskThread.h"
#include "wiRenderer.h"
#include "wiWaterPlane.h"
#include "wiGraphicsDevice.h"
#include <atomic>

class Renderable3DComponent :
	public Renderable2DComponent
{
private:
	float lightShaftQuality;
	float bloomDownSample;
	float bloomStren;
	float bloomThreshold;
	float bloomSaturation;
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
		;
	static std::vector<wiRenderTarget> rtSun, rtBloom, rtSSAO;
	static wiDepthTarget dtDepthCopy;
	static wiGraphicsTypes::Texture2D* smallDepth;

	virtual void ResizeBuffers() override;

	std::vector<wiTaskThread*> workerThreads;

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

	inline float getLightShaftQuality(){ return lightShaftQuality; }
	inline float getBloomDownSample(){ return bloomDownSample; }
	inline float getBloomStrength(){ return bloomStren; }
	inline float getBloomThreshold(){ return bloomThreshold; }
	inline float getBloomSaturation(){ return bloomSaturation; }
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

	inline UINT getMSAASampleCount() { return msaaSampleCount; }

	inline unsigned int getThreadingCount(){ return (unsigned int)workerThreads.size(); }

	inline void setLightShaftQuality(float value){ lightShaftQuality = value; }
	inline void setBloomDownSample(float value){ bloomDownSample = value; }
	inline void setBloomStrength(float value){ bloomStren = value; }
	inline void setBloomThreshold(float value){ bloomThreshold = value; }
	inline void setBloomSaturation(float value){ bloomSaturation = value; }
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

	inline void setMSAASampleCount(UINT value) { msaaSampleCount = value; ResizeBuffers(); }


	virtual void setPreferredThreadingCount(unsigned short value);

	void setProperties();

	Renderable3DComponent();
	virtual ~Renderable3DComponent();

	virtual void Initialize() override;
	virtual void Load() override;
	virtual void Start() override;
	virtual void FixedUpdate() override;
	virtual void Update(float dt) override;
	virtual void Compose() override;
};

