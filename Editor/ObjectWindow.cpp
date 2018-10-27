#include "stdafx.h"
#include "ObjectWindow.h"
#include "wiSceneSystem.h"

#include <sstream>

using namespace wiECS;
using namespace wiSceneSystem;


ObjectWindow::ObjectWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	objectWindow = new wiWindow(GUI, "Object Window");
	objectWindow->SetSize(XMFLOAT2(600, 400));
	objectWindow->SetEnabled(false);
	GUI->AddWidget(objectWindow);

	float x = 450;
	float y = 0;

	nameLabel = new wiLabel("NAMELABEL");
	nameLabel->SetText("");
	nameLabel->SetPos(XMFLOAT2(x - 30, y += 30));
	nameLabel->SetSize(XMFLOAT2(150, 20));
	objectWindow->AddWidget(nameLabel);

	renderableCheckBox = new wiCheckBox("Renderable: ");
	renderableCheckBox->SetTooltip("Set object to be participating in rendering.");
	renderableCheckBox->SetPos(XMFLOAT2(x, y += 30));
	renderableCheckBox->SetCheck(true);
	renderableCheckBox->OnClick([&](wiEventArgs args) {
		ObjectComponent* object = wiRenderer::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->SetRenderable(args.bValue);
		}
	});
	objectWindow->AddWidget(renderableCheckBox);

	ditherSlider = new wiSlider(0, 1, 0, 1000, "Dither: ");
	ditherSlider->SetTooltip("Adjust dithered transparency of the object. This disables some optimizations so performance can be affected.");
	ditherSlider->SetSize(XMFLOAT2(100, 30));
	ditherSlider->SetPos(XMFLOAT2(x, y += 30));
	ditherSlider->OnSlide([&](wiEventArgs args) {
		ObjectComponent* object = wiRenderer::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->color.w = 1 - args.fValue;
		}
	});
	objectWindow->AddWidget(ditherSlider);

	cascadeMaskSlider = new wiSlider(0, 3, 0, 3, "Cascade Mask: ");
	cascadeMaskSlider->SetTooltip("How many shadow cascades to skip when rendering this object into shadow maps? (0: skip none, it will be in all cascades, 1: skip first (biggest cascade), ...etc...");
	cascadeMaskSlider->SetSize(XMFLOAT2(100, 30));
	cascadeMaskSlider->SetPos(XMFLOAT2(x, y += 30));
	cascadeMaskSlider->OnSlide([&](wiEventArgs args) {
		ObjectComponent* object = wiRenderer::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->cascadeMask = (uint32_t)args.iValue;
		}
	});
	objectWindow->AddWidget(cascadeMaskSlider);


	colorPicker = new wiColorPicker(GUI, "Object Color");
	colorPicker->SetPos(XMFLOAT2(10, 30));
	colorPicker->RemoveWidgets();
	colorPicker->SetVisible(true);
	colorPicker->SetEnabled(true);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		ObjectComponent* object = wiRenderer::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->color = XMFLOAT4(powf(args.color.x, 1.f / 2.2f), powf(args.color.y, 1.f / 2.2f), powf(args.color.z, 1.f / 2.2f), object->color.w);
		}
	});
	objectWindow->AddWidget(colorPicker);

	y += 60;

	physicsLabel = new wiLabel("PHYSICSLABEL");
	physicsLabel->SetText("PHYSICS SETTINGS");
	physicsLabel->SetPos(XMFLOAT2(x - 30, y += 30));
	physicsLabel->SetSize(XMFLOAT2(150, 20));
	objectWindow->AddWidget(physicsLabel);



	rigidBodyCheckBox = new wiCheckBox("Rigid Body Physics: ");
	rigidBodyCheckBox->SetTooltip("Enable rigid body physics simulation.");
	rigidBodyCheckBox->SetPos(XMFLOAT2(x, y += 30));
	rigidBodyCheckBox->SetCheck(false);
	rigidBodyCheckBox->OnClick([&](wiEventArgs args) 
	{
		Scene& scene = wiRenderer::GetScene();
		RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				RigidBodyPhysicsComponent& rigidbody = scene.rigidbodies.Create(entity);
				rigidbody.SetKinematic(kinematicCheckBox->GetCheck());
				rigidbody.SetDisableDeactivation(disabledeactivationCheckBox->GetCheck());
				rigidbody.shape = (RigidBodyPhysicsComponent::CollisionShape)collisionShapeComboBox->GetSelected();
			}
		}
		else
		{
			if (physicscomponent != nullptr)
			{
				scene.rigidbodies.Remove(entity);
			}
		}

	});
	objectWindow->AddWidget(rigidBodyCheckBox);

	kinematicCheckBox = new wiCheckBox("Kinematic: ");
	kinematicCheckBox->SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox->SetPos(XMFLOAT2(x, y += 30));
	kinematicCheckBox->SetCheck(false);
	kinematicCheckBox->OnClick([&](wiEventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wiRenderer::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetKinematic(args.bValue);
		}
	});
	objectWindow->AddWidget(kinematicCheckBox);

	disabledeactivationCheckBox = new wiCheckBox("Disable Deactivation: ");
	disabledeactivationCheckBox->SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox->SetPos(XMFLOAT2(x, y += 30));
	disabledeactivationCheckBox->SetCheck(false);
	disabledeactivationCheckBox->OnClick([&](wiEventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wiRenderer::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetDisableDeactivation(args.bValue);
		}
	});
	objectWindow->AddWidget(disabledeactivationCheckBox);

	collisionShapeComboBox = new wiComboBox("Collision Shape:");
	collisionShapeComboBox->SetSize(XMFLOAT2(100, 20));
	collisionShapeComboBox->SetPos(XMFLOAT2(x, y += 30));
	collisionShapeComboBox->AddItem("Box");
	collisionShapeComboBox->AddItem("Sphere");
	collisionShapeComboBox->AddItem("Capsule");
	collisionShapeComboBox->AddItem("Convex Hull");
	collisionShapeComboBox->AddItem("Triangle Mesh");
	collisionShapeComboBox->OnSelect([&](wiEventArgs args) 
	{
		RigidBodyPhysicsComponent* physicscomponent = wiRenderer::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (args.iValue)
			{
			case 0:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::BOX;
				break;
			case 1:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::SPHERE;
				break;
			case 2:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CAPSULE;
				break;
			case 3:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL;
				break;
			case 4:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH;
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

	SetEntity(INVALID_ENTITY);
}


ObjectWindow::~ObjectWindow()
{
	objectWindow->RemoveWidgets(true);
	GUI->RemoveWidget(objectWindow);
	SAFE_DELETE(objectWindow);
}


void ObjectWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wiRenderer::GetScene();

	const ObjectComponent* object = scene.objects.GetComponent(entity);

	if (object != nullptr)
	{
		const NameComponent* name = scene.names.GetComponent(entity);
		if (name != nullptr)
		{
			std::stringstream ss("");
			ss << name->name << " (" << entity << ")";
			nameLabel->SetText(ss.str());
		}
		else
		{
			std::stringstream ss("");
			ss<< "(" << entity << ")";
			nameLabel->SetText(ss.str());
		}

		renderableCheckBox->SetCheck(object->IsRenderable());
		cascadeMaskSlider->SetValue((float)object->cascadeMask);
		ditherSlider->SetValue(object->GetTransparency());

		const RigidBodyPhysicsComponent* physicsComponent = scene.rigidbodies.GetComponent(entity);

		rigidBodyCheckBox->SetCheck(physicsComponent != nullptr);

		if (physicsComponent != nullptr)
		{
			kinematicCheckBox->SetCheck(physicsComponent->IsKinematic());
			disabledeactivationCheckBox->SetCheck(physicsComponent->IsDisableDeactivation());

			if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::BOX)
			{
				collisionShapeComboBox->SetSelected(0);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::SPHERE)
			{
				collisionShapeComboBox->SetSelected(1);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CAPSULE)
			{
				collisionShapeComboBox->SetSelected(2);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL)
			{
				collisionShapeComboBox->SetSelected(3);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH)
			{
				collisionShapeComboBox->SetSelected(4);
			}
		}

		objectWindow->SetEnabled(true);

	}
	else
	{
		objectWindow->SetEnabled(false);
	}

}
