#include "ForwardRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiProfiler.h"
#include "wiTextureHelper.h"

using namespace wiGraphicsTypes;

ForwardRenderableComponent::ForwardRenderableComponent()
{
	Renderable3DComponent::setProperties();

	setSSREnabled(false);
	setSSAOEnabled(false);
	setShadowsEnabled(false);
}
ForwardRenderableComponent::~ForwardRenderableComponent()
{
}

wiRenderTarget ForwardRenderableComponent::rtMain;
void ForwardRenderableComponent::ResizeBuffers()
{
	Renderable3DComponent::ResizeBuffers();

	FORMAT defaultTextureFormat = wiRenderer::GetDevice()->GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == wiRenderer::GetInternalResolution().x &&
		lastBufferResHeight == wiRenderer::GetInternalResolution().y &&
		lastBufferMSAA == getMSAASampleCount() &&
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = wiRenderer::GetInternalResolution().x;
		lastBufferResHeight = wiRenderer::GetInternalResolution().y;
		lastBufferMSAA = getMSAASampleCount();
		lastBufferFormat = defaultTextureFormat;
	}

	rtMain.Initialize(wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y, true, wiRenderer::RTFormat_hdr, 1, getMSAASampleCount(), false);
	rtMain.Add(wiRenderer::RTFormat_gbuffer_1); // thin gbuffer
}

void ForwardRenderableComponent::Initialize()
{
	ResizeBuffers();

	Renderable3DComponent::Initialize();
}
void ForwardRenderableComponent::Load()
{
	Renderable3DComponent::Load();

}
void ForwardRenderableComponent::Start()
{
	Renderable3DComponent::Start();
}
void ForwardRenderableComponent::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderShadows(GRAPHICSTHREAD_IMMEDIATE);
	RenderReflections(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);
	RenderSecondaryScene(rtMain, rtMain, GRAPHICSTHREAD_IMMEDIATE);
	RenderComposition(rtMain, rtMain, GRAPHICSTHREAD_IMMEDIATE);

	Renderable2DComponent::Render();
}


void ForwardRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Opaque Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	GPUResource* dsv[] = { rtMain.depth->GetTexture() };
	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_READ, RESOURCE_STATE_DEPTH_WRITE, threadID);

	wiImageEffects fx((float)wiRenderer::GetInternalResolution().x, (float)wiRenderer::GetInternalResolution().y);

	rtMain.Activate(threadID, 0, 0, 0, 0);
	{
		wiRenderer::GetDevice()->BindResource(PS, getReflectionsEnabled() ? rtReflection.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_REFLECTION, threadID);
		wiRenderer::GetDevice()->BindResource(PS, getSSAOEnabled() ? rtSSAO.back().GetTexture() : wiTextureHelper::getInstance()->getWhite(), TEXSLOT_RENDERABLECOMPONENT_SSAO, threadID);
		wiRenderer::GetDevice()->BindResource(PS, getSSREnabled() ? rtSSR.GetTexture() : wiTextureHelper::getInstance()->getTransparent(), TEXSLOT_RENDERABLECOMPONENT_SSR, threadID);
		wiRenderer::DrawWorld(wiRenderer::getCamera(), getTessellationEnabled(), threadID, SHADERTYPE_FORWARD, getHairParticlesEnabled(), true, getLayerMask());
		wiRenderer::DrawSky(threadID);
	}
	rtMain.Deactivate(threadID);
	wiRenderer::BindGBufferTextures(rtMain.GetTextureResolvedMSAA(threadID, 0), rtMain.GetTextureResolvedMSAA(threadID, 1), nullptr, nullptr, nullptr, threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_DEPTH_WRITE, RESOURCE_STATE_COPY_SOURCE, threadID);

	dtDepthCopy.CopyFrom(*rtMain.depth, threadID);

	wiRenderer::GetDevice()->TransitionBarrier(dsv, ARRAYSIZE(dsv), RESOURCE_STATE_COPY_SOURCE, RESOURCE_STATE_DEPTH_READ, threadID);

	rtLinearDepth.Activate(threadID); {
		fx.blendFlag = BLENDMODE_OPAQUE;
		fx.sampleFlag = SAMPLEMODE_CLAMP;
		fx.quality = QUALITY_NEAREST;
		fx.process.setLinDepth(true);
		wiImage::Draw(dtDepthCopy.GetTextureResolvedMSAA(threadID), fx, threadID);
		fx.process.clear();
	}
	rtLinearDepth.Deactivate(threadID);

	wiRenderer::BindDepthTextures(dtDepthCopy.GetTextureResolvedMSAA(threadID), rtLinearDepth.GetTexture(), threadID);


	if (getSSAOEnabled()) {
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSAO, 1, threadID);
		wiRenderer::GetDevice()->EventBegin("SSAO", threadID);
		fx.stencilRef = STENCILREF_DEFAULT;
		fx.stencilComp = STENCILMODE_LESS;
		rtSSAO[0].Activate(threadID); {
			fx.process.setSSAO(true);
			fx.setMaskMap(wiTextureHelper::getInstance()->getRandom64x64());
			fx.quality = QUALITY_BILINEAR;
			fx.sampleFlag = SAMPLEMODE_MIRROR;
			wiImage::Draw(nullptr, fx, threadID);
			fx.process.clear();
		}
		rtSSAO[1].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 0;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[0].GetTexture(), fx, threadID);
		}
		rtSSAO[2].Activate(threadID); {
			fx.blur = getSSAOBlur();
			fx.blurDir = 1;
			fx.blendFlag = BLENDMODE_OPAQUE;
			wiImage::Draw(rtSSAO[1].GetTexture(), fx, threadID);
			fx.blur = 0;
		}
		fx.stencilRef = 0;
		fx.stencilComp = STENCILMODE_DISABLED;
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	if (getSSREnabled()) {
		wiRenderer::GetDevice()->UnbindResources(TEXSLOT_RENDERABLECOMPONENT_SSR, 1, threadID);
		wiRenderer::GetDevice()->EventBegin("SSR", threadID);
		rtSSR.Activate(threadID); {
			fx.process.clear();
			fx.presentFullScreen = false;
			fx.process.setSSR(true);
			fx.setMaskMap(nullptr);
			wiImage::Draw(rtMain.GetTexture(), fx, threadID);
			fx.process.clear();
		}
		wiRenderer::GetDevice()->EventEnd(threadID);
	}

	wiProfiler::GetInstance().EndRange(threadID); // Opaque Scene
}

wiDepthTarget* ForwardRenderableComponent::GetDepthBuffer()
{
	return rtMain.depth;
}
