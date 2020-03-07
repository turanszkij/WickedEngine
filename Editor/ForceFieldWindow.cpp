#include "stdafx.h"
#include "ForceFieldWindow.h"

using namespace wiECS;
using namespace wiScene;


ForceFieldWindow::ForceFieldWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	forceFieldWindow = new wiWindow(GUI, "Force Field Window");
	forceFieldWindow->SetSize(XMFLOAT2(600, 300));
	GUI->AddWidget(forceFieldWindow);

	float x = 150;
	float y = 10;
	float step = 35;

	typeComboBox = new wiComboBox("Force Field type: ");
	typeComboBox->SetPos(XMFLOAT2(x, y += step));
	typeComboBox->SetSize(XMFLOAT2(300, 25));
	typeComboBox->OnSelect([&](wiEventArgs args) {
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr && args.iValue >= 0)
		{
			switch (args.iValue)
			{
			case 0:
				force->type = ENTITY_TYPE_FORCEFIELD_POINT;
				break;
			case 1:
				force->type = ENTITY_TYPE_FORCEFIELD_PLANE;
				break;
			default:
				assert(0); // error
				break;
			}
		}
	});
	typeComboBox->AddItem("Point");
	typeComboBox->AddItem("Plane");
	typeComboBox->SetEnabled(false);
	typeComboBox->SetTooltip("Choose the force field type.");
	forceFieldWindow->AddWidget(typeComboBox);


	gravitySlider = new wiSlider(-10, 10, 0, 100000, "Gravity: ");
	gravitySlider->SetSize(XMFLOAT2(200, 30));
	gravitySlider->SetPos(XMFLOAT2(x, y += step));
	gravitySlider->OnSlide([&](wiEventArgs args) {
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			force->gravity = args.fValue;
		}
	});
	gravitySlider->SetEnabled(false);
	gravitySlider->SetTooltip("Set the amount of gravity. Positive values attract, negatives deflect.");
	forceFieldWindow->AddWidget(gravitySlider);


	rangeSlider = new wiSlider(0.0f, 100.0f, 10, 100000, "Range: ");
	rangeSlider->SetSize(XMFLOAT2(200, 30));
	rangeSlider->SetPos(XMFLOAT2(x, y += step));
	rangeSlider->OnSlide([&](wiEventArgs args) {
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			force->range_local = args.fValue;
		}
	});
	rangeSlider->SetEnabled(false);
	rangeSlider->SetTooltip("Set the range of affection.");
	forceFieldWindow->AddWidget(rangeSlider);


	addButton = new wiButton("Add Force Field");
	addButton->SetSize(XMFLOAT2(150, 30));
	addButton->SetPos(XMFLOAT2(x, y += step * 2));
	addButton->OnClick([&](wiEventArgs args) {
		Entity entity = wiScene::GetScene().Entity_CreateForce("editorForce");
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			switch (typeComboBox->GetSelected())
			{
			case 0:
				force->type = ENTITY_TYPE_FORCEFIELD_POINT;
				break;
			case 1:
				force->type = ENTITY_TYPE_FORCEFIELD_PLANE;
				break;
			default:
				assert(0);
				break;
			}
		}
		else
		{
			assert(0);
		}
	});
	addButton->SetEnabled(true);
	addButton->SetTooltip("Add new Force Field to the simulation.");
	forceFieldWindow->AddWidget(addButton);



	forceFieldWindow->Translate(XMFLOAT3(gui->GetSize().x - 720, 50, 0));
	forceFieldWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


ForceFieldWindow::~ForceFieldWindow()
{
	forceFieldWindow->RemoveWidgets(true);
	GUI->RemoveWidget(forceFieldWindow);
	delete forceFieldWindow;
}

void ForceFieldWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);

	if (force != nullptr)
	{
		typeComboBox->SetSelected(force->type == ENTITY_TYPE_FORCEFIELD_POINT ? 0 : 1);
		gravitySlider->SetValue(force->gravity);
		rangeSlider->SetValue(force->range_local);

		gravitySlider->SetEnabled(true);
		rangeSlider->SetEnabled(true);

	}
	else
	{
		gravitySlider->SetEnabled(false);
		rangeSlider->SetEnabled(false);
	}

	addButton->SetEnabled(true);
}
