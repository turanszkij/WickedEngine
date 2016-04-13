#include "wiWidget.h"
#include "wiGUI.h"



wiWidget::wiWidget()
{
}


wiWidget::~wiWidget()
{
}

string wiWidget::GetName()
{
	return name;
}

void wiWidget::SetName(const string& newName)
{
	name = newName;
}





void wiButton::Update(wiGUI* gui)
{

}
void wiButton::Render(wiGUI* gui)
{

}

void wiButton::OnClick(function<void()> func)
{
	onClick = move(func);
}