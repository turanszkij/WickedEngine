#include "stdafx.h"
#include "ForceFieldWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


void ForceFieldWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Force Field Window");
	SetSize(XMFLOAT2(420, 120));

	float x = 150;
	float y = 10;
	float hei = 18;
	float step = hei + 2;

	addButton.Create("Add Force Field");
	addButton.SetSize(XMFLOAT2(150, hei));
	addButton.SetPos(XMFLOAT2(x, y += step));
	addButton.OnClick([=](wiEventArgs args) {
		Entity entity = wiScene::GetScene().Entity_CreateForce("editorForce");
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			switch (typeComboBox.GetSelected())
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
			editor->ClearSelected();
			editor->AddSelected(entity);
			editor->RefreshSceneGraphView();
			SetEntity(entity);
		}
		else
		{
			assert(0);
		}
		});
	addButton.SetEnabled(true);
	addButton.SetTooltip("Add new Force Field to the simulation.");
	AddWidget(&addButton);

	typeComboBox.Create("Force Field type: ");
	typeComboBox.SetPos(XMFLOAT2(x, y += step));
	typeComboBox.SetSize(XMFLOAT2(200, hei));
	typeComboBox.OnSelect([&](wiEventArgs args) {
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
	typeComboBox.AddItem("Point");
	typeComboBox.AddItem("Plane");
	typeComboBox.SetEnabled(false);
	typeComboBox.SetTooltip("Choose the force field type.");
	AddWidget(&typeComboBox);


	gravitySlider.Create(-10, 10, 0, 100000, "Gravity: ");
	gravitySlider.SetSize(XMFLOAT2(200, hei));
	gravitySlider.SetPos(XMFLOAT2(x, y += step));
	gravitySlider.OnSlide([&](wiEventArgs args) {
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			force->gravity = args.fValue;
		}
	});
	gravitySlider.SetEnabled(false);
	gravitySlider.SetTooltip("Set the amount of gravity. Positive values attract, negatives deflect.");
	AddWidget(&gravitySlider);


	rangeSlider.Create(0.0f, 100.0f, 10, 100000, "Range: ");
	rangeSlider.SetSize(XMFLOAT2(200, hei));
	rangeSlider.SetPos(XMFLOAT2(x, y += step));
	rangeSlider.OnSlide([&](wiEventArgs args) {
		ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			force->range_local = args.fValue;
		}
	});
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Set the range of affection.");
	AddWidget(&rangeSlider);



	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 720, 50, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ForceFieldWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const ForceFieldComponent* force = wiScene::GetScene().forces.GetComponent(entity);

	if (force != nullptr)
	{
		typeComboBox.SetSelected(force->type == ENTITY_TYPE_FORCEFIELD_POINT ? 0 : 1);
		gravitySlider.SetValue(force->gravity);
		rangeSlider.SetValue(force->range_local);

		gravitySlider.SetEnabled(true);
		rangeSlider.SetEnabled(true);

	}
	else
	{
		gravitySlider.SetEnabled(false);
		rangeSlider.SetEnabled(false);
	}

	addButton.SetEnabled(true);
}
