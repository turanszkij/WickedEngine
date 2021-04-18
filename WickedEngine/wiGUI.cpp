#include "wiGUI.h"
#include "wiWidget.h"
#include "wiRenderer.h"
#include "wiInput.h"

using namespace std;
using namespace wiGraphics;

void wiGUI::Update(float dt)
{
	if (!visible)
	{
		return;
	}

	XMFLOAT4 _p = wiInput::GetPointer();
	pointerpos.x = _p.x;
	pointerpos.y = _p.y;
	pointerhitbox = Hitbox2D(pointerpos, XMFLOAT2(1, 1));

	if (activeWidget != nullptr)
	{
		if (!activeWidget->IsEnabled() || !activeWidget->IsVisible())
		{
			// deactivate active widget if it became invisible or disabled
			DeactivateWidget(activeWidget);
		}
	}

	focus = false;
	for (auto& widget : widgets)
	{
		// the contained child widgets will be updated by the containers
		widget->Update(this, dt);

		if (widget->IsVisible() && widget->hitBox.intersects(pointerhitbox))
		{
			// hitbox can only intersect with one element (avoid detecting multiple overlapping elements)
			pointerhitbox.pos = XMFLOAT2(-FLT_MAX, -FLT_MAX);
			pointerhitbox.siz = XMFLOAT2(0, 0);
			focus = true;
		}

		if (widget->priority_change)
		{
			widget->priority_change = false;
			priorityChangeQueue.push_back(widget);
		}
	}

	for (auto& widget : priorityChangeQueue)
	{
		if (std::find(widgets.begin(), widgets.end(), widget) != widgets.end()) // only add back to widgets if it's still there!
		{
			widgets.remove(widget);
			widgets.push_front(widget);
		}
	}
	priorityChangeQueue.clear();
}

void wiGUI::Render(CommandList cmd) const
{
	if (!visible)
	{
		return;
	}

	Rect scissorRect;
	scissorRect.bottom = (int32_t)(GetCanvas().GetPhysicalHeight());
	scissorRect.left = (int32_t)(0);
	scissorRect.right = (int32_t)(GetCanvas().GetPhysicalWidth());
	scissorRect.top = (int32_t)(0);

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("GUI", cmd);
	for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		const wiWidget* widget = (*it);
		if (widget != activeWidget)
		{
			device->BindScissorRects(1, &scissorRect, cmd);
			widget->Render(this, cmd);
		}
	}
	if (activeWidget != nullptr)
	{
		// Active widget is always on top!
		device->BindScissorRects(1, &scissorRect, cmd);
		activeWidget->Render(this, cmd);
	}

	device->BindScissorRects(1, &scissorRect, cmd);
	if (activeWidget == nullptr)
	{
		for (auto& x : widgets)
		{
			x->RenderTooltip(this, cmd);
		}
	}

	device->EventEnd(cmd);
}

void wiGUI::AddWidget(wiWidget* widget)
{
	if (widget != nullptr)
	{
		assert(std::find(widgets.begin(), widgets.end(), widget) == widgets.end()); // don't attach one widget twice!
		widgets.push_back(widget);
	}
}

void wiGUI::RemoveWidget(wiWidget* widget)
{
	if (widget != nullptr)
	{
		widget->Detach();
		widgets.remove(widget);
	}
}

wiWidget* wiGUI::GetWidget(const std::string& name)
{
	for (auto& x : widgets)
	{
		if (x->GetName() == name)
		{
			return x;
		}
	}
	return nullptr;
}

void wiGUI::ActivateWidget(wiWidget* widget)
{
	widget->priority_change = false;
	priorityChangeQueue.push_back(widget);

	if (activeWidget == nullptr)
	{
		activeWidget = widget;
		activeWidget->Activate();
	}
}
void wiGUI::DeactivateWidget(wiWidget* widget)
{
	widget->Deactivate();
	if (activeWidget == widget)
	{
		activeWidget = nullptr;
	}
}
const wiWidget* wiGUI::GetActiveWidget() const
{
	return activeWidget;
}
bool wiGUI::IsWidgetDisabled(wiWidget* widget)
{
	return (activeWidget != nullptr && activeWidget != widget);
}
bool wiGUI::HasFocus()
{
	if (!visible)
	{
		return false;
	}

	return focus;
}
