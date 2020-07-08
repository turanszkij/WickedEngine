#include "RenderPath2D.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiRenderer.h"

using namespace wiGraphics;


void RenderPath2D::ResizeBuffers()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	const Texture* dsv = GetDepthStencil();
	if(dsv != nullptr && (wiRenderer::GetResolutionScale() != 1.0f ||  dsv->GetDesc().SampleCount > 1))
	{
		TextureDesc desc = GetDepthStencil()->GetDesc();
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.layout = IMAGE_LAYOUT_GENERAL;
		device->CreateTexture(&desc, nullptr, &rtStenciled);
		device->SetName(&rtStenciled, "rtStenciled");

		if (desc.SampleCount > 1)
		{
			desc.SampleCount = 1;
			device->CreateTexture(&desc, nullptr, &rtStenciled_resolved);
			device->SetName(&rtStenciled_resolved, "rtStenciled_resolved");
		}
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = device->GetResolutionWidth();
		desc.Height = device->GetResolutionHeight();
		device->CreateTexture(&desc, nullptr, &rtFinal);
		device->SetName(&rtFinal, "rtFinal");
	}

	if (rtStenciled.IsValid())
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtStenciled, RenderPassAttachment::LOADOP_CLEAR));
		desc.attachments.push_back(
			RenderPassAttachment::DepthStencil(
				dsv,
				RenderPassAttachment::LOADOP_LOAD,
				RenderPassAttachment::STOREOP_STORE,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
				IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
			)
		);

		if (rtStenciled.GetDesc().SampleCount > 1)
		{
			desc.attachments.push_back(RenderPassAttachment::Resolve(&rtStenciled_resolved));
		}

		device->CreateRenderPass(&desc, &renderpass_stenciled);

		dsv = nullptr;
	}
	{
		RenderPassDesc desc;
		desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtFinal, RenderPassAttachment::LOADOP_CLEAR));
		
		if(dsv != nullptr && !rtStenciled.IsValid())
		{
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					dsv,
					RenderPassAttachment::LOADOP_LOAD,
					RenderPassAttachment::STOREOP_STORE,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY,
					IMAGE_LAYOUT_DEPTHSTENCIL_READONLY
				)
			);
		}

		device->CreateRenderPass(&desc, &renderpass_final);
	}

}

void RenderPath2D::Load()
{
	// ideally, this would happen here, under loading screen
	if (!resolutionChange_handle.IsValid())
	{
		ResizeBuffers();
		resolutionChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION, [this](uint64_t userdata) {
			ResizeBuffers();
			ResizeLayout();
			});
	}
	if (!resolutionScaleChange_handle.IsValid())
	{
		resolutionScaleChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION_SCALE, [this](uint64_t userdata) {
			ResizeBuffers();
			});
	}
	if (!dpiChange_handle.IsValid())
	{
		ResizeLayout();
		dpiChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_DPI, [this](uint64_t userdata) {
			ResizeLayout();
			});
	}

	RenderPath::Load();
}
void RenderPath2D::Start()
{
	RenderPath::Start();
}
void RenderPath2D::Update(float dt)
{
	// this is last resort, if Load() wasn't called
	if (!resolutionChange_handle.IsValid())
	{
		ResizeBuffers();
		resolutionChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION, [this](uint64_t userdata) {
			ResizeBuffers();
			ResizeLayout();
			});
	}
	if (!resolutionScaleChange_handle.IsValid())
	{
		resolutionScaleChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_RESOLUTION_SCALE, [this](uint64_t userdata) {
			ResizeBuffers();
			});
	}
	if (!dpiChange_handle.IsValid())
	{
		ResizeLayout();
		dpiChange_handle = wiEvent::Subscribe(SYSTEM_EVENT_CHANGE_DPI, [this](uint64_t userdata) {
			ResizeLayout();
			});
	}

	GetGUI().Update(dt);

	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Update(dt);
			}
		}
	}

	RenderPath::Update(dt);
}
void RenderPath2D::FixedUpdate()
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->FixedUpdate();
			}
		}
	}

	RenderPath::FixedUpdate();
}
void RenderPath2D::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();
	CommandList cmd = device->BeginCommandList();

	wiRenderer::GetDevice()->BindResource(PS, GetGUIBlurredBackground(), TEXSLOT_IMAGE_BACKGROUND, cmd);

	// Special care for internal resolution, because stencil buffer is of internal resolution, 
	//	so we might need to render stencil sprites to separate render target that matches internal resolution!
	if (rtStenciled.IsValid())
	{
		device->RenderPassBegin(&renderpass_stenciled, cmd);

		Viewport vp;
		vp.Width = (float)rtStenciled.GetDesc().Width;
		vp.Height = (float)rtStenciled.GetDesc().Height;
		device->BindViewports(1, &vp, cmd);

		wiRenderer::GetDevice()->EventBegin("STENCIL Sprite Layers", cmd);
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.sprite != nullptr && y.sprite->params.stencilComp != STENCILMODE_DISABLED)
				{
					y.sprite->Draw(cmd);
				}
			}
		}
		wiRenderer::GetDevice()->EventEnd(cmd);

		device->RenderPassEnd(cmd);
	}

	device->RenderPassBegin(&renderpass_final, cmd);

	Viewport vp;
	vp.Width = (float)rtFinal.GetDesc().Width;
	vp.Height = (float)rtFinal.GetDesc().Height;
	device->BindViewports(1, &vp, cmd);

	if (GetDepthStencil() != nullptr)
	{
		if (rtStenciled.IsValid())
		{
			wiRenderer::GetDevice()->EventBegin("Copy STENCIL Sprite Layers", cmd);
			wiImageParams fx;
			fx.enableFullScreen();
			if (rtStenciled.GetDesc().SampleCount > 1)
			{
				wiImage::Draw(&rtStenciled_resolved, fx, cmd);
			}
			else
			{
				wiImage::Draw(&rtStenciled, fx, cmd);
			}
			wiRenderer::GetDevice()->EventEnd(cmd);
		}
		else
		{
			wiRenderer::GetDevice()->EventBegin("STENCIL Sprite Layers", cmd);
			for (auto& x : layers)
			{
				for (auto& y : x.items)
				{
					if (y.sprite != nullptr && y.sprite->params.stencilComp != STENCILMODE_DISABLED)
					{
						y.sprite->Draw(cmd);
					}
				}
			}
			wiRenderer::GetDevice()->EventEnd(cmd);
		}
	}

	wiRenderer::GetDevice()->EventBegin("Sprite Layers", cmd);
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr && y.sprite->params.stencilComp == STENCILMODE_DISABLED)
			{
				y.sprite->Draw(cmd);
			}
			if (y.font != nullptr)
			{
				y.font->Draw(cmd);
			}
		}
	}
	wiRenderer::GetDevice()->EventEnd(cmd);

	GetGUI().Render(cmd);

	device->RenderPassEnd(cmd);

	RenderPath::Render();
}
void RenderPath2D::Compose(CommandList cmd) const
{
	wiImageParams fx;
	fx.enableFullScreen();
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;

	wiImage::Draw(&rtFinal, fx, cmd);

	RenderPath::Compose(cmd);
}


