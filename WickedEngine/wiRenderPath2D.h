#pragma once
#include "wiRenderPath.h"
#include "wiGUI.h"
#include "wiVector.h"
#include "wiVideo.h"

#include <string>

namespace wi
{
	class Sprite;
	class SpriteFont;

	class RenderPath2D : public RenderPath
	{
	protected:
		wi::graphics::Texture rtStencilExtracted;
		wi::graphics::Texture stencilScaled;

		wi::graphics::Texture rtFinal;
		wi::graphics::Texture rtFinal_MSAA;

		wi::gui::GUI GUI;

		XMUINT2 current_buffersize{};
		float current_layoutscale{};

		float hdr_scaling = 9.0f;

		uint32_t msaaSampleCount = 1;
		uint32_t msaaSampleCount2D = 1;

		wi::vector<wi::video::VideoInstance*> video_decodes;

	public:
		// Delete GPU resources and initialize them to default
		virtual void DeleteGPUResources();
		// create resolution dependent resources, such as render targets
		virtual void ResizeBuffers();
		// update DPI dependent elements, such as GUI elements, sprites
		virtual void ResizeLayout();

		void Update(float dt) override;
		void FixedUpdate() override;
		void PreRender() override;
		void Render() const override;
		void Compose(wi::graphics::CommandList cmd) const override;

		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }
		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		virtual void setMSAASampleCount2D(uint32_t value) { msaaSampleCount2D = value; }
		constexpr uint32_t getMSAASampleCount2D() const { return msaaSampleCount2D; }

		const wi::graphics::Texture& GetRenderResult() const { return rtFinal; }
		virtual const wi::graphics::Texture* GetDepthStencil() const { return nullptr; }
		virtual const wi::graphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

		void AddSprite(wi::Sprite* sprite, const std::string& layer = "");
		void RemoveSprite(wi::Sprite* sprite);
		void ClearSprites();
		int GetSpriteOrder(wi::Sprite* sprite);

		void AddFont(wi::SpriteFont* font, const std::string& layer = "");
		void RemoveFont(wi::SpriteFont* font);
		void ClearFonts();
		int GetFontOrder(wi::SpriteFont* font);

		void AddVideoSprite(wi::video::VideoInstance* videoinstance, wi::Sprite* sprite, const std::string& layer = "");

		struct RenderItem2D
		{
			wi::Sprite* sprite = nullptr;
			wi::SpriteFont* font = nullptr;
			wi::video::VideoInstance* videoinstance = nullptr;
			int order = 0;
		};
		struct RenderLayer2D
		{
			wi::vector<RenderItem2D> items;
			std::string name;
			int order = 0;
		};
		wi::vector<RenderLayer2D> layers{ 1 };
		void AddLayer(const std::string& name);
		void SetLayerOrder(const std::string& name, int order);
		void SetSpriteOrder(wi::Sprite* sprite, int order);
		void SetFontOrder(wi::SpriteFont* font, int order);
		void SortLayers();
		void CleanLayers();

		const wi::gui::GUI& GetGUI() const { return GUI; }
		wi::gui::GUI& GetGUI() { return GUI; }

		float resolutionScale = 1.0f;
		XMUINT2 GetInternalResolution() const
		{
			return XMUINT2(
				uint32_t((float)GetPhysicalWidth() * resolutionScale),
				uint32_t((float)GetPhysicalHeight() * resolutionScale)
			);
		}

		float GetHDRScaling() const { return hdr_scaling; }
		void SetHDRScaling(float value) { hdr_scaling = value; }

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr std::string_view script_check_identifier = relative_path(__FILE__);
		virtual std::string_view GetScriptBindingID() const override { return script_check_identifier; }
	};

}
