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

		// MSAA sample count of main rendering
		virtual void setMSAASampleCount(uint32_t value) { msaaSampleCount = value; }
		constexpr uint32_t getMSAASampleCount() const { return msaaSampleCount; }

		// MSAA sample count of overlay rendering
		virtual void setMSAASampleCount2D(uint32_t value) { msaaSampleCount2D = value; }
		constexpr uint32_t getMSAASampleCount2D() const { return msaaSampleCount2D; }

		// The 2D scene rendering that is used in Compose() to present the final image. In RenderPath3D this is only the 2D overlay with transparency
		const wi::graphics::Texture& GetRenderResult2D() const { return rtFinal; }

		// The depth stencil is not used in pure 2D rendering, but when it's also a RenderPath3D it is available (not texture compatible, only depth-stencil)
		virtual const wi::graphics::Texture* GetDepthStencil() const { return nullptr; }

		// The blurred GUI background is not available in pure 2D rendering, but other render path can make it available, such as RenderPath3D
		virtual const wi::graphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

		// Adds a sprite to be rendered. The pointer lifetime is managed by the user, it must be valid as long as it is used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
		void AddSprite(wi::Sprite* sprite, const std::string& layer = "");
		void RemoveSprite(wi::Sprite* sprite);
		void ClearSprites();
		int GetSpriteOrder(wi::Sprite* sprite);

		// Adds a SpriteFont to be rendered. The pointer lifetime is managed by the user, it must be valid as long as it is used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
		void AddFont(wi::SpriteFont* font, const std::string& layer = "");
		void RemoveFont(wi::SpriteFont* font);
		void ClearFonts();
		int GetFontOrder(wi::SpriteFont* font);

		// Adds a sprite showing a video, the pointer lifetimes are managed by the user, they must be valid as long as they are used by the RenderPath2D
		//	layer is optional, it can be used to manage overlapping render groups
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

		// Creates a new layer, for managing overlapping render groups
		void AddLayer(const std::string& name);

		// Set the render order of a layer (smallest layer renders earliest, greatest layer renders latest)
		void SetLayerOrder(const std::string& name, int order);

		// Set the render order of a specific sprite within its layer
		void SetSpriteOrder(wi::Sprite* sprite, int order);

		// Set the render order of a specific font within its layer
		void SetFontOrder(wi::SpriteFont* font, int order);

		// Sorts all layers based on their order
		void SortLayers();

		// Removes null items from all layers
		void CleanLayers();

		// Deletes all layers
		void DeleteLayers();

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
		static constexpr const auto script_check_identifier = relative_path_storage(__FILE__);
		const char* GetScriptBindingID() const override { return script_check_identifier.c_str(); }
	};

}
