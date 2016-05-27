#include "wiGUI.h"
#include "wiWidget.h"
#include "wiHashString.h"


wiGUI::wiGUI(GRAPHICSTHREAD threadID) :threadID(threadID), activeWidget(nullptr)
{
}


wiGUI::~wiGUI()
{
}


void wiGUI::Update()
{
	if (activeWidget != nullptr)
	{
		if (!activeWidget->IsEnabled() || !activeWidget->IsVisible())
		{
			// deactivate active widget if it became invisible or disabled
			DeactivateWidget(activeWidget);
		}
	}

	for (auto&x : widgets)
	{
		x->Update(this);
	}
}

void wiGUI::Render()
{
	for (auto&x : widgets)
	{
		x->Render(this);
	}
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