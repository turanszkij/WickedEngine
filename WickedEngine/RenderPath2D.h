#pragma once
#include "RenderPath.h"
#include "wiGUI.h"

#include <string>

class wiSprite;
class wiFont;

static const std::string DEFAULT_RENDERLAYER = "default";

struct RenderItem2D
{
	enum TYPE
	{
		SPRITE,
		FONT,
	} type;
	wiSprite* sprite = nullptr;
	wiFont* font = nullptr;
	int order = 0;
};
struct RenderLayer2D
{
	std::vector<RenderItem2D> items;
	std::string name;
	int order = 0;

	RenderLayer2D(const std::string& name) :name(name) {}
};

class RenderPath2D :
	public RenderPath
{
private:
	static wiRenderTarget rtFinal;
	wiGUI GUI;
	float spriteSpeed = 1.0f;

protected:
	void ResizeBuffers() override;
public:
	RenderPath2D();
	virtual ~RenderPath2D();

	void Initialize() override;
	void Load() override;
	void Unload() override;
	void Start() override;
	void Update(float dt) override;
	void FixedUpdate() override;
	void Render() override;
	void Compose() override;

	wiGraphicsTypes::Texture2D* GetRenderResult() { return rtFinal.GetTexture(); }

	void addSprite(wiSprite* sprite, const std::string& layer = DEFAULT_RENDERLAYER);
	void removeSprite(wiSprite* sprite);
	void clearSprites();
	void setSpriteSpeed(float value) { spriteSpeed = value; }
	float getSpriteSpeed() { return spriteSpeed; }
	int getSpriteOrder(wiSprite* sprite);

	void addFont(wiFont* font, const std::string& layer = DEFAULT_RENDERLAYER);
	void removeFont(wiFont* font);
	void clearFonts();
	int getFontOrder(wiFont* font);

	std::vector<RenderLayer2D> layers;
	void addLayer(const std::string& name);
	void setLayerOrder(const std::string& name, int order);
	void SetSpriteOrder(wiSprite* sprite, int order);
	void SetFontOrder(wiFont* font, int order);
	void SortLayers();
	void CleanLayers();

	wiGUI& GetGUI() { return GUI; }
};

