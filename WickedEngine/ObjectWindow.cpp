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

	float x = 200;
	float y = 0;

	ditherSlider = new wiSlider(0, 1, 0, 1000, "Dither: ");
	ditherSlider->SetTooltip("Adjust dithered transparency of the object.");
	ditherSlider->SetSize(XMFLOAT2(100, 30));
	ditherSlider->SetPos(XMFLOAT2(x, y += 30));
	ditherSlider->OnSlide([&](wiEventArgs args) {
		if (object != nullptr)
		{
			object->transparency = args.fValue;
		}
	});
	objectWindow->AddWidget(ditherSlider);


	colorPickerToggleButton = new wiButton("Color");
	colorPickerToggleButton->SetTooltip("Adjust the object color.");
	colorPickerToggleButton->SetPos(XMFLOAT2(x, y += 30));
	colorPickerToggleButton->OnClick([&](wiEventArgs args) {
		colorPicker->SetVisible(!colorPicker->IsVisible());
	});
	objectWindow->AddWidget(colorPickerToggleButton);


	colorPicker = new wiColorPicker(GUI, "Object Color");
	colorPicker->SetVisible(false);
	colorPicker->SetEnabled(false);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		if (object != nullptr)
		{
			object->color = XMFLOAT3(args.color.x, args.color.y, args.color.z);
		}
	});
	GUI->AddWidget(colorPicker);

	y += 60;

	physicsLabel = new wiLabel("PHYSICSLABEL");
	physicsLabel->SetText("--- PHYSICS SETTINGS ---");
	physicsLabel->SetPos(XMFLOAT2(x, y += 30));
	physicsLabel->SetSize(XMFLOAT2(200, 20));
	objectWindow->AddWidget(physicsLabel);

	simulationTypeComboBox = new wiComboBox("Simulation Type:");
	simulationTypeComboBox->SetSize(XMFLOAT2(100, 20));
	simulationTypeComboBox->SetPos(XMFLOAT2(x, y += 30));
	simulationTypeComboBox->AddItem("None");
	simulationTypeComboBox->AddItem("Rigid Body");
	simulationTypeComboBox->AddItem("Soft Body");
	simulationTypeComboBox->OnSelect([&](wiEventArgs args) {
		if (object != nullptr)
		{
			switch (args.iValue)
			{
			case 0:
				object->rigidBody = false;
				if (object->mesh != nullptr)
				{
					object->mesh->softBody = false;
				}
				break;
			case 1:
				object->rigidBody = true;
				if (object->mesh != nullptr)
				{
					object->mesh->softBody = false;
				}
				break;
			case 2:
				object->rigidBody = false;
				if (object->mesh != nullptr)
				{
					object->mesh->softBody = true;
				}
				break;
			default:
				break;
			}
		}
	});
	simulationTypeComboBox->SetSelected(0);
	simulationTypeComboBox->SetEnabled(true);
	simulationTypeComboBox->SetTooltip("Set simulation type.");
	objectWindow->AddWidget(simulationTypeComboBox);

	kinematicCheckBox = new wiCheckBox("Kinematic: ");
	kinematicCheckBox->SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox->SetPos(XMFLOAT2(x, y += 30));
	kinematicCheckBox->SetCheck(false);
	kinematicCheckBox->OnClick([&](wiEventArgs args) {
		if (object != nullptr)
		{
			object->kinematic = args.bValue;
		}
	});
	objectWindow->AddWidget(kinematicCheckBox);

	physicsTypeComboBox = new wiComboBox("Contribution Type:");
	physicsTypeComboBox->SetSize(XMFLOAT2(100, 20));
	physicsTypeComboBox->SetPos(XMFLOAT2(x, y += 30));
	physicsTypeComboBox->AddItem("Active");
	physicsTypeComboBox->AddItem("Passive");
	physicsTypeComboBox->OnSelect([&](wiEventArgs args) {
		if (object != nullptr)
		{
			switch (args.iValue)
			{
			case 0:
				object->physicsType = "ACTIVE";
				break;
			case 1:
				object->physicsType = "PASSIVE";
				break;
			default:
				break;
			}
		}
	});
	physicsTypeComboBox->SetSelected(0);
	physicsTypeComboBox->SetEnabled(true);
	physicsTypeComboBox->SetTooltip("Set physics type.");
	objectWindow->AddWidget(physicsTypeComboBox);

	collisionShapeComboBox = new wiComboBox("Collision Shape:");
	collisionShapeComboBox->SetSize(XMFLOAT2(100, 20));
	collisionShapeComboBox->SetPos(XMFLOAT2(x, y += 30));
	collisionShapeComboBox->AddItem("Box");
	collisionShapeComboBox->AddItem("Sphere");
	collisionShapeComboBox->AddItem("Capsule");
	collisionShapeComboBox->AddItem("Convex Hull");
	collisionShapeComboBox->AddItem("Triangle Mesh");
	collisionShapeComboBox->OnSelect([&](wiEventArgs args) {
		if (object != nullptr)
		{
			switch (args.iValue)
			{
			case 0:
				object->collisionShape = "BOX";
				break;
			case 1:
				object->collisionShape = "SPHERE";
				break;
			case 2:
				object->collisionShape = "CAPSULE";
				break;
			case 3:
				object->collisionShape = "CONVEX_HULL";
				break;
			case 4:
				object->collisionShape = "MESH";
				break;
			default:
				break;
			}
		}
	});
	collisionShapeComboBox->SetSelected(0);
	collisionShapeComboBox->SetEnabled(true);
	collisionShapeComboBox->SetTooltip("Set rigid body collision shape.");
	objectWindow->AddWidget(collisionShapeComboBox);




	objectWindow->Translate(XMFLOAT3(1300, 100, 0));
	objectWindow->SetVisible(false);

	SetObject(nullptr);
}


