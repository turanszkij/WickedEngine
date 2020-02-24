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
	wiWidget* activeWidget;
	bool focus;
	bool visible;

	XMFLOAT2 pointerpos;
public:
	wiGUI();
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


	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
};

