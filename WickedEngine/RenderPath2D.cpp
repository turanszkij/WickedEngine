#include "RenderPath2D.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

RenderPath2D::RenderPath2D() : RenderPath()
{
	addLayer(DEFAULT_RENDERLAYER);
	GUI = wiGUI(GRAPHICSTHREAD_IMMEDIATE);
}


RenderPath2D::~RenderPath2D()
{
	Unload();
}

wiRenderTarget RenderPath2D::rtFinal;
void RenderPath2D::ResizeBuffers()
{
	wiRenderer::GetDevice()->WaitForGPU();

	FORMAT defaultTextureFormat = wiRenderer::GetDevice()->GetBackBufferFormat();

	// Protect against multiple buffer resizes when there is no change!
	static UINT lastBufferResWidth = 0, lastBufferResHeight = 0, lastBufferMSAA = 0;
	static FORMAT lastBufferFormat = FORMAT_UNKNOWN;
	if (lastBufferResWidth == (UINT)wiRenderer::GetDevice()->GetScreenWidth() &&
		lastBufferResHeight == (UINT)wiRenderer::GetDevice()->GetScreenHeight() &&
		lastBufferFormat == defaultTextureFormat)
	{
		return;
	}
	else
	{
		lastBufferResWidth = (UINT)wiRenderer::GetDevice()->GetScreenWidth();
		lastBufferResHeight = (UINT)wiRenderer::GetDevice()->GetScreenHeight();
		lastBufferFormat = defaultTextureFormat;
	}

	rtFinal.Initialize(wiRenderer::GetDevice()->GetScreenWidth(), wiRenderer::GetDevice()->GetScreenHeight(), false, defaultTextureFormat);
}

void RenderPath2D::Initialize()
{
	ResizeBuffers();

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
void RenderPath2D::Render()
{
	rtFinal.Activate(GRAPHICSTHREAD_IMMEDIATE, 0.0f, 0.0f, 0.0f, 0.0f);

	wiRenderer::GetDevice()->EventBegin("Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
	for (auto& x : layers)
	{
		for (auto& y : x.items)
		{
			if (y.sprite != nullptr)
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
void RenderPath2D::Compose()
{
	wiImageParams fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.enableFullScreen();
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;

	wiImage::Draw(rtFinal.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);

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

