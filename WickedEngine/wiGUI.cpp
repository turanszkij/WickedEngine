#include "wiGUI.h"
#include "wiRenderer.h"
#include "wiInput.h"
#include "wiIntersect.h"

using namespace wiGraphics;

void wiGUI::Update(const wiCanvas& canvas, float dt)
{
	if (!visible)
	{
		return;
	}

	XMFLOAT4 pointer = wiInput::GetPointer();
	Hitbox2D pointerHitbox = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));

	uint32_t priority = 0;

	focus = false;
	for (auto& widget : widgets)
	{
		widget->force_disable = focus;
		widget->Update(canvas, dt);
		widget->force_disable = false;

		if (widget->priority_change)
		{
			widget->priority_change = false;
			widget->priority = priority++;
		}
		else
		{
			widget->priority = ~0u;
		}

		if (widget->IsVisible() && widget->hitBox.intersects(pointerHitbox))
		{
			focus = true;
		}
		if (widget->GetState() > wiWidget::IDLE)
		{
			focus = true;
		}
	}

	if(priority > 0) //Sort only if there are priority changes
		//Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
		std::stable_sort(widgets.begin(), widgets.end(), [](const wiWidget* a, const wiWidget* b) {
			return a->priority < b->priority;
			});
}

void wiGUI::Render(const wiCanvas& canvas, CommandList cmd) const
{
	if (!visible)
	{
		return;
	}

	Rect scissorRect;
	scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
	scissorRect.left = (int32_t)(0);
	scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
	scissorRect.top = (int32_t)(0);

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("GUI", cmd);
	// Rendering is back to front:
	for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		const wiWidget* widget = (*it);
		device->BindScissorRects(1, &scissorRect, cmd);
		widget->Render(canvas, cmd);
	}

	device->BindScissorRects(1, &scissorRect, cmd);
	for (auto& x : widgets)
	{
		x->RenderTooltip(canvas, cmd);
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
	for (auto& x : widgets)
	{
		if (x == widget)
		{
			x = widgets.back();
			widgets.pop_back();
			break;
		}
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

bool wiGUI::HasFocus()
{
	if (!visible)
	{
		return false;
	}

	return focus;
}
