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