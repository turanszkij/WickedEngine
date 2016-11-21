#include "wiGUI.h"
#include "wiWidget.h"
#include "wiHashString.h"
#include "wiRenderer.h"
#include "wiInputManager.h"


wiGUI::wiGUI(GRAPHICSTHREAD threadID) :threadID(threadID), activeWidget(nullptr), focus(false), pointerpos(XMFLOAT2(0,0))
{
}


wiGUI::~wiGUI()
{
}


void wiGUI::Update()
{
	XMFLOAT4 _p = wiInputManager::GetInstance()->getpointer();
	pointerpos.x = _p.x;
	pointerpos.y = _p.y;

	if (activeWidget != nullptr)
	{
		// place to the end of the list to render on top
		std::iter_swap(
			std::find(widgets.begin(),widgets.end(),widgets.back()),
			std::find(widgets.begin(), widgets.end(),activeWidget)
		);

		if (!activeWidget->IsEnabled() || !activeWidget->IsVisible())
		{
			// deactivate active widget if it became invisible or disabled
			DeactivateWidget(activeWidget);
		}
	}

	focus = false;
	for (list<wiWidget*>::reverse_iterator it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		if ((*it)->container == nullptr)
		{
			// the contained child widgets will be updated by the containers
			(*it)->Update(this);
		}

		if ((*it)->IsEnabled() && (*it)->IsVisible() && (*it)->GetState() > wiWidget::WIDGETSTATE::IDLE)
		{
			focus = true;
		}
	}
}

void wiGUI::Render()
{
	wiRenderer::GetDevice()->EventBegin(L"GUI");
	for (auto&x : widgets)
	{
		if (x->container == nullptr)
		{
			// the contained child widgets will be rendered by the containers
			x->Render(this);
		}
	}
	for (auto&x : widgets)
	{
		x->RenderTooltip(this);
	}

	wiGraphicsTypes::Rect scissor[1];
	scissor[0].bottom = (LONG)(wiRenderer::GetDevice()->GetScreenHeight());
	scissor[0].left = (LONG)(0);
	scissor[0].right = (LONG)(wiRenderer::GetDevice()->GetScreenWidth());
	scissor[0].top = (LONG)(0);
	wiRenderer::GetDevice()->SetScissorRects(1, scissor, GetGraphicsThread());
	wiRenderer::GetDevice()->EventEnd();
}

void wiGUI::AddWidget(wiWidget* widget)
{
	widgets.push_back(widget);
}

void wiGUI::RemoveWidget(wiWidget* widget)
{
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
	return focus;
}
