#include "RenderPath2D.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

using namespace wiGraphics;

RenderPath2D::RenderPath2D()
{
	addLayer(DEFAULT_RENDERLAYER);
}

void RenderPath2D::ResizeBuffers()
{
	RenderPath::ResizeBuffers();

	GraphicsDevice* device = wiRenderer::GetDevice();

	wiRenderer::GetDevice()->WaitForGPU();

	FORMAT defaultTextureFormat = device->GetBackBufferFormat();

	if(GetDepthStencil() != nullptr && wiRenderer::GetResolutionScale() != 1.0f)
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = wiRenderer::GetInternalResolution().x;
		desc.Height = wiRenderer::GetInternalResolution().y;
		device->CreateTexture2D(&desc, nullptr, &rtStenciled);
		device->SetName(&rtStenciled, "rtStenciled");
	}
	{
		TextureDesc desc;
		desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
		desc.Format = defaultTextureFormat;
		desc.Width = device->GetScreenWidth();
		desc.Height = device->GetScreenHeight();
		device->CreateTexture2D(&desc, nullptr, &rtFinal);
		device->SetName(&rtFinal, "rtFinal");
	}

}

void RenderPath2D::Initialize()
{
	RenderPath::Initialize();
}

void RenderPath2D::Load()
{
	RenderPath::Load();
}
void RenderPath2D::Unload()
{
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->CleanUp();
				delete y.sprite;
			}
			if (y.font != nullptr)
			{
				y.font->CleanUp();
				delete y.font;
			}
		}
	}

	RenderPath::Unload();
}
void RenderPath2D::Start()
{
	RenderPath::Start();
}
void RenderPath2D::Update(float dt)
{
	GetGUI().Update(dt);

	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Update(dt * getSpriteSpeed());
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
				y.sprite->FixedUpdate(getSpriteSpeed());
			}
		}
	}

	RenderPath::FixedUpdate();
}
void RenderPath2D::Render() const
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	const Texture2D* dsv = GetDepthStencil();

	// Special care for internal resolution, because stencil buffer is of internal resolution, 
	//	so we might need to render stencil sprites to separate render target that matches internal resolution!
	if (GetDepthStencil() != nullptr && wiRenderer::GetResolutionScale() != 1.0f)
	{
		const Texture2D* rts[] = { &rtStenciled };
		device->BindRenderTargets(ARRAYSIZE(rts), rts, dsv, GRAPHICSTHREAD_IMMEDIATE);

		float clear[] = { 0,0,0,0 };
		device->ClearRenderTarget(rts[0], clear, GRAPHICSTHREAD_IMMEDIATE);

		ViewPort vp;
		vp.Width = (float)rtStenciled.GetDesc().Width;
		vp.Height = (float)rtStenciled.GetDesc().Height;
		device->BindViewports(1, &vp, GRAPHICSTHREAD_IMMEDIATE);

		wiRenderer::GetDevice()->EventBegin("STENCIL Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.sprite != nullptr && y.sprite->params.stencilComp != STENCILMODE_DISABLED)
				{
					y.sprite->Draw(GRAPHICSTHREAD_IMMEDIATE);
				}
			}
		}
		wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

		dsv = nullptr;
	}


	const Texture2D* rts[] = { &rtFinal };
	device->BindRenderTargets(ARRAYSIZE(rts), rts, dsv, GRAPHICSTHREAD_IMMEDIATE);

	float clear[] = { 0,0,0,0 };
	device->ClearRenderTarget(rts[0], clear, GRAPHICSTHREAD_IMMEDIATE);

	ViewPort vp;
	vp.Width = (float)rtFinal.GetDesc().Width;
	vp.Height = (float)rtFinal.GetDesc().Height;
	device->BindViewports(1, &vp, GRAPHICSTHREAD_IMMEDIATE);

	if (GetDepthStencil() != nullptr)
	{
		if (wiRenderer::GetResolutionScale() != 1.0f)
		{
			wiRenderer::GetDevice()->EventBegin("Copy STENCIL Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
			wiImageParams fx;
			fx.enableFullScreen();
			wiImage::Draw(&rtStenciled, fx, GRAPHICSTHREAD_IMMEDIATE);
			wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
		}
		else
		{
			wiRenderer::GetDevice()->EventBegin("STENCIL Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
			for (auto& x : layers)
			{
				for (auto& y : x.items)
				{
					if (y.sprite != nullptr && y.sprite->params.stencilComp != STENCILMODE_DISABLED)
					{
						y.sprite->Draw(GRAPHICSTHREAD_IMMEDIATE);
					}
				}
			}
			wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);
		}
	}

	wiRenderer::GetDevice()->EventBegin("Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr && y.sprite->params.stencilComp == STENCILMODE_DISABLED)
			{
				y.sprite->Draw(GRAPHICSTHREAD_IMMEDIATE);
			}
			if (y.font != nullptr)
			{
				y.font->Draw(GRAPHICSTHREAD_IMMEDIATE);
			}
		}
	}
	wiRenderer::GetDevice()->EventEnd(GRAPHICSTHREAD_IMMEDIATE);

	GetGUI().Render();

	RenderPath::Render();
}
void RenderPath2D::Compose() const
{
	wiImageParams fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.enableFullScreen();
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;

	wiImage::Draw(&rtFinal, fx, GRAPHICSTHREAD_IMMEDIATE);

	RenderPath::Compose();
}


void RenderPath2D::addSprite(wiSprite* sprite, const std::string& layer)
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
void RenderPath2D::removeSprite(wiSprite* sprite)
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
void RenderPath2D::clearSprites()
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
int RenderPath2D::getSpriteOrder(wiSprite* sprite)
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

void RenderPath2D::addFont(wiFont* font, const std::string& layer)
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
void RenderPath2D::removeFont(wiFont* font)
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
void RenderPath2D::clearFonts()
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
int RenderPath2D::getFontOrder(wiFont* font)
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


void RenderPath2D::addLayer(const std::string& name)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
			return;
	}
	RenderLayer2D layer = RenderLayer2D(name);
	layer.order = (int)layers.size();
	layers.push_back(layer);
	layers.back().items.clear();
}
void RenderPath2D::setLayerOrder(const std::string& name, int order)
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
void RenderPath2D::SetFontOrder(wiFont* font, int order)
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

