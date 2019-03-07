#include "wiDepthTarget.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

void wiDepthTarget::Initialize(int width, int height, UINT MSAAC)
{
	dirty = true;

	TextureDesc desc;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = wiRenderer::DSFormat_full_alias;
	desc.SampleDesc.Count = MSAAC;
	// depthDesc.SampleDesc.Quality = 0; // auto-filll in device
	desc.Usage = USAGE_DEFAULT;
	desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &target_primary);

	if (MSAAC > 1)
	{
		desc.SampleDesc.Count = 1;
		desc.Format = wiRenderer::RTFormat_depthresolve;
		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &target_resolved);
	}
}
void wiDepthTarget::InitializeCube(int size, bool independentFaces)
{
	dirty = true;

	TextureDesc desc;
	desc.Width = size;
	desc.Height = size;
	desc.MipLevels = 1;
	desc.ArraySize = 6;
	desc.Format = wiRenderer::DSFormat_small_alias;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = USAGE_DEFAULT;
	desc.BindFlags = BIND_DEPTH_STENCIL | BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

	target_primary.RequestIndependentRenderTargetCubemapFaces(independentFaces);
	wiRenderer::GetDevice()->CreateTexture2D(&desc, nullptr, &target_primary);
}

void wiDepthTarget::Clear(GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->ClearDepthStencil(&GetTexture(), CLEAR_DEPTH | CLEAR_STENCIL, 0.0f, 0, threadID);
	dirty = true;
}
void wiDepthTarget::CopyFrom(const wiDepthTarget& from, GRAPHICSTHREAD threadID)
{
	wiRenderer::GetDevice()->CopyTexture2D(&GetTexture(), &from.GetTexture(), threadID);
	dirty = true;
}

const Texture2D& wiDepthTarget::GetTextureResolvedMSAA(GRAPHICSTHREAD threadID)
{
	if (GetDesc().SampleDesc.Count > 1)
	{
		if (dirty)
		{
			wiRenderer::ResolveMSAADepthBuffer(&target_resolved, &target_primary, threadID);
			dirty = false;
		}
		return target_resolved;
	}
	return target_primary;
}
