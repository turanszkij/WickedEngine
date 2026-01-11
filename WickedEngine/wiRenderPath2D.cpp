#include "wiRenderPath2D.h"
#include "wiResourceManager.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiRenderer.h"

using namespace wi::graphics;

namespace wi
{
	void RenderPath2D::DeleteGPUResources()
	{
		current_buffersize = {};
		current_layoutscale = 0; // invalidate layout

		rtFinal = {};
		rtFinal_MSAA = {};
		rtStencilExtracted = {};
		stencilScaled = {};
	}

	void RenderPath2D::ResizeBuffers()
	{
		current_buffersize = GetInternalResolution();
		current_layoutscale = 0; // invalidate layout

		GraphicsDevice* device = wi::graphics::GetDevice();

		const uint32_t sampleCount = std::max(getMSAASampleCount(), getMSAASampleCount2D());

		const Texture* dsv = GetDepthStencil();
		if (dsv != nullptr && (resolutionScale != 1.0f || dsv->GetDesc().sample_count != sampleCount))
		{
			TextureDesc desc = GetDepthStencil()->GetDesc();
			desc.layout = ResourceState::SHADER_RESOURCE;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R8_UINT;
			desc.layout = ResourceState::SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &rtStencilExtracted);
			device->SetName(&rtStencilExtracted, "rtStencilExtracted");

			desc.width = GetPhysicalWidth();
			desc.height = GetPhysicalHeight();
			desc.sample_count = sampleCount;
			desc.bind_flags = BindFlag::DEPTH_STENCIL;
			desc.format = Format::D32_FLOAT_S8X24_UINT; // D24 was erroring on Vulkan AMD though here it would be enough
			desc.layout = ResourceState::DEPTHSTENCIL;
			device->CreateTexture(&desc, nullptr, &stencilScaled);
			device->SetName(&stencilScaled, "stencilScaled");
		}
		else
		{
			rtStencilExtracted = {};
			stencilScaled = {};
		}

