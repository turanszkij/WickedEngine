#include "PathTracingRenderableComponent.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiImageEffects.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"

using namespace wiGraphicsTypes;

PathTracingRenderableComponent::PathTracingRenderableComponent()
{
	Renderable3DComponent::setProperties();
}
PathTracingRenderableComponent::~PathTracingRenderableComponent()
{
}


wiGraphicsTypes::Texture2D* PathTracingRenderableComponent::traceResult = nullptr;
wiRenderTarget PathTracingRenderableComponent::rtAccumulation;
void PathTracingRenderableComponent::ResizeBuffers()
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

	
	SAFE_DELETE(traceResult);
	
	TextureDesc desc;
	desc.Width = lastBufferResWidth;
	desc.Height = lastBufferResHeight;
	desc.Format = FORMAT_R32G32B32A32_FLOAT;
	desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	desc.Usage = USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.Depth = 1;

	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &traceResult);


	rtAccumulation.Initialize(
		wiRenderer::GetInternalResolution().x, wiRenderer::GetInternalResolution().y
		, false, FORMAT_R32G32B32A32_FLOAT); // needs full float for correct accumulation over long time period!



	// also reset accumulation buffer state:
	sam = -1;
}

void PathTracingRenderableComponent::Initialize()
{
	ResizeBuffers();

	Renderable3DComponent::Initialize();
}
void PathTracingRenderableComponent::Load()
{
	Renderable3DComponent::Load();
}
void PathTracingRenderableComponent::Start()
{
	Renderable3DComponent::Start();
}
void PathTracingRenderableComponent::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);

	Renderable2DComponent::Render();
}


void PathTracingRenderableComponent::Update(float dt)
{
	if (wiRenderer::getCamera()->hasChanged)
	{
		sam = -1;
	}
	sam++;

	Renderable3DComponent::Update(dt);
}


void PathTracingRenderableComponent::RenderFrameSetUp(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateRenderData(threadID);

	if (sam == 0)
	{
		wiRenderer::BuildSceneBVH(threadID);
	}
}

void PathTracingRenderableComponent::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::GetInstance().BeginRange("Traced Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::getCamera(), threadID);

	wiRenderer::DrawTracedScene(wiRenderer::getCamera(), traceResult, threadID);




	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.hdr = true;


	// Accumulate with moving averaged blending:
	fx.opacity = 1.0f / (sam + 1.0f);
	fx.blendFlag = BLENDMODE_ALPHA;

	rtAccumulation.Set(threadID);
	wiImage::Draw(traceResult, fx, threadID);


	wiProfiler::GetInstance().EndRange(threadID); // Traced Scene
}

void PathTracingRenderableComponent::Compose()
{
	wiRenderer::GetDevice()->EventBegin("PathTracingRenderableComponent::Compose", GRAPHICSTHREAD_IMMEDIATE);


	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_BILINEAR;
	fx.process.setToneMap(true);
	fx.setDistortionMap(wiTextureHelper::getInstance()->getBlack()); // tonemap shader uses signed distortion mask, so black = no distortion
	fx.setMaskMap(wiTextureHelper::getInstance()->getColor(wiColor::Gray));
	
	wiImage::Draw(rtAccumulation.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);


	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

	Renderable2DComponent::Compose();
}


