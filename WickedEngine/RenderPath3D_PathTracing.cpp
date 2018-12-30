#include "RenderPath3D_PathTracing.h"
#include "wiRenderer.h"
#include "wiImage.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiSprite.h"
#include "ResourceMapping.h"
#include "wiProfiler.h"
#include "wiSceneSystem.h"

using namespace wiGraphicsTypes;
using namespace wiSceneSystem;

RenderPath3D_PathTracing::RenderPath3D_PathTracing()
{
	RenderPath3D::setProperties();
}
RenderPath3D_PathTracing::~RenderPath3D_PathTracing()
{
}


wiGraphicsTypes::Texture2D* RenderPath3D_PathTracing::traceResult = nullptr;
wiRenderTarget RenderPath3D_PathTracing::rtAccumulation;
void RenderPath3D_PathTracing::ResizeBuffers()
{
	RenderPath3D::ResizeBuffers();

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

void RenderPath3D_PathTracing::Initialize()
{
	ResizeBuffers();

	RenderPath3D::Initialize();
}
void RenderPath3D_PathTracing::Load()
{
	RenderPath3D::Load();
}
void RenderPath3D_PathTracing::Start()
{
	RenderPath3D::Start();
}
void RenderPath3D_PathTracing::Render()
{
	RenderFrameSetUp(GRAPHICSTHREAD_IMMEDIATE);
	RenderScene(GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Render();
}


void RenderPath3D_PathTracing::Update(float dt)
{
	const Scene& scene = wiRenderer::GetScene();

	if (wiRenderer::GetCamera().IsDirty())
	{
		sam = -1;
	}
	else
	{
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			const TransformComponent& transform = scene.transforms[i];

			if (transform.IsDirty())
			{
				sam = -1;
				break;
			}
		}
	}
	sam++;

	RenderPath3D::Update(dt);
}


void RenderPath3D_PathTracing::RenderFrameSetUp(GRAPHICSTHREAD threadID)
{
	wiRenderer::UpdateRenderData(threadID);

	if (sam == 0)
	{
		wiRenderer::BuildSceneBVH(threadID);
	}
}

void RenderPath3D_PathTracing::RenderScene(GRAPHICSTHREAD threadID)
{
	wiProfiler::BeginRange("Traced Scene", wiProfiler::DOMAIN_GPU, threadID);

	wiRenderer::UpdateCameraCB(wiRenderer::GetCamera(), threadID);

	wiRenderer::DrawTracedScene(wiRenderer::GetCamera(), traceResult, threadID);




	wiImageParams fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.enableHDR();


	// Accumulate with moving averaged blending:
	fx.opacity = 1.0f / (sam + 1.0f);
	fx.blendFlag = BLENDMODE_ALPHA;

	rtAccumulation.Set(threadID);
	wiImage::Draw(traceResult, fx, threadID);


	wiProfiler::EndRange(threadID); // Traced Scene
}

void RenderPath3D_PathTracing::Compose()
{
	wiRenderer::GetDevice()->EventBegin("RenderPath3D_PathTracing::Compose", GRAPHICSTHREAD_IMMEDIATE);


	wiImageParams fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.blendFlag = BLENDMODE_OPAQUE;
	fx.quality = QUALITY_LINEAR;
	fx.process.setToneMap(getExposure());
	fx.setDistortionMap(wiTextureHelper::getBlack()); // tonemap shader uses signed distortion mask, so black = no distortion
	fx.setMaskMap(wiTextureHelper::getColor(wiColor::Gray()));
	
	wiImage::Draw(rtAccumulation.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);


	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

	RenderPath2D::Compose();
}


