#include "stdafx.h"
#include "ForceFieldWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void ForceFieldWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Force Field", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(420, 120));

	float x = 150;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	addButton.Create("Add Force Field");
	addButton.SetSize(XMFLOAT2(150, hei));
	addButton.SetPos(XMFLOAT2(x, y));
	addButton.OnClick([=](wi::gui::EventArgs args) {
		Entity entity = editor->GetCurrentScene().Entity_CreateForce("editorForce");
		ForceFieldComponent* force = editor->GetCurrentScene().forces.GetComponent(entity);
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

			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			editor->RecordSelection(archive);

			editor->ClearSelected();
			editor->AddSelected(entity);

			editor->RecordSelection(archive);
			editor->RecordAddedEntity(archive, entity);

			editor->RefreshEntityTree();
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
	typeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		ForceFieldComponent* force = editor->GetCurrentScene().forces.GetComponent(entity);
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
	gravitySlider.OnSlide([&](wi::gui::EventArgs args) {
		ForceFieldComponent* force = editor->GetCurrentScene().forces.GetComponent(entity);
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
	rangeSlider.OnSlide([&](wi::gui::EventArgs args) {
		ForceFieldComponent* force = editor->GetCurrentScene().forces.GetComponent(entity);
		if (force != nullptr)
		{
			force->range = args.fValue;
		}
	});
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Set the range of affection.");
	AddWidget(&rangeSlider);



	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ForceFieldWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const ForceFieldComponent* force = editor->GetCurrentScene().forces.GetComponent(entity);

	if (force != nullptr)
	{
		typeComboBox.SetSelected(force->type == ENTITY_TYPE_FORCEFIELD_POINT ? 0 : 1);
		gravitySlider.SetValue(force->gravity);
		rangeSlider.SetValue(force->range);

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
