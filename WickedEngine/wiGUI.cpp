#include "wiGUI.h"
#include "wiWidget.h"
#include "wiRenderer.h"
#include "wiInput.h"

using namespace std;
using namespace wiGraphics;

void wiGUIElement::AttachTo(wiGUIElement* parent)
{
	this->parent = parent;

	this->parent->UpdateTransform();
	XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&parent->world));

	MatrixTransform(B);
	UpdateTransform();
	UpdateTransform_Parented(*parent);
}
void wiGUIElement::Detach()
{
	this->parent = nullptr;
	ApplyTransform();
}
void wiGUIElement::ApplyScissor(const Rect rect, CommandList cmd, bool constrain_to_parent) const
{
	Rect scissor = rect;

	if (constrain_to_parent && parent != nullptr)
	{
		wiGUIElement* recurse_parent = parent;
		while (recurse_parent != nullptr)
		{
			scissor.bottom = std::min(scissor.bottom, recurse_parent->scissorRect.bottom);
			scissor.top = std::max(scissor.top, recurse_parent->scissorRect.top);
			scissor.left = std::max(scissor.left, recurse_parent->scissorRect.left);
			scissor.right = std::min(scissor.right, recurse_parent->scissorRect.right);

			recurse_parent = recurse_parent->parent;
		}
	}

	if (scissor.left > scissor.right)
	{
		scissor.left = scissor.right;
	}
	if (scissor.top > scissor.bottom)
	{
		scissor.top = scissor.bottom;
	}

	GraphicsDevice* device = wiRenderer::GetDevice();
	float scale_x = (float)device->GetResolutionWidth() / (float)device->GetScreenWidth();
	float scale_y = (float)device->GetResolutionHeight() / (float)device->GetScreenHeight();
	scissor.bottom = int32_t((float)scissor.bottom * scale_y);
	scissor.top = int32_t((float)scissor.top * scale_y);
	scissor.left = int32_t((float)scissor.left * scale_x);
	scissor.right = int32_t((float)scissor.right * scale_x);
	device->BindScissorRects(1, &scissor, cmd);
}



wiGUI::~wiGUI()
{
	for (auto& widget : widgets)
	{
		delete widget;
	}
}

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
		if (widget->parent == this)
		{
			// the contained child widgets will be updated by the containers
			widget->Update(this, dt);

			if (widget->IsVisible() && widget->hitBox.intersects(pointerhitbox))
			{
				// hitbox can only intersect with one element (avoid detecting multiple overlapping elements)
				pointerhitbox.pos = XMFLOAT2(-FLT_MAX, -FLT_MAX);
				pointerhitbox.siz = XMFLOAT2(0, 0);
			}
		}

		if (widget->IsEnabled() && widget->IsVisible() && widget->GetState() > wiWidget::WIDGETSTATE::IDLE)
		{
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

	scissorRect.bottom = (int32_t)(wiRenderer::GetDevice()->GetScreenHeight());
	scissorRect.left = (int32_t)(0);
	scissorRect.right = (int32_t)(wiRenderer::GetDevice()->GetScreenWidth());
	scissorRect.top = (int32_t)(0);
}

void wiGUI::Render(CommandList cmd) const
{
	if (!visible)
	{
		return;
	}

	wiRenderer::GetDevice()->EventBegin("GUI", cmd);
	for (auto it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		const wiWidget* widget = (*it);
		if (widget->parent == this && widget != activeWidget)
		{
			// the contained child widgets will be rendered by the containers
			ApplyScissor(scissorRect, cmd);
			widget->Render(this, cmd);
		}
	}
	if (activeWidget != nullptr)
	{
		// Active widget is always on top!
		ApplyScissor(scissorRect, cmd);
		activeWidget->Render(this, cmd);
	}

	ApplyScissor(scissorRect, cmd);

	for (auto&x : widgets)
	{
		x->RenderTooltip(this, cmd);
	}

	wiRenderer::GetDevice()->EventEnd(cmd);
}

void wiGUI::AddWidget(wiWidget* widget)
{
	widget->AttachTo(this);
	widgets.push_back(widget);
}

void wiGUI::RemoveWidget(wiWidget* widget)
{
	widget->Detach();
	widgets.remove(widget);
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
