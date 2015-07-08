#include "DeferredRenderableComponent.h"
#include "WickedEngine.h"

DeferredRenderableComponent::DeferredRenderableComponent(){
}
DeferredRenderableComponent::~DeferredRenderableComponent(){
}
void DeferredRenderableComponent::Initialize()
{
	Renderable3DSceneComponent::Initialize();

	setSSREnabled(true);
	setSSAOEnabled(true);
}
void DeferredRenderableComponent::Load()
{
	Renderable3DSceneComponent::Load();

	rtGBuffer.Initialize(
		screenW, screenH
		, 3, true, 1, 0
		, DXGI_FORMAT_R16G16B16A16_FLOAT
		);
	rtDeferred.Initialize(
		screenW, screenH
		, 1, false, 1, 0, DXGI_FORMAT_R16G16B16A16_FLOAT, 0);
}
void DeferredRenderableComponent::Start()
{
	Renderable3DSceneComponent::Start();

	wiRenderer::SetEnviromentMap(nullptr);
	wiRenderer::SetColorGrading(nullptr);
}
void DeferredRenderableComponent::Render(){
	RenderReflections();
	RenderShadows();
	RenderScene();
	RenderSecondaryScene(rtGBuffer, rtDeferred);
	RenderLightShafts(rtGBuffer);
	RenderComposition1(rtDeferred);
	RenderBloom();
	RenderComposition2();
}


void DeferredRenderableComponent::RenderScene(){
	static const int tessellationQuality = 0;

	wiRenderer::UpdatePerFrameCB(wiRenderer::immediateContext);
	wiImageEffects fx(screenW, screenH);

	rtGBuffer.Activate(wiRenderer::immediateContext); {
		wiRenderer::UpdatePerRenderCB(wiRenderer::immediateContext, tessellationQuality);
		wiRenderer::UpdatePerViewCB(wiRenderer::immediateContext, wiRenderer::getCamera()->View, wiRenderer::getCamera()->refView, wiRenderer::getCamera()->Projection, wiRenderer::getCamera()->Eye);


		wiRenderer::UpdatePerEffectCB(wiRenderer::immediateContext, XMFLOAT4(0, 0, 0, 0), XMFLOAT4(0, 0, 0, 0));
		wiRenderer::DrawWorld(wiRenderer::getCamera()->View, wiRenderer::DX11, tessellationQuality, wiRenderer::immediateContext, false
			, wiRenderer::SHADED_DEFERRED, rtReflection.shaderResource.front(), true, 2);


	}


	rtLinearDepth.Activate(wiRenderer::immediateContext); {
		wiImage::BatchBegin(wiRenderer::immediateContext);
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(rtGBuffer.depth->shaderResource, fx, wiRenderer::immediateContext);
		fx.process.clear();
	}
	dtDepthCopy.CopyFrom(*rtGBuffer.depth, wiRenderer::immediateContext);

	rtGBuffer.Set(wiRenderer::immediateContext); {
		wiRenderer::DrawDecals(wiRenderer::getCamera()->View, wiRenderer::immediateContext, dtDepthCopy.shaderResource);
	}

	rtLight.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiRenderer::DrawLights(wiRenderer::getCamera()->View, wiRenderer::immediateContext,
			dtDepthCopy.shaderResource, rtGBuffer.shaderResource[1], rtGBuffer.shaderResource[2]);
	}



	if (getSSAOEnabled()){
		wiImage::BatchBegin(wiRenderer::immediateContext, STENCILREF_DEFAULT);
		rtSSAO[0].Activate(wiRenderer::immediateContext); {
			fx.process.setSSAO(true);
			fx.setDepthMap(rtLinearDepth.shaderResource.back());
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setMaskMap((ID3D11ShaderResourceView*)wiResourceManager::add("images/noise.png"));
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, wiRenderer::immediateContext);
			//fx.sampleFlag=SAMPLEMODE_CLAMP;
			fx.process.clear();
		}
		rtSSAO[1].Activate(wiRenderer::immediateContext); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].shaderResource.back(), fx, wiRenderer::immediateContext);
		}
		rtSSAO[2].Activate(wiRenderer::immediateContext); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].shaderResource.back(), fx, wiRenderer::immediateContext);
			fx.blur = 0;
		}
	}


	rtDeferred.Activate(wiRenderer::immediateContext, rtGBuffer.depth); {
		wiImage::DrawDeferred(rtGBuffer.shaderResource[0]
			, rtLinearDepth.shaderResource.back(), rtLight.shaderResource.front(), rtGBuffer.shaderResource[1]
			, rtSSAO.back().shaderResource.back(), wiRenderer::immediateContext, STENCILREF_DEFAULT);
		wiRenderer::DrawSky(wiRenderer::getCamera()->Eye, wiRenderer::immediateContext);
		wiRenderer::DrawDebugLines(wiRenderer::getCamera()->View, wiRenderer::immediateContext);
		wiRenderer::DrawDebugBoxes(wiRenderer::getCamera()->View, wiRenderer::immediateContext);
	}


	if (getSSREnabled()){
		rtSSR.Activate(wiRenderer::immediateContext); {
			wiImage::BatchBegin(wiRenderer::immediateContext);
			wiRenderer::immediateContext->GenerateMips(rtDeferred.shaderResource[0]);
			fx.process.setSSR(true);
			fx.setDepthMap(dtDepthCopy.shaderResource);
			fx.setNormalMap(rtGBuffer.shaderResource[1]);
			fx.setVelocityMap(rtGBuffer.shaderResource[2]);
			fx.setMaskMap(rtLinearDepth.shaderResource.front());
			wiImage::Draw(rtDeferred.shaderResource.front(), fx, wiRenderer::immediateContext);
			fx.process.clear();
		}
	}
}