void RenderPath2D::AddSprite(wiSprite* sprite, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			x.items.push_back(RenderItem2D());
			x.items.back().type = RenderItem2D::SPRITE;
			x.items.back().sprite = sprite;
		}
	}
	SortLayers();
}
void RenderPath2D::RemoveSprite(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite == sprite)
			{
				y.sprite = nullptr;
			}
		}
	}
	CleanLayers();
}
void RenderPath2D::ClearSprites()
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			y.sprite = nullptr;
		}
	}
	CleanLayers();
}
int RenderPath2D::GetSpriteOrder(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite == sprite)
			{
				return y.order;
			}
		}
	}
	return 0;
}

void RenderPath2D::AddFont(wiSpriteFont* font, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			x.items.push_back(RenderItem2D());
			x.items.back().type = RenderItem2D::FONT;
			x.items.back().font = font;
		}
	}
	SortLayers();
}
void RenderPath2D::RemoveFont(wiSpriteFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.font == font)
			{
				y.font = nullptr;
			}
		}
	}
	CleanLayers();
}
void RenderPath2D::ClearFonts()
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			y.font = nullptr;
		}
	}
	CleanLayers();
}
int RenderPath2D::GetFontOrder(wiSpriteFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.font == font)
			{
				return y.order;
			}
		}
	}
	return 0;
}


void RenderPath2D::AddLayer(const std::string& name)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
			return;
	}
	RenderLayer2D layer;
	layer.name = name;
	layer.order = (int)layers.size();
	layers.push_back(layer);
	layers.back().items.clear();
}
void RenderPath2D::SetLayerOrder(const std::string& name, int order)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
		{
			x.order = order;
			break;
		}
	}
	SortLayers();
}
void RenderPath2D::SetSpriteOrder(wiSprite* sprite, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.type == RenderItem2D::SPRITE && y.sprite == sprite)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void RenderPath2D::SetFontOrder(wiSpriteFont* font, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.type == RenderItem2D::FONT && y.font == font)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void RenderPath2D::SortLayers()
{
	if (layers.empty())
	{
		return;
	}

	for (size_t i = 0; i < layers.size() - 1; ++i)
	{
		for (size_t j = i + 1; j < layers.size(); ++j)
		{
			if (layers[i].order > layers[j].order)
			{
				RenderLayer2D swap = layers[i];
				layers[i] = layers[j];
				layers[j] = swap;
			}
		}
	}
	for (auto& x : layers)
	{
		if (x.items.empty())
		{
			continue;
		}
		for (size_t i = 0; i < x.items.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < x.items.size(); ++j)
			{
				if (x.items[i].order > x.items[j].order)
				{
					RenderItem2D swap = x.items[i];
					x.items[i] = x.items[j];
					x.items[j] = swap;
				}
			}
		}
	}
}

void RenderPath2D::CleanLayers()
{
	for (auto& x : layers)
	{
		if (x.items.empty())
		{
			continue;
		}
		std::vector<RenderItem2D> itemsToRetain(0);
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr || y.font!=nullptr)
			{
				itemsToRetain.push_back(y);
			}
		}
		x.items.clear();
		x.items = itemsToRetain;
	}
}

