#include "stdafx.h"
#include "ObjectWindow.h"


ObjectWindow::ObjectWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	object = nullptr;


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	objectWindow = new wiWindow(GUI, "Object Window");
	objectWindow->SetSize(XMFLOAT2(600, 400));
	objectWindow->SetEnabled(false);
	GUI->AddWidget(objectWindow);

	ditherSlider = new wiSlider(0, 1, 0, 1000, "Dither: ");
	ditherSlider->SetSize(XMFLOAT2(100, 30));
	ditherSlider->SetPos(XMFLOAT2(400, 30));
	ditherSlider->OnSlide([&](wiEventArgs args) {
		object->transparency = args.fValue;
	});
	objectWindow->AddWidget(ditherSlider);




	objectWindow->Translate(XMFLOAT3(30, 30, 0));
	objectWindow->SetVisible(false);

}


ObjectWindow::~ObjectWindow()
{
	SAFE_DELETE(objectWindow);
	SAFE_DELETE(ditherSlider);
}


void ObjectWindow::SetObject(Object* obj)
{
	object = obj;

	if (object != nullptr)
	{
		ditherSlider->SetValue(object->transparency);
		objectWindow->SetEnabled(true);
	}
	else
	{
		objectWindow->SetEnabled(false);
	}

}
