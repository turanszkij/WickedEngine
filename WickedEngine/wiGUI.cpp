#include "wiGUI.h"
#include "wiWidget.h"
#include "wiHashString.h"
#include "wiRenderer.h"
#include "wiInputManager.h"

using namespace std;

wiGUI::wiGUI(GRAPHICSTHREAD threadID) :threadID(threadID), activeWidget(nullptr), focus(false), visible(true), pointerpos(XMFLOAT2(0,0))
{
	SetDirty();
	scale_local.x = (float)wiRenderer::GetDevice()->GetScreenWidth();
	scale_local.y = (float)wiRenderer::GetDevice()->GetScreenHeight();
	UpdateTransform();
}


wiGUI::~wiGUI()
{
}


void wiGUI::Update(float dt)
{
	if (!visible)
	{
		return;
	}

	if (wiRenderer::GetDevice()->ResolutionChanged())
	{
		SetDirty();
		scale_local.x = (float)wiRenderer::GetDevice()->GetScreenWidth();
		scale_local.y = (float)wiRenderer::GetDevice()->GetScreenHeight();
		UpdateTransform();
	}

	XMFLOAT4 _p = wiInputManager::getpointer();
	pointerpos.x = _p.x;
	pointerpos.y = _p.y;

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
		}

		if (widget->IsEnabled() && widget->IsVisible() && widget->GetState() > wiWidget::WIDGETSTATE::IDLE)
		{
			focus = true;
		}
	}
}

void wiGUI::Render()
{
	if (!visible)
	{
		return;
	}

	wiRenderer::GetDevice()->EventBegin("GUI", GetGraphicsThread());
	for (auto&x : widgets)
	{
		if (x->parent == this && x != activeWidget)
		{
			// the contained child widgets will be rendered by the containers
			x->Render(this);
		}
	}
	if (activeWidget != nullptr)
	{
		// render the active widget on top of everything
		activeWidget->Render(this);
	}

	for (auto&x : widgets)
	{
		x->RenderTooltip(this);
	}

	ResetScissor();
	wiRenderer::GetDevice()->EventEnd(GetGraphicsThread());
}

void wiGUI::ResetScissor()
{
	wiGraphicsTypes::Rect scissor[1];
	scissor[0].bottom = (LONG)(wiRenderer::GetDevice()->GetScreenHeight());
	scissor[0].left = (LONG)(0);
	scissor[0].right = (LONG)(wiRenderer::GetDevice()->GetScreenWidth());
	scissor[0].top = (LONG)(0);
	wiRenderer::GetDevice()->BindScissorRects(1, scissor, GetGraphicsThread());
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

wiWidget* wiGUI::GetWidget(const wiHashString& name)
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
	activeWidget = widget;
	activeWidget->Activate();
}
void wiGUI::DeactivateWidget(wiWidget* widget)
{
	widget->Deactivate();
	if (activeWidget == widget)
	{
		activeWidget = nullptr;
	}
}
wiWidget* wiGUI::GetActiveWidget()
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
