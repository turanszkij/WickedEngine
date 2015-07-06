#pragma once
#include "RenderableComponent.h"


class DeferredRenderableComponent :
	public RenderableComponent
{
private:
	wiRenderTarget
		rtReflection
		, rtGBuffer
		, rtDeferred
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

	void RenderReflections();
	void RenderShadows();
	void RenderScene();
	void RenderBloom();
	void RenderLightShafts();
	void RenderComposition1();
	void RenderComposition2();
	void RenderColorGradedComposition();

protected:
	bool ssr, ssao;
public:
	DeferredRenderableComponent();
	~DeferredRenderableComponent();

	void Update();
	void Render();
	void Compose();
};

