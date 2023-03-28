#include "stdafx.h"
#include "RigidBodyWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void RigidBodyWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_RIGIDBODY " Rigid Body Physics", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 300));

	closeButton.SetTooltip("Delete RigidBodyPhysicsComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().rigidbodies.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 140;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 130;


	collisionShapeComboBox.Create("Collision Shape: ");
	collisionShapeComboBox.SetSize(XMFLOAT2(wid, hei));
	collisionShapeComboBox.SetPos(XMFLOAT2(x, y));
	collisionShapeComboBox.AddItem("Box", RigidBodyPhysicsComponent::CollisionShape::BOX);
	collisionShapeComboBox.AddItem("Sphere", RigidBodyPhysicsComponent::CollisionShape::SPHERE);
	collisionShapeComboBox.AddItem("Capsule", RigidBodyPhysicsComponent::CollisionShape::CAPSULE);
	collisionShapeComboBox.AddItem("Convex Hull", RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL);
	collisionShapeComboBox.AddItem("Triangle Mesh", RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH);
	collisionShapeComboBox.OnSelect([&](wi::gui::EventArgs args)
		{
			if (entity == INVALID_ENTITY)
				return;

			Scene& scene = editor->GetCurrentScene();
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);
			if (physicscomponent == nullptr)
				return;

			XSlider.SetEnabled(false);
			YSlider.SetEnabled(false);
			ZSlider.SetEnabled(false);
			XSlider.SetText("-");
			YSlider.SetText("-");
			ZSlider.SetText("-");

			RigidBodyPhysicsComponent::CollisionShape shape = (RigidBodyPhysicsComponent::CollisionShape)args.userdata;
			if (physicscomponent->shape != shape)
			{
				physicscomponent->physicsobject = nullptr;
				physicscomponent->shape = shape;
			}

			switch (shape)
			{
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				XSlider.SetEnabled(true);
				YSlider.SetEnabled(true);
				ZSlider.SetEnabled(true);
				XSlider.SetText("Width");
				YSlider.SetText("Height");
				ZSlider.SetText("Depth");
				XSlider.SetValue(physicscomponent->box.halfextents.x);
				YSlider.SetValue(physicscomponent->box.halfextents.y);
				ZSlider.SetValue(physicscomponent->box.halfextents.z);
				break;
			case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
				XSlider.SetEnabled(true);
				XSlider.SetText("Radius");
				YSlider.SetText("-");
				ZSlider.SetText("-");
				XSlider.SetValue(physicscomponent->sphere.radius);
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::CAPSULE)
				{
					physicscomponent->physicsobject = nullptr;
					physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CAPSULE;
				}
				XSlider.SetEnabled(true);
				YSlider.SetEnabled(true);
				XSlider.SetText("Height");
				YSlider.SetText("Radius");
				ZSlider.SetText("-");
				XSlider.SetValue(physicscomponent->capsule.height);
				YSlider.SetValue(physicscomponent->capsule.radius);
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
			case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
			default:
				break;
			}
		});
	collisionShapeComboBox.SetSelected(0);
	collisionShapeComboBox.SetEnabled(true);
	collisionShapeComboBox.SetTooltip("Set rigid body collision shape.");
	AddWidget(&collisionShapeComboBox);

	XSlider.Create(0, 10, 1, 100000, "X: ");
	XSlider.SetSize(XMFLOAT2(wid, hei));
	XSlider.SetPos(XMFLOAT2(x, y += step));
	XSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.x = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
				physicscomponent->sphere.radius = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				physicscomponent->capsule.height = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&XSlider);

	YSlider.Create(0, 10, 1, 100000, "Y: ");
	YSlider.SetSize(XMFLOAT2(wid, hei));
	YSlider.SetPos(XMFLOAT2(x, y += step));
	YSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.y = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				physicscomponent->capsule.radius = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&YSlider);

	ZSlider.Create(0, 10, 1, 100000, "Z: ");
	ZSlider.SetSize(XMFLOAT2(wid, hei));
	ZSlider.SetPos(XMFLOAT2(x, y += step));
	ZSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.z = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&ZSlider);

	XSlider.SetText("Width");
	YSlider.SetText("Height");
	ZSlider.SetText("Depth");

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.SetSize(XMFLOAT2(wid, hei));
	massSlider.SetPos(XMFLOAT2(x, y += step));
	massSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.SetSize(XMFLOAT2(wid, hei));
	frictionSlider.SetPos(XMFLOAT2(x, y += step));
	frictionSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.SetSize(XMFLOAT2(wid, hei));
	restitutionSlider.SetPos(XMFLOAT2(x, y += step));
	restitutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->restitution = args.fValue;
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&restitutionSlider);

	lineardampingSlider.Create(0, 1, 0, 100000, "Linear Damping: ");
	lineardampingSlider.SetTooltip("Set the linear damping amount for the physics engine.");
	lineardampingSlider.SetSize(XMFLOAT2(wid, hei));
	lineardampingSlider.SetPos(XMFLOAT2(x, y += step));
	lineardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->damping_linear = args.fValue;
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&lineardampingSlider);

	angulardampingSlider.Create(0, 1, 0, 100000, "Angular Damping: ");
	angulardampingSlider.SetTooltip("Set the angular damping amount for the physics engine.");
	angulardampingSlider.SetSize(XMFLOAT2(wid, hei));
	angulardampingSlider.SetPos(XMFLOAT2(x, y += step));
	angulardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->damping_angular = args.fValue;
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&angulardampingSlider);

	physicsMeshLODSlider.Create(0, 6, 0, 6, "Use Mesh LOD: ");
	physicsMeshLODSlider.SetTooltip("Specify which LOD to use for triangle mesh physics.");
	physicsMeshLODSlider.SetSize(XMFLOAT2(wid, hei));
	physicsMeshLODSlider.SetPos(XMFLOAT2(x, y += step));
	physicsMeshLODSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			if (physicscomponent->mesh_lod != uint32_t(args.iValue))
			{
				physicscomponent->physicsobject = nullptr; // will be recreated automatically
				physicscomponent->mesh_lod = uint32_t(args.iValue);
			}
			physicscomponent->mesh_lod = uint32_t(args.iValue);
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&physicsMeshLODSlider);

	kinematicCheckBox.Create("Kinematic: ");
	kinematicCheckBox.SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox.SetSize(XMFLOAT2(hei, hei));
	kinematicCheckBox.SetPos(XMFLOAT2(x, y += step));
	kinematicCheckBox.SetCheck(false);
	kinematicCheckBox.OnClick([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetKinematic(args.bValue);
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&kinematicCheckBox);

	disabledeactivationCheckBox.Create("Disable Deactivation: ");
	disabledeactivationCheckBox.SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledeactivationCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledeactivationCheckBox.SetCheck(false);
	disabledeactivationCheckBox.OnClick([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = editor->GetCurrentScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetDisableDeactivation(args.bValue);
			physicscomponent->physicsobject = {};
		}
		});
	AddWidget(&disabledeactivationCheckBox);



	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


