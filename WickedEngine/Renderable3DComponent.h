#pragma once
#include "Renderable2DComponent.h"
#include "wiTaskThread.h"
#include "wiRenderer.h"
#include "wiWaterPlane.h"

class Renderable3DComponent :
	public Renderable2DComponent
{
private:
	float lightShaftQuality;
	float bloomDownSample;
	float bloomStren;
	float bloomThreshold;
	float bloomSaturation;
	float particleAlphaDownSample;
	float particleAdditiveDownSample;
	float reflectionQuality;
	float ssaoQuality;
	float ssaoBlur;
	float dofFocus;
	float dofStrength;

	wiWaterPlane waterPlane;

	bool fxaaEnabled;
	bool ssaoEnabled;
	bool ssrEnabled;
	bool reflectionsEnabled;
	bool shadowsEnabled;
	bool bloomEnabled;
	bool colorGradingEnabled;
	bool emittedParticlesEnabled;
	bool hairParticlesEnabled;
	bool volumeLightsEnabled;
	bool lightShaftsEnabled;
	bool lensFlareEnabled;
	bool motionBlurEnabled;
	bool sssEnabled;
	bool depthOfFieldEnabled;

protected:
	wiRenderTarget
		rtReflection
		, rtSSR
		, rtMotionBlur
		, rtVolumeLight
		, rtTransparent
		, rtWater
		, rtWaterRipple
		, rtLinearDepth
		, rtParticle
		, rtParticleAdditive
		, rtLensFlare
		, rtFinal[2]
		, rtDof[3]
		;
	vector<wiRenderTarget> rtSun, rtBloom, rtSSAO, rtSSS;
	wiDepthTarget dtDepthCopy;

	vector<wiTaskThread*> workerThreads;

	virtual void RenderReflections(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderShadows(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderScene(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext()) = 0;
	virtual void RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderBloom(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderLightShafts(wiRenderTarget& mainRT, wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderComposition1(wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderComposition2(wiRenderer::DeviceContext context = wiRenderer::getImmediateContext());
	virtual void RenderColorGradedComposition();
public:
	float getLightShaftQuality(){ return lightShaftQuality; }
	float getBloomDownSample(){ return bloomDownSample; }
	float getBloomStrength(){ return bloomStren; }
	float getBloomThreshold(){ return bloomThreshold; }
	float getBloomSaturation(){ return bloomSaturation; }
	float getAlphaParticleDownSample(){ return particleAlphaDownSample; }
	float getAdditiveParticleDownSample(){ return particleAdditiveDownSample; }
	float getReflectionQuality(){ return reflectionQuality; }
	float getSSAOQuality(){ return ssaoQuality; }
	float getSSAOBlur(){ return ssaoBlur; }
	float getDepthOfFieldFocus(){ return dofFocus; }
	float getDepthOfFieldStrength(){ return dofStrength; }

	wiWaterPlane getWaterPlane(){ return waterPlane; }

	bool getSSAOEnabled(){ return ssaoEnabled; }
	bool getSSREnabled(){ return ssrEnabled; }
	bool getShadowsEnabled(){ return shadowsEnabled; }
	bool getReflectionsEnabled(){ return reflectionsEnabled; }
	bool getFXAAEnabled(){ return fxaaEnabled; }
	bool getBloomEnabled(){ return bloomEnabled; }
	bool getColorGradingEnabled(){ return colorGradingEnabled; }
	bool getEmittedParticlesEnabled(){ return emittedParticlesEnabled; }
	bool getHairParticlesEnabled(){ return hairParticlesEnabled; }
	bool getVolumeLightsEnabled(){ return volumeLightsEnabled; }
	bool getLightShaftsEnabled(){ return lightShaftsEnabled; }
	bool getLensFlareEnabled(){ return lensFlareEnabled; }
	bool getMotionBlurEnabled(){ return motionBlurEnabled; }
	bool getSSSEnabled(){ return sssEnabled; }
	bool getDepthOfFieldEnabled(){ return depthOfFieldEnabled; }

	unsigned short getThreadingCount(){ return (unsigned short)workerThreads.size(); }

	void setLightShaftQuality(float value){ lightShaftQuality = value; }
	void setBloomDownSample(float value){ bloomDownSample = value; }
	void setBloomStrength(float value){ bloomStren = value; }
	void setBloomThreshold(float value){ bloomThreshold = value; }
	void setBloomSaturation(float value){ bloomSaturation = value; }
	void setAlphaParticleDownSample(float value){ particleAlphaDownSample = value; }
	void setAdditiveParticleDownSample(float value){ particleAdditiveDownSample = value; }
	void setReflectionQuality(float value){ reflectionQuality = value; }
	void setSSAOQuality(float value){ ssaoQuality = value; }
	void setSSAOBlur(float value){ ssaoBlur = value; }
	void setDepthOfFieldFocus(float value){ dofFocus = value; }
	void setDepthOfFieldStrength(float value){ dofStrength = value; }

	void setWaterPlane(const wiWaterPlane& value){ waterPlane = value; }

	void setSSAOEnabled(bool value){ ssaoEnabled = value; }
	void setSSREnabled(bool value){ ssrEnabled = value; }
	void setShadowsEnabled(bool value){ shadowsEnabled = value; }
	void setReflectionsEnabled(bool value){ reflectionsEnabled = value; }
	void setFXAAEnabled(bool value){ fxaaEnabled = value; }
	void setBloomEnabled(bool value){ bloomEnabled = value; }
	void setColorGradingEnabled(bool value){ colorGradingEnabled = value; }
	void setEmitterParticlesEnabled(bool value){ emittedParticlesEnabled = value; }
	void setHairParticlesEnabled(bool value){ hairParticlesEnabled = value; }
	void setVolumeLightsEnabled(bool value){ volumeLightsEnabled = value; }
	void setLightShaftsEnabled(bool value){ lightShaftsEnabled = value; }
	void setLensFlareEnabled(bool value){ lensFlareEnabled = value; }
	void setMotionBlurEnabled(bool value){ motionBlurEnabled = value; }
	void setSSSEnabled(bool value){ sssEnabled = value; }
	void setDepthOfFieldEnabled(bool value){ depthOfFieldEnabled = value; }

	virtual void setPreferredThreadingCount(unsigned short value);

	void setProperties();

	Renderable3DComponent();
	~Renderable3DComponent();

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Update();
	virtual void Compose();
};