		{
			TextureDesc desc;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.width = GetPhysicalWidth();
			desc.height = GetPhysicalHeight();
			device->CreateTexture(&desc, nullptr, &rtFinal);
			device->SetName(&rtFinal, "rtFinal");

			if (sampleCount > 1)
			{
				desc.sample_count = sampleCount;
				desc.bind_flags = BindFlag::RENDER_TARGET;
				desc.misc_flags = ResourceMiscFlag::TRANSIENT_ATTACHMENT;
				desc.layout = ResourceState::RENDERTARGET;
				device->CreateTexture(&desc, nullptr, &rtFinal_MSAA);
				device->SetName(&rtFinal_MSAA, "rtFinal_MSAA");
			}
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

		video_decodes.clear();
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.sprite != nullptr)
				{
					y.sprite->Update(dt);
				}
				if (y.font != nullptr)
				{
					y.font->Update(dt);
				}
				if (y.videoinstance != nullptr)
				{
					wi::video::UpdateVideo(y.videoinstance, dt);
					if (wi::video::IsDecodingRequired(y.videoinstance))
					{
						video_decodes.push_back(y.videoinstance);
					}
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
				if (y.font != nullptr)
				{
					y.font->FixedUpdate();
				}
			}
		}

		RenderPath::FixedUpdate();
	}
	void RenderPath2D::PreRender()
	{
		XMUINT2 internalResolution = GetInternalResolution();
		if (current_buffersize.x != internalResolution.x || current_buffersize.y != internalResolution.y)
		{
			ResizeBuffers();
		}

		RenderPath::PreRender();
	}
	void RenderPath2D::Render() const
	{
		if (!rtFinal.IsValid())
		{
			// Since 0.71.694: PreRender must be called before Render() because it sets up rendering resources!
			//	The proper fix is to call PreRender() yourself for a manually managed RenderPath3D
			//	But if you don't do that, as a last resort it will be called here using const_cast
			assert(0);
			const_cast<RenderPath2D*>(this)->PreRender();
		}

		GraphicsDevice* device = wi::graphics::GetDevice();

		CommandList video_cmd;
		if (!video_decodes.empty())
		{
			video_cmd = device->BeginCommandList(QUEUE_VIDEO_DECODE);
			device->EventBegin("RenderPath2D video decoding", video_cmd);
			for (auto& x : video_decodes)
			{
				wi::video::DecodeVideo(x, video_cmd);
			}
			device->EventEnd(video_cmd);
		}

		CommandList cmd = device->BeginCommandList();
		device->EventBegin("RenderPath2D::Render", cmd);
		wi::image::SetCanvas(*this);
		wi::font::SetCanvas(*this);

		if (!video_decodes.empty())
		{
			device->WaitCommandList(cmd, video_cmd);
			device->EventBegin("RenderPath2D video resolving", cmd);
			for (auto& x : video_decodes)
			{
				wi::video::ResolveVideoToRGB(x, cmd);
			}
			device->EventEnd(cmd);
		}

		wi::renderer::ProcessDeferredTextureRequests(cmd);

		if (GetGUIBlurredBackground() != nullptr)
		{
			wi::image::SetBackground(*GetGUIBlurredBackground());
		}

		const Texture* dsv = GetDepthStencil();

		if (rtStencilExtracted.IsValid())
		{
			wi::renderer::ExtractStencil(*dsv, rtStencilExtracted, cmd);
		}

		RenderPassImage rp[4];
		uint32_t rp_count = 0;
		if (rtFinal_MSAA.IsValid())
		{
			// MSAA:
			rp[rp_count++] = RenderPassImage::RenderTarget(
				&rtFinal_MSAA,
				RenderPassImage::LoadOp::CLEAR,
				RenderPassImage::StoreOp::DONTCARE,
				ResourceState::RENDERTARGET,
				ResourceState::RENDERTARGET
			);
			rp[rp_count++] = RenderPassImage::Resolve(&rtFinal);
		}
		else
		{
			// Single sample:
			rp[rp_count++] = RenderPassImage::RenderTarget(&rtFinal, RenderPassImage::LoadOp::CLEAR);
		}
		if (stencilScaled.IsValid())
		{
			// Scaled stencil:
			rp[rp_count++] = RenderPassImage::DepthStencil(&stencilScaled, RenderPassImage::LoadOp::CLEAR, RenderPassImage::StoreOp::DONTCARE);
		}
		else if (dsv != nullptr)
		{
			// Native stencil:
			rp[rp_count++] = RenderPassImage::DepthStencil(dsv, RenderPassImage::LoadOp::LOAD, RenderPassImage::StoreOp::STORE);
		}
		device->RenderPassBegin(rp, rp_count, cmd);

		Viewport vp;
		vp.width = (float)rtFinal.GetDesc().width;
		vp.height = (float)rtFinal.GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		wi::graphics::Rect rect;
		rect.left = 0;
		rect.right = (int32_t)rtFinal.GetDesc().width;
		rect.top = 0;
		rect.bottom = (int32_t)rtFinal.GetDesc().height;
		device->BindScissorRects(1, &rect, cmd);

		bool stencil_scaled = false;

		device->EventBegin("Layers", cmd);
		for (auto& x : layers)
		{
			for (auto& y : x.items)
			{
				if (y.sprite != nullptr)
				{
					if (!stencil_scaled && y.sprite->params.stencilComp != wi::image::STENCILMODE_DISABLED && stencilScaled.IsValid())
					{
						// Only need a scaled stencil mask if there are any stenciled sprites, and only once is enough before the first one
						stencil_scaled = true;
						wi::renderer::ScaleStencilMask(vp, rtStencilExtracted, cmd);
					}
					if (y.videoinstance != nullptr)
					{
						y.sprite->textureResource.SetTexture(y.videoinstance->GetCurrentFrameTexture());
					}
					y.sprite->Draw(cmd);
				}
				if (y.font != nullptr)
				{
					y.font->Draw(cmd);
				}
			}
		}
		device->EventEnd(cmd);

		GetGUI().Render(*this, cmd);

		device->RenderPassEnd(cmd);

		device->EventEnd(cmd);

		RenderPath::Render();
	}
	void RenderPath2D::Compose(CommandList cmd) const
	{
		GraphicsDevice* device = wi::graphics::GetDevice();
		device->EventBegin("RenderPath2D::Compose", cmd);

		wi::image::Params fx;
		fx.enableFullScreen();
		fx.blendFlag = wi::enums::BLENDMODE_PREMULTIPLIED;
		if (colorspace != ColorSpace::SRGB)
		{
			// Convert the regular SRGB result of the render path to linear space for HDR compositing:
			fx.enableLinearOutputMapping(hdr_scaling);
		}
		wi::image::Draw(&GetRenderResult(), fx, cmd);

		device->EventEnd(cmd);

		RenderPath::Compose(cmd);
	}


	void RenderPath2D::AddSprite(wi::Sprite* sprite, const std::string& layer)
	{
		for (auto& x : layers)
		{
			if (x.name == layer)
			{
				x.items.emplace_back().sprite = sprite;
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
				if (y.sprite == sprite)
				{
					y.sprite = nullptr;
					y.videoinstance = nullptr;
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
				y.videoinstance = nullptr;
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
			if (x.name == layer)
			{
				x.items.emplace_back().font = font;
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

	void RenderPath2D::AddVideoSprite(wi::video::VideoInstance* videoinstance, wi::Sprite* sprite, const std::string& layer)
	{
		for (auto& x : layers)
		{
			if (x.name == layer)
			{
				RenderItem2D& item = x.items.emplace_back();
				item.sprite = sprite;
				item.videoinstance = videoinstance;
			}
		}
		SortLayers();
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
				if (y.sprite == sprite)
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
				if (y.font == font)
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
				if (y.sprite != nullptr || y.font != nullptr || y.videoinstance != nullptr)
				{
					itemsToRetain.push_back(y);
				}
			}
			x.items.clear();
			x.items = itemsToRetain;
		}
	}

}
