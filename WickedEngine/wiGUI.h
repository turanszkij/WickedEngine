#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiScene.h"

#include <list>

class wiHashString;

class wiWidget;

class wiGUIElement : public wiScene::TransformComponent
{
protected:
	void ApplyScissor(const wiGraphics::Rect rect, wiGraphics::CommandList cmd, bool constrain_to_parent = true) const;
public:
	wiGraphics::Rect scissorRect;

	wiGUIElement* parent = nullptr;
	XMFLOAT4X4 world_parent_bind = IDENTITYMATRIX;
	void AttachTo(wiGUIElement* parent);
	void Detach();
};

class wiGUI : public wiGUIElement
{
	friend class wiWidget;
private:
	std::list<wiWidget*> widgets;
	wiWidget* activeWidget = nullptr;
	bool focus = false;
	bool visible = true;
	XMFLOAT2 pointerpos = XMFLOAT2(0, 0);
public:
	wiGUI()
	{
		SetSize(1920, 1080); // default size
	}
	~wiGUI();

	void Update(float dt);
	void Render(wiGraphics::CommandList cmd) const;

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	wiWidget* GetWidget(const wiHashString& name);

	void ActivateWidget(wiWidget* widget);
	void DeactivateWidget(wiWidget* widget);
	const wiWidget* GetActiveWidget() const;
	// true if another widget is currently active
	bool IsWidgetDisabled(wiWidget* widget);

	// returns true if any gui element has the focus
	bool HasFocus();

	void SetVisible(bool value) { visible = value; }
	bool IsVisible() { return visible; }

	XMFLOAT2 GetSize() const { return XMFLOAT2(scale_local.x, scale_local.y); }
	void SetSize(float width, float height)
	{
		SetDirty();
		scale_local.x = width;
		scale_local.y = height;
		UpdateTransform();
	}

	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
};

