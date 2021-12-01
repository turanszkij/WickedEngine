#include "wiRenderPath2D.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiRenderer.h"

using namespace wi::graphics;

namespace wi
{

	void RenderPath2D::ResizeBuffers()
	{
		current_buffersize = GetInternalResolution();
		current_layoutscale = 0; // invalidate layout

		GraphicsDevice* device = wi::graphics::GetDevice();

		const Texture* dsv = GetDepthStencil();
		if (dsv != nullptr && (resolutionScale != 1.0f || dsv->GetDesc().sample_count > 1))
		{
			TextureDesc desc = GetDepthStencil()->GetDesc();
			desc.layout = ResourceState::SHADER_RESOURCE;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R8G8B8A8_UNORM;
			device->CreateTexture(&desc, nullptr, &rtStenciled);
			device->SetName(&rtStenciled, "rtStenciled");

			if (desc.sample_count > 1)
			{
				desc.sample_count = 1;
				device->CreateTexture(&desc, nullptr, &rtStenciled_resolved);
				device->SetName(&rtStenciled_resolved, "rtStenciled_resolved");
			}
		}
		else
		{
			rtStenciled = Texture(); // this will be deleted here
		}

		{
			TextureDesc desc;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.width = GetPhysicalWidth();
			desc.height = GetPhysicalHeight();
			device->CreateTexture(&desc, nullptr, &rtFinal);
			device->SetName(&rtFinal, "rtFinal");
		}

		if (rtStenciled.IsValid())
		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtStenciled, RenderPassAttachment::LoadOp::CLEAR));
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					dsv,
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);

			if (rtStenciled.GetDesc().sample_count > 1)
			{
				desc.attachments.push_back(RenderPassAttachment::Resolve(&rtStenciled_resolved));
			}

			device->CreateRenderPass(&desc, &renderpass_stenciled);

			dsv = nullptr;
		}
		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtFinal, RenderPassAttachment::LoadOp::CLEAR));

			if (dsv != nullptr && !rtStenciled.IsValid())
			{
				desc.attachments.push_back(
					RenderPassAttachment::DepthStencil(
						dsv,
						RenderPassAttachment::LoadOp::LOAD,
						RenderPassAttachment::StoreOp::STORE,
						ResourceState::DEPTHSTENCIL_READONLY,
						ResourceState::DEPTHSTENCIL_READONLY,
						ResourceState::DEPTHSTENCIL_READONLY
					)
				);
			}

			device->CreateRenderPass(&desc, &renderpass_final);
		}

	}
	void RenderPath2D::ResizeLayout()
	{
		current_layoutscale = GetDPIScaling();
	}

	void RenderPath2D::Update(float dt)
	{
		XMUINT2 internalResolution = GetInternalResolution();

		if (current_buffersize.x != internalResolution.x || current_buffersize.y != internalResolution.y)
		{
			ResizeBuffers();
		}
		if (current_layoutscale != GetDPIScaling())
		{
			ResizeLayout();
		}

		GetGUI().Update(*this, dt);

		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				switch (y.type)
				{
				default:
				case RenderItem2D::SPRITE:
					if (y.sprite != nullptr)
					{
						y.sprite->Update(dt);
					}
					break;
				case RenderItem2D::FONT:
					if (y.font != nullptr)
					{
						y.font->Update(dt);
					}
					break;
				}
			}
		}

		if (colorspace != ColorSpace::SRGB && (rtLinearColorSpace.desc.width != rtFinal.desc.width || rtLinearColorSpace.desc.height != rtFinal.desc.height))
		{
			TextureDesc desc = rtFinal.desc;
			desc.format = Format::R16G16B16A16_FLOAT;
			bool success = wi::graphics::GetDevice()->CreateTexture(&desc, nullptr, &rtLinearColorSpace);
			assert(success);
			wi::graphics::GetDevice()->SetName(&rtLinearColorSpace, "rtLinearColorSpace");

			RenderPassDesc renderpassdesc;
			renderpassdesc.attachments.push_back(RenderPassAttachment::RenderTarget(&rtLinearColorSpace, RenderPassAttachment::LoadOp::CLEAR));
			success = wi::graphics::GetDevice()->CreateRenderPass(&renderpassdesc, &renderpass_linearize);
			assert(success);
		}

		RenderPath::Update(dt);
	}
	void RenderPath2D::FixedUpdate()
	{
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				switch (y.type)
				{
				default:
				case RenderItem2D::SPRITE:
					if (y.sprite != nullptr)
					{
						y.sprite->FixedUpdate();
					}
					break;
				case RenderItem2D::FONT:
					if (y.font != nullptr)
					{
						y.font->FixedUpdate();
					}
					break;
				}
			}
		}

		RenderPath::FixedUpdate();
	}
	void RenderPath2D::Render() const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();
		wi::image::SetCanvas(*this, cmd);
		wi::font::SetCanvas(*this, cmd);

		wi::renderer::ProcessDeferredMipGenRequests(cmd);

		if (GetGUIBlurredBackground() != nullptr)
		{
			wi::image::SetBackground(*GetGUIBlurredBackground(), cmd);
		}

		// Special care for internal resolution, because stencil buffer is of internal resolution, 
		//	so we might need to render stencil sprites to separate render target that matches internal resolution!
		if (rtStenciled.IsValid())
		{
			device->RenderPassBegin(&renderpass_stenciled, cmd);

			Viewport vp;
			vp.width = (float)rtStenciled.GetDesc().width;
			vp.height = (float)rtStenciled.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			device->EventBegin("STENCIL Sprite Layers", cmd);
			for (auto& x : layers)
			{
				for (auto& y : x.items)
				{
					if (y.type == RenderItem2D::SPRITE &&
						y.sprite != nullptr &&
						y.sprite->params.stencilComp != wi::image::STENCILMODE_DISABLED)
					{
						y.sprite->Draw(cmd);
					}
				}
			}
			device->EventEnd(cmd);

			device->RenderPassEnd(cmd);
		}

		device->RenderPassBegin(&renderpass_final, cmd);

		Viewport vp;
		vp.width = (float)rtFinal.GetDesc().width;
		vp.height = (float)rtFinal.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		if (GetDepthStencil() != nullptr)
		{
			if (rtStenciled.IsValid())
			{
				device->EventBegin("Copy STENCIL Sprite Layers", cmd);
				wi::image::Params fx;
				fx.enableFullScreen();
				if (rtStenciled.GetDesc().sample_count > 1)
				{
					wi::image::Draw(&rtStenciled_resolved, fx, cmd);
				}
				else
				{
					wi::image::Draw(&rtStenciled, fx, cmd);
				}
				device->EventEnd(cmd);
			}
			else
			{
				device->EventBegin("STENCIL Sprite Layers", cmd);
				for (auto& x : layers)
				{
					for (auto& y : x.items)
					{
						if (y.type == RenderItem2D::SPRITE &&
							y.sprite != nullptr &&
							y.sprite->params.stencilComp != wi::image::STENCILMODE_DISABLED)
						{
							y.sprite->Draw(cmd);
						}
					}
				}
				device->EventEnd(cmd);
			}
		}

		device->EventBegin("Sprite Layers", cmd);
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				switch (y.type)
				{
				default:
				case RenderItem2D::SPRITE:
					if (y.sprite != nullptr && y.sprite->params.stencilComp == wi::image::STENCILMODE_DISABLED)
					{
						y.sprite->Draw(cmd);
					}
					break;
				case RenderItem2D::FONT:
					if (y.font != nullptr)
					{
						y.font->Draw(cmd);
					}
					break;
				}
			}
		}
		device->EventEnd(cmd);

		GetGUI().Render(*this, cmd);

		device->RenderPassEnd(cmd);

		if (colorspace != ColorSpace::SRGB)
		{
			// Convert the regular SRGB result of the render path to linear space for HDR compositing:
			device->RenderPassBegin(&renderpass_linearize, cmd);
			wi::image::Params fx;
			fx.enableFullScreen();
			fx.enableLinearOutputMapping(hdr_scaling);
			wi::image::Draw(&rtFinal, fx, cmd);
			device->RenderPassEnd(cmd);
			render_result = rtLinearColorSpace;
		}
		else
		{
			render_result = rtFinal;
		}

		RenderPath::Render();
	}
	void RenderPath2D::Compose(CommandList cmd) const
	{
		wi::image::Params fx;
		fx.enableFullScreen();
		fx.blendFlag = wi::enums::BLENDMODE_PREMULTIPLIED;
		wi::image::Draw(&render_result, fx, cmd);

		RenderPath::Compose(cmd);
	}


	void RenderPath2D::AddSprite(wi::Sprite* sprite, const std::string& layer)
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
	void RenderPath2D::RemoveSprite(wi::Sprite* sprite)
	{
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.type == RenderItem2D::SPRITE && y.sprite == sprite)
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
				if (y.type == RenderItem2D::SPRITE)
				{
					y.sprite = nullptr;
				}
			}
		}
		CleanLayers();
	}
	int RenderPath2D::GetSpriteOrder(wi::Sprite* sprite)
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

	void RenderPath2D::AddFont(wi::SpriteFont* font, const std::string& layer)
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
	void RenderPath2D::RemoveFont(wi::SpriteFont* font)
	{
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.type == RenderItem2D::FONT && y.font == font)
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
				if (y.type == RenderItem2D::FONT)
				{
					y.font = nullptr;
				}
			}
		}
		CleanLayers();
	}
	int RenderPath2D::GetFontOrder(wi::SpriteFont* font)
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
	void RenderPath2D::SetSpriteOrder(wi::Sprite* sprite, int order)
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
	void RenderPath2D::SetFontOrder(wi::SpriteFont* font, int order)
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
			wi::vector<RenderItem2D> itemsToRetain(0);
			for (auto& y : x.items)
			{
				if (y.sprite != nullptr || y.font != nullptr)
				{
					itemsToRetain.push_back(y);
				}
			}
			x.items.clear();
			x.items = itemsToRetain;
		}
	}

}
