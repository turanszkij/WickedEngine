#pragma once
#include "RenderPath.h"
#include "wiContainer.h"
#include "wiGUI.h"

#include <string>

class wiSprite;
class wiSpriteFont;

struct RenderItem2D
{
	enum TYPE
	{
		SPRITE,
		FONT,
	} type = SPRITE;
	union
	{
		wiSprite* sprite = nullptr;
		wiSpriteFont* font;
	};
	int order = 0;
};
struct RenderLayer2D
{
	wiContainer::vector<RenderItem2D> items;
	std::string name;
	int order = 0;
};

class RenderPath2D :
	public RenderPath
{
private:
	wiGraphics::Texture rtStenciled;
	wiGraphics::Texture rtStenciled_resolved;
	wiGraphics::Texture rtFinal;

	wiGraphics::RenderPass renderpass_stenciled;
	wiGraphics::RenderPass renderpass_final;

	wiGraphics::Texture rtLinearColorSpace;
	wiGraphics::RenderPass renderpass_linearize;

	wiGUI GUI;

	XMUINT2 current_buffersize{};
	float current_layoutscale{};

	mutable wiGraphics::Texture render_result = rtFinal;

	float hdr_scaling = 9.0f;

public:
	// create resolution dependent resources, such as render targets
	virtual void ResizeBuffers();
	// update DPI dependent elements, such as GUI elements, sprites
	virtual void ResizeLayout();

	void Update(float dt) override;
	void FixedUpdate() override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;

	const wiGraphics::Texture& GetRenderResult() const { return render_result; }
	virtual const wiGraphics::Texture* GetDepthStencil() const { return nullptr; }
	virtual const wiGraphics::Texture* GetGUIBlurredBackground() const { return nullptr; }

	void AddSprite(wiSprite* sprite, const std::string& layer = "");
	void RemoveSprite(wiSprite* sprite);
	void ClearSprites();
	int GetSpriteOrder(wiSprite* sprite);

	void AddFont(wiSpriteFont* font, const std::string& layer = "");
	void RemoveFont(wiSpriteFont* font);
	void ClearFonts();
	int GetFontOrder(wiSpriteFont* font);

	wiContainer::vector<RenderLayer2D> layers{ 1 };
	void AddLayer(const std::string& name);
	void SetLayerOrder(const std::string& name, int order);
	void SetSpriteOrder(wiSprite* sprite, int order);
	void SetFontOrder(wiSpriteFont* font, int order);
	void SortLayers();
	void CleanLayers();

	const wiGUI& GetGUI() const { return GUI; }
	wiGUI& GetGUI() { return GUI; }

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

