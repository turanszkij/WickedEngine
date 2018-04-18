#include "Renderable2DComponent.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiFont.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

Renderable2DComponent::Renderable2DComponent() : RenderableComponent()
{
	setSpriteSpeed(1.f);
	addLayer(DEFAULT_RENDERLAYER);
	GUI = wiGUI(GRAPHICSTHREAD_IMMEDIATE);
}


Renderable2DComponent::~Renderable2DComponent()
{
	Unload();
}

wiRenderTarget Renderable2DComponent::rtFinal;
void Renderable2DComponent::ResizeBuffers()
{
	FORMAT defaultTextureFormat = GraphicsDevice::GetBackBufferFormat();

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

void Renderable2DComponent::Initialize()
{
	ResizeBuffers();

	RenderableComponent::Initialize();
}

void Renderable2DComponent::Load()
{
	RenderableComponent::Load();
}
void Renderable2DComponent::Unload()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
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

	RenderableComponent::Unload();
}
void Renderable2DComponent::Start()
{
	RenderableComponent::Start();
}
void Renderable2DComponent::Update(float dt)
{
	GetGUI().Update(dt);

	RenderableComponent::Update(dt);
}
void Renderable2DComponent::FixedUpdate()
{
	
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr)
			{
				y.sprite->Update(getSpriteSpeed());
			}
			if (y.font != nullptr)
			{
				// this is intentianally left blank
			}
		}
	}

	RenderableComponent::FixedUpdate();
}
void Renderable2DComponent::Render()
{
	rtFinal.Activate(GRAPHICSTHREAD_IMMEDIATE, 0.0f, 0.0f, 0.0f, 0.0f);

	wiRenderer::GetDevice()->EventBegin("Sprite Layers", GRAPHICSTHREAD_IMMEDIATE);
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
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

	RenderableComponent::Render();
}
void Renderable2DComponent::Compose()
{
	wiImageEffects fx((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight());
	fx.presentFullScreen = true;
	fx.blendFlag = BLENDMODE_PREMULTIPLIED;

	GPUResource* res[] = {
		rtFinal.GetTexture()
	};
	wiRenderer::GetDevice()->TransitionBarrier(res, 1, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PIXEL_SHADER_RESOURCE, GRAPHICSTHREAD_IMMEDIATE);

	wiImage::Draw(rtFinal.GetTexture(), fx, GRAPHICSTHREAD_IMMEDIATE);

	RenderableComponent::Compose();
}


void Renderable2DComponent::addSprite(wiSprite* sprite, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			LayeredRenderEntity entity = LayeredRenderEntity();
			entity.type = LayeredRenderEntity::SPRITE;
			entity.sprite = sprite;
			x.entities.push_back(entity);
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeSprite(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite == sprite)
			{
				y.sprite = nullptr;
			}
		}
	}
	CleanLayers();
}
void Renderable2DComponent::clearSprites()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			y.sprite = nullptr;
		}
	}
	CleanLayers();
}
int Renderable2DComponent::getSpriteOrder(wiSprite* sprite)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.sprite == sprite)
			{
				return y.order;
			}
		}
	}
	return 0;
}

void Renderable2DComponent::addFont(wiFont* font, const std::string& layer)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(layer))
		{
			LayeredRenderEntity entity = LayeredRenderEntity();
			entity.type = LayeredRenderEntity::FONT;
			entity.font = font;
			x.entities.push_back(entity);
		}
	}
	SortLayers();
}
void Renderable2DComponent::removeFont(wiFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.font == font)
			{
				y.font = nullptr;
			}
		}
	}
	CleanLayers();
}
void Renderable2DComponent::clearFonts()
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			y.font = nullptr;
		}
	}
	CleanLayers();
}
int Renderable2DComponent::getFontOrder(wiFont* font)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.font == font)
			{
				return y.order;
			}
		}
	}
	return 0;
}


void Renderable2DComponent::addLayer(const std::string& name)
{
	for (auto& x : layers)
	{
		if (!x.name.compare(name))
			return;
	}
	RenderLayer layer = RenderLayer(name);
	layer.order = (int)layers.size();
	layers.push_back(layer);
	layers.back().entities.clear();
}
void Renderable2DComponent::setLayerOrder(const std::string& name, int order)
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
void Renderable2DComponent::SetSpriteOrder(wiSprite* sprite, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.type == LayeredRenderEntity::SPRITE && y.sprite == sprite)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void Renderable2DComponent::SetFontOrder(wiFont* font, int order)
{
	for (auto& x : layers)
	{
		for (auto& y : x.entities)
		{
			if (y.type == LayeredRenderEntity::FONT && y.font == font)
			{
				y.order = order;
			}
		}
	}
	SortLayers();
}
void Renderable2DComponent::SortLayers()
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
				RenderLayer swap = layers[i];
				layers[i] = layers[j];
				layers[j] = swap;
			}
		}
	}
	for (auto& x : layers)
	{
		if (x.entities.empty())
		{
			continue;
		}
		for (size_t i = 0; i < x.entities.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < x.entities.size(); ++j)
			{
				if (x.entities[i].order > x.entities[j].order)
				{
					LayeredRenderEntity swap = x.entities[i];
					x.entities[i] = x.entities[j];
					x.entities[j] = swap;
				}
			}
		}
	}
}

void Renderable2DComponent::CleanLayers()
{
	for (auto& x : layers)
	{
		if (x.entities.empty())
		{
			continue;
		}
		std::vector<LayeredRenderEntity> cleanEntities(0);
		for (auto& y : x.entities)
		{
			if (y.sprite != nullptr || y.font!=nullptr)
			{
				cleanEntities.push_back(y);
			}
		}
		x.entities.clear();
		x.entities = cleanEntities;
	}
}

