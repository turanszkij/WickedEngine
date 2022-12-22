#pragma once
#include "wiRenderPath.h"
#include "wiGUI.h"
#include "wiVector.h"

#include <string>

namespace wi
{
	class Sprite;
	class SpriteFont;

	class RenderPath2D :
		public RenderPath
	{
	private:
		wi::graphics::Texture rtStenciled;
		wi::graphics::Texture rtStenciled_resolved;
		wi::graphics::Texture rtFinal;

		wi::gui::GUI GUI;

		XMUINT2 current_buffersize{};
		float current_layoutscale{};

		float hdr_scaling = 9.0f;

	public:
		// create resolution dependent resources, such as render targets
		virtual void ResizeBuffers();
		// update DPI dependent elements, such as GUI elements, sprites
		virtual void ResizeLayout();

		void Update(float dt) override;
		void FixedUpdate() override;
		void Render() const override;
		void Compose(wi::graphics::CommandList cmd) const override;

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

		struct RenderItem2D
		{
			enum class TYPE
			{
				SPRITE,
				FONT,
			} type = TYPE::SPRITE;
			union
			{
				wi::Sprite* sprite = nullptr;
				wi::SpriteFont* font;
			};
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
	};

}
