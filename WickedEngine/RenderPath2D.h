#pragma once
#include "RenderPath.h"
#include "wiGUI.h"
#include "wiEvent.h"

#include <string>

class wiSprite;
class wiSpriteFont;

struct RenderItem2D
{
	enum TYPE
	{
		SPRITE,
		FONT,
	} type;
	wiSprite* sprite = nullptr;
	wiSpriteFont* font = nullptr;
	int order = 0;
};
struct RenderLayer2D
{
	std::vector<RenderItem2D> items;
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

	wiGUI GUI;
	wiEvent::Handle resolutionChange_handle;
	wiEvent::Handle resolutionScaleChange_handle;
	wiEvent::Handle dpiChange_handle;

protected:
	// create resolution dependant resources, such as render targets
	virtual void ResizeBuffers();
	// update resolution dependent elements, such as elements dependent on current monitor DPI
	virtual void ResizeLayout() {}
public:

	void Load() override;
	void Start() override;
	void Update(float dt) override;
	void FixedUpdate() override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;

	const wiGraphics::Texture& GetRenderResult() const { return rtFinal; }
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

	std::vector<RenderLayer2D> layers{ 1 };
	void AddLayer(const std::string& name);
	void SetLayerOrder(const std::string& name, int order);
	void SetSpriteOrder(wiSprite* sprite, int order);
	void SetFontOrder(wiSpriteFont* font, int order);
	void SortLayers();
	void CleanLayers();

	const wiGUI& GetGUI() const { return GUI; }
	wiGUI& GetGUI() { return GUI; }
};

