#include "stdafx.h"
#include "LightWindow.h"


LightWindow::LightWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	lightWindow = new wiWindow(GUI, "Light Window");
	lightWindow->SetSize(XMFLOAT2(400, 300));
	//lightWindow->SetEnabled(false);
	GUI->AddWidget(lightWindow);

	float x = 200;
	float y = 0;

	energySlider = new wiSlider(0.1f, 64, 0, 100000, "Energy: ");
	energySlider->SetSize(XMFLOAT2(100, 30));
	energySlider->SetPos(XMFLOAT2(x, y += 30));
	energySlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.x = args.fValue;
		}
	});
	energySlider->SetEnabled(false);
	lightWindow->AddWidget(energySlider);

	distanceSlider = new wiSlider(1, 1000, 0, 100000, "Distance: ");
	distanceSlider->SetSize(XMFLOAT2(100, 30));
	distanceSlider->SetPos(XMFLOAT2(x, y += 30));
	distanceSlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.y = args.fValue;
		}
	});
	distanceSlider->SetEnabled(false);
	lightWindow->AddWidget(distanceSlider);

	fovSlider = new wiSlider(0.1f, XM_PI - 0.01f, 0, 100000, "FOV: ");
	fovSlider->SetSize(XMFLOAT2(100, 30));
	fovSlider->SetPos(XMFLOAT2(x, y += 30));
	fovSlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.z = args.fValue;
		}
	});
	fovSlider->SetEnabled(false);
	lightWindow->AddWidget(fovSlider);

	shadowCheckBox = new wiCheckBox("Shadow: ");
	shadowCheckBox->SetPos(XMFLOAT2(x, y += 30));
	shadowCheckBox->OnClick([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->shadow = args.bValue;
		}
	});
	shadowCheckBox->SetEnabled(false);
	lightWindow->AddWidget(shadowCheckBox);

	haloCheckBox = new wiCheckBox("Halo: ");
	haloCheckBox->SetPos(XMFLOAT2(x, y += 30));
	haloCheckBox->OnClick([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->noHalo = !args.bValue;
		}
	});
	haloCheckBox->SetEnabled(false);
	lightWindow->AddWidget(haloCheckBox);

	addLightButton = new wiButton("Add Light");
	addLightButton->SetPos(XMFLOAT2(x, y += 30));
	addLightButton->SetSize(XMFLOAT2(180, 30));
	addLightButton->OnClick([&](wiEventArgs args) {
		Model* model = new Model;
		Light* light = new Light;
		light->enerDis = XMFLOAT4(2, 60, XM_PIDIV4, 0);
		light->color = XMFLOAT4(1, 1, 1, 1);
		light->type = Light::SPOT;
		model->lights.push_back(light);
		wiRenderer::AddModel(model);
	});
	lightWindow->AddWidget(addLightButton);


	lightWindow->Translate(XMFLOAT3(30, 30, 0));
	lightWindow->SetVisible(false);
}


LightWindow::~LightWindow()
{
	SAFE_DELETE(lightWindow);
	SAFE_DELETE(energySlider);
	SAFE_DELETE(distanceSlider);
	SAFE_DELETE(fovSlider);
	SAFE_DELETE(shadowCheckBox);
	SAFE_DELETE(haloCheckBox);
	SAFE_DELETE(addLightButton);
}

void LightWindow::SetLight(Light* light)
{
	this->light = light;
	if (light != nullptr)
	{
		//lightWindow->SetEnabled(true);
		energySlider->SetEnabled(true);
		energySlider->SetValue(light->enerDis.x);
		if (light->type == Light::DIRECTIONAL)
		{
			distanceSlider->SetEnabled(false);
			fovSlider->SetEnabled(false);
		}
		else
		{
			distanceSlider->SetEnabled(true);
			distanceSlider->SetValue(light->enerDis.y);
			if (light->type == Light::SPOT)
			{
				fovSlider->SetEnabled(true);
				fovSlider->SetValue(light->enerDis.z);
			}
			else
			{
				fovSlider->SetEnabled(false);
			}
		}
		shadowCheckBox->SetCheck(light->shadow);
		haloCheckBox->SetCheck(!light->noHalo);
	}
	else
	{
		distanceSlider->SetEnabled(false);
		fovSlider->SetEnabled(false);
		energySlider->SetEnabled(false);
		//lightWindow->SetEnabled(false);
	}
}
