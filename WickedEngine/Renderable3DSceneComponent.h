#pragma once
#include "RenderableComponent.h"
#include "wiTaskThread.h"
#include "wiRenderer.h"
#include "wiWaterPlane.h"

class Renderable3DSceneComponent :
	public RenderableComponent
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
	float ssrQuality; 

	wiWaterPlane waterPlane;

	bool fxaaEnabled;
	bool ssaoEnabled;
	bool ssrEnabled;
	bool reflectionsEnabled;
	bool shadowsEnabled;
	bool bloomEnabled;
	bool colorGradingEnabled;

protected:
	wiRenderTarget
		rtReflection
		, rtSSR
		, rtLight
		, rtVolumeLight
		, rtTransparent
		, rtWater
		, rtWaterRipple
		, rtLinearDepth
		, rtParticle
		, rtParticleAdditive
		, rtLensFlare
		, rtFinal[2]
		;
	vector<wiRenderTarget> rtSun, rtBloom, rtSSAO;
	wiDepthTarget dtDepthCopy;

	vector<wiTaskThread*> workerThreads;

	virtual void RenderReflections(wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderShadows(wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderScene(wiRenderer::DeviceContext context = wiRenderer::immediateContext) = 0;
	virtual void RenderSecondaryScene(wiRenderTarget& mainRT, wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderBloom(wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderLightShafts(wiRenderTarget& mainRT, wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderComposition1(wiRenderTarget& shadedSceneRT, wiRenderer::DeviceContext context = wiRenderer::immediateContext);
	virtual void RenderComposition2(wiRenderer::DeviceContext context = wiRenderer::immediateContext);
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
	float getSSRQuality(){ return ssrQuality; }

	wiWaterPlane getWaterPlane(){ return waterPlane; }

	bool getSSAOEnabled(){ return ssaoEnabled; }
	bool getSSREnabled(){ return ssrEnabled; }
	bool getShadowsEnabled(){ return shadowsEnabled; }
	bool getReflectionsEnabled(){ return reflectionsEnabled; }
	bool getFXAAEnabled(){ return fxaaEnabled; }
	bool getBloomEnabled(){ return bloomEnabled; }
	bool getColorGradingEnabled(){ return colorGradingEnabled; }

	unsigned short getThreadingCount(){ return workerThreads.size(); }

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
	void setSSRQuality(float value){ ssrQuality = value; }

	void setWaterPlane(const wiWaterPlane& value){ waterPlane = value; }

	void setSSAOEnabled(bool value){ ssaoEnabled = value; }
	void setSSREnabled(bool value){ ssrEnabled = value; }
	void setShadowsEnabled(bool value){ shadowsEnabled = value; }
	void setReflectionsEnabled(bool value){ reflectionsEnabled = value; }
	void setFXAAEnabled(bool value){ fxaaEnabled = value; }
	void setBloomEnabled(bool value){ bloomEnabled = value; }
	void setColorGradingEnabled(bool value){ colorGradingEnabled = value; }

	virtual void setPreferredThreadingCount(unsigned short value);

	Renderable3DSceneComponent();
	~Renderable3DSceneComponent();

	virtual void Initialize();
	virtual void Load();
	virtual void Start();
	virtual void Update();
	virtual void Compose();
};