void RigidBodyWindow::SetEntity(Entity entity)
{
	Scene& scene = editor->GetCurrentScene();

	const RigidBodyPhysicsComponent* physicsComponent = scene.rigidbodies.GetComponent(entity);

	if (physicsComponent != nullptr)
	{
		if (this->entity == entity)
			return;
		this->entity = entity;
		massSlider.SetValue(physicsComponent->mass);
		frictionSlider.SetValue(physicsComponent->friction);
		restitutionSlider.SetValue(physicsComponent->restitution);
		lineardampingSlider.SetValue(physicsComponent->damping_linear);
		angulardampingSlider.SetValue(physicsComponent->damping_angular);
		physicsMeshLODSlider.SetValue(float(physicsComponent->mesh_lod));

		kinematicCheckBox.SetCheck(physicsComponent->IsKinematic());
		disabledeactivationCheckBox.SetCheck(physicsComponent->IsDisableDeactivation());

		collisionShapeComboBox.SetSelectedByUserdata((uint64_t)physicsComponent->shape);
	}
	else
	{
		this->entity = INVALID_ENTITY;
	}

}


void RigidBodyWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 120;
	const float margin_right = 40;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add(collisionShapeComboBox);
	add(XSlider);
	add(YSlider);
	add(ZSlider);
	add(massSlider);
	add(frictionSlider);
	add(restitutionSlider);
	add(lineardampingSlider);
	add(angulardampingSlider);
	add(physicsMeshLODSlider);
	add_right(disabledeactivationCheckBox);
	add_right(kinematicCheckBox);

}