ObjectWindow::~ObjectWindow()
{
	SAFE_DELETE(objectWindow);
	SAFE_DELETE(ditherSlider);
	SAFE_DELETE(colorPickerToggleButton);
	SAFE_DELETE(colorPicker);
	SAFE_DELETE(physicsLabel);
	SAFE_DELETE(simulationTypeComboBox);
	SAFE_DELETE(kinematicCheckBox);
	SAFE_DELETE(physicsTypeComboBox);
	SAFE_DELETE(collisionShapeComboBox);
}


void ObjectWindow::SetObject(Object* obj)
{
	object = obj;

	if (object != nullptr)
	{
		ditherSlider->SetValue(object->transparency);

		if (object->rigidBody)
		{
			simulationTypeComboBox->SetSelected(1);
		}
		else
		{
			if (object->mesh != nullptr)
			{
				if (object->mesh->softBody)
				{
					simulationTypeComboBox->SetSelected(2);
				}
				else
				{
					simulationTypeComboBox->SetSelected(0);
				}
			}
			else
			{
				simulationTypeComboBox->SetSelected(0);
			}
		}

		kinematicCheckBox->SetCheck(object->kinematic);

		if (!object->physicsType.compare("ACTIVE"))
		{
			physicsTypeComboBox->SetSelected(0);
		}
		else if (!object->physicsType.compare("PASSIVE"))
		{
			physicsTypeComboBox->SetSelected(1);
		}

		if (!object->collisionShape.compare("BOX"))
		{
			collisionShapeComboBox->SetSelected(0);
		}
		else if (!object->collisionShape.compare("SPHERE"))
		{
			collisionShapeComboBox->SetSelected(1);
		}
		else if (!object->collisionShape.compare("CAPSULE"))
		{
			collisionShapeComboBox->SetSelected(2);
		}
		else if (!object->collisionShape.compare("CONVEX_HULL"))
		{
			collisionShapeComboBox->SetSelected(3);
		}
		else if (!object->collisionShape.compare("MESH"))
		{
			collisionShapeComboBox->SetSelected(4);
		}

		objectWindow->SetEnabled(true);

		switch (simulationTypeComboBox->GetSelected())
		{
		case 1:
			kinematicCheckBox->SetEnabled(true);
			physicsTypeComboBox->SetEnabled(true);
			collisionShapeComboBox->SetEnabled(true);
			break;
		case 2:
			kinematicCheckBox->SetEnabled(false);
			physicsTypeComboBox->SetEnabled(true);
			collisionShapeComboBox->SetEnabled(false);
			break;
		default:
			kinematicCheckBox->SetEnabled(false);
			physicsTypeComboBox->SetEnabled(false);
			collisionShapeComboBox->SetEnabled(false);
			break;
		}
	}
	else
	{
		objectWindow->SetEnabled(false);
	}

}
