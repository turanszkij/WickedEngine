#include "stdafx.h"
#include "RigidBodyWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void RigidBodyWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_RIGIDBODY " Rigid Body Physics", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 440));

	closeButton.SetTooltip("Delete RigidBodyPhysicsComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().rigidbodies.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	collisionShapeComboBox.Create("Collision Shape: ");
	collisionShapeComboBox.AddItem("Box", RigidBodyPhysicsComponent::CollisionShape::BOX);
	collisionShapeComboBox.AddItem("Sphere", RigidBodyPhysicsComponent::CollisionShape::SPHERE);
	collisionShapeComboBox.AddItem("Capsule", RigidBodyPhysicsComponent::CollisionShape::CAPSULE);
	collisionShapeComboBox.AddItem("Cylinder", RigidBodyPhysicsComponent::CollisionShape::CYLINDER);
	collisionShapeComboBox.AddItem("Convex Hull", RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL);
	collisionShapeComboBox.AddItem("Triangle Mesh", RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH);
	collisionShapeComboBox.AddItem("Height Field", RigidBodyPhysicsComponent::CollisionShape::HEIGHTFIELD);
	collisionShapeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				RigidBodyPhysicsComponent::CollisionShape shape = (RigidBodyPhysicsComponent::CollisionShape)args.userdata;
				if (physicscomponent->shape != shape)
				{
					physicscomponent->physicsobject = nullptr;
					physicscomponent->shape = shape;
				}
			}
		}
		RefreshShapeType();
	});
	collisionShapeComboBox.SetSelected(0);
	collisionShapeComboBox.SetEnabled(true);
	collisionShapeComboBox.SetTooltip("Set rigid body collision shape.");
	AddWidget(&collisionShapeComboBox);

	XSlider.Create(0, 10, 1, 100000, "X: ");
	XSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
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
				case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
					physicscomponent->capsule.height = args.fValue;
					break;
				}
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&XSlider);

	YSlider.Create(0, 10, 1, 100000, "Y: ");
	YSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->shape)
				{
				default:
				case RigidBodyPhysicsComponent::CollisionShape::BOX:
					physicscomponent->box.halfextents.y = args.fValue;
					break;
				case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
					physicscomponent->capsule.radius = args.fValue;
					break;
				}
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&YSlider);

	ZSlider.Create(0, 10, 1, 100000, "Z: ");
	ZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->shape)
				{
				default:
				case RigidBodyPhysicsComponent::CollisionShape::BOX:
				case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
					physicscomponent->box.halfextents.z = args.fValue;
					break;
				}
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&ZSlider);

	XSlider.SetText("Width");
	YSlider.SetText("Height");
	ZSlider.SetText("Depth");

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->mass = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->friction = args.fValue;
			}
		}
	});
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->restitution = args.fValue;
			}
		}
	});
	AddWidget(&restitutionSlider);

	lineardampingSlider.Create(0, 1, 0, 100000, "Linear Damping: ");
	lineardampingSlider.SetTooltip("Set the linear damping amount for the physics engine.");
	lineardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->damping_linear = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&lineardampingSlider);

	angulardampingSlider.Create(0, 1, 0, 100000, "Angular Damping: ");
	angulardampingSlider.SetTooltip("Set the angular damping amount for the physics engine.");
	angulardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->damping_angular = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&angulardampingSlider);

	buoyancySlider.Create(0, 2, 0, 1000, "Buoyancy: ");
	buoyancySlider.SetTooltip("Higher buoyancy will make the bodies float up faster in water.");
	buoyancySlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->buoyancy = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&buoyancySlider);

	physicsMeshLODSlider.Create(0, 6, 0, 6, "Use Mesh LOD: ");
	physicsMeshLODSlider.SetTooltip("Specify which LOD to use for triangle mesh physics.");
	physicsMeshLODSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				if (physicscomponent->mesh_lod != uint32_t(args.iValue))
				{
					physicscomponent->physicsobject = nullptr; // will be recreated automatically
					physicscomponent->mesh_lod = uint32_t(args.iValue);
				}
				physicscomponent->mesh_lod = uint32_t(args.iValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&physicsMeshLODSlider);

	kinematicCheckBox.Create("Kinematic: ");
	kinematicCheckBox.SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox.SetCheck(false);
	kinematicCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->SetKinematic(args.bValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&kinematicCheckBox);

	disabledeactivationCheckBox.Create("Disable Deactivation: ");
	disabledeactivationCheckBox.SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox.SetCheck(false);
	disabledeactivationCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->SetDisableDeactivation(args.bValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&disabledeactivationCheckBox);

	startDeactivatedCheckBox.Create("Start deactivated: ");
	startDeactivatedCheckBox.SetTooltip("If enabled, the rigid body will start in a deactivated state.\nEven if the body is dynamic (non-kinematic, mass > 0), it will not fall unless interacted with.");
	startDeactivatedCheckBox.SetCheck(false);
	startDeactivatedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->SetStartDeactivated(args.bValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&startDeactivatedCheckBox);



	offsetXSlider.Create(-10, 10, 0, 100000, "Local Offset X: ");
	offsetXSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetXSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->local_offset.x = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&offsetXSlider);

	offsetYSlider.Create(-10, 10, 0, 100000, "Local Offset Y: ");
	offsetYSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetYSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->local_offset.y = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&offsetYSlider);

	offsetZSlider.Create(-10, 10, 0, 100000, "Local Offset Z: ");
	offsetZSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->local_offset.z = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&offsetZSlider);

	physicsDebugCheckBox.Create(ICON_EYE " Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
		editor->generalWnd.physicsDebugCheckBox.SetCheck(args.bValue);
	});
	physicsDebugCheckBox.SetCheck(wi::physics::IsDebugDrawEnabled());
	AddWidget(&physicsDebugCheckBox);


	vehicleLabel.Create("VehicleLabel");
	std::string tips;
	tips += "Vehicle physics tips:\n";
	tips += "- The vehicle's base shape can be configured in the standard shape settings above\n";
	tips += "- A vehicle is always facing forward in +Z direction by default\n";
	tips += "- Check that mass is set higher than the default 1kg, somewhere around ~1000 kg\n";
	tips += "- Enable physics visualizer while editing vehicle parameters\n";
	tips += "- You can reset vehicles by Ctrl + left click on the physics toggle button\n";
	vehicleLabel.SetText(tips);
	vehicleLabel.SetSize(XMFLOAT2(100, 240));
	AddWidget(&vehicleLabel);

	vehicleCombo.Create("Vehicle physics: ");
	vehicleCombo.AddItem("None", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::None);
	vehicleCombo.AddItem("Car", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::Car);
	vehicleCombo.AddItem("Motorcycle", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::Motorcycle);
	vehicleCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.type = (RigidBodyPhysicsComponent::Vehicle::Type)args.userdata;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&vehicleCombo);


	vehicleCollisionCombo.Create("Vehicle collision: ");
	vehicleCollisionCombo.AddItem("Ray", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Ray);
	vehicleCollisionCombo.AddItem("Sphere", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Sphere);
	vehicleCollisionCombo.AddItem("Cylinder", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Cylinder);
	vehicleCollisionCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.collision_mode = (RigidBodyPhysicsComponent::Vehicle::CollisionMode)args.userdata;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&vehicleCollisionCombo);


	wheelRadiusSlider.Create(0.001f, 10, 1, 1000, "Wheel radius: ");
	wheelRadiusSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_radius = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&wheelRadiusSlider);

	wheelWidthSlider.Create(0.001f, 10, 1, 1000, "Wheel width: ");
	wheelWidthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_width = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&wheelWidthSlider);

	chassisHalfWidthSlider.Create(0, 10, 1, 1000, "Chassis width: ");
	chassisHalfWidthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.chassis_half_width = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&chassisHalfWidthSlider);

	chassisHalfHeightSlider.Create(-10, 10, 1, 1000, "Chassis height: ");
	chassisHalfHeightSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.chassis_half_height = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&chassisHalfHeightSlider);

	chassisHalfLengthSlider.Create(0, 10, 1, 1000, "Chassis length: ");
	chassisHalfLengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.chassis_half_length = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&chassisHalfLengthSlider);

	frontWheelOffsetSlider.Create(-10, 10, 0, 1000, "Front wheel offset: ");
	frontWheelOffsetSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.front_wheel_offset = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&frontWheelOffsetSlider);

	rearWheelOffsetSlider.Create(-10, 10, 0, 1000, "Rear wheel offset: ");
	rearWheelOffsetSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.rear_wheel_offset = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&rearWheelOffsetSlider);

	maxTorqueSlider.Create(0, 1000, 0, 1000, "Max Torque: ");
	maxTorqueSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.max_engine_torque = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&maxTorqueSlider);

	clutchStrengthSlider.Create(0, 100, 0, 1000, "Clutch Strength: ");
	clutchStrengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.clutch_strength = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&clutchStrengthSlider);

	maxRollAngleSlider.Create(0, 180, 0, 180, "Max Roll Angle: ");
	maxRollAngleSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.max_roll_angle = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&maxRollAngleSlider);

	maxSteeringAngleSlider.Create(0, 90, 0, 90, "Max Steering Angle: ");
	maxSteeringAngleSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.max_steering_angle = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&maxSteeringAngleSlider);

	fSuspensionMinSlider.Create(0, 2, 0, 200, "F. Susp. Min Length: ");
	fSuspensionMinSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.front_suspension.min_length;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&fSuspensionMinSlider);

	fSuspensionMaxSlider.Create(0, 2, 0, 200, "F. Susp. Max Length: ");
	fSuspensionMaxSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.front_suspension.max_length;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&fSuspensionMaxSlider);

	fSuspensionFrequencySlider.Create(0, 2, 0, 200, "F. Susp. Frequency: ");
	fSuspensionFrequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.front_suspension.frequency;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&fSuspensionFrequencySlider);

	fSuspensionDampingSlider.Create(0, 2, 0, 200, "F. Susp. Damping: ");
	fSuspensionDampingSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.front_suspension.damping;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&fSuspensionDampingSlider);

	rSuspensionMinSlider.Create(0, 2, 0, 200, "R. Susp. Min Length: ");
	rSuspensionMinSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.rear_suspension.min_length;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&rSuspensionMinSlider);

	rSuspensionMaxSlider.Create(0, 2, 0, 200, "R. Susp. Max Length: ");
	rSuspensionMaxSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.rear_suspension.max_length;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&rSuspensionMaxSlider);

	rSuspensionFrequencySlider.Create(0, 2, 0, 200, "R. Susp. Frequency: ");
	rSuspensionFrequencySlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.rear_suspension.frequency;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&rSuspensionFrequencySlider);

	rSuspensionDampingSlider.Create(0, 2, 0, 200, "R. Susp. Damping: ");
	rSuspensionDampingSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.rear_suspension.damping;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&rSuspensionDampingSlider);

	motorcycleFBrakeSuspensionAngleSlider.Create(0, 90, 0, 90, "F. Brake Susp. Angle: ");
	motorcycleFBrakeSuspensionAngleSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.motorcycle.front_suspension_angle = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&motorcycleFBrakeSuspensionAngleSlider);

	motorcycleFBrakeTorqueSlider.Create(0, 2000, 0, 2000, "F. Brake Torque: ");
	motorcycleFBrakeTorqueSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.motorcycle.front_brake_torque = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&motorcycleFBrakeTorqueSlider);

	motorcycleRBrakeTorqueSlider.Create(0, 2000, 0, 2000, "R. Brake Torque: ");
	motorcycleRBrakeTorqueSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.motorcycle.rear_brake_torque = args.fValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
		});
	AddWidget(&motorcycleRBrakeTorqueSlider);

	fourwheelCheckbox.Create("4-wheel drive: ");
	fourwheelCheckbox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.car.four_wheel_drive = args.bValue;
				physicscomponent->SetRefreshParametersNeeded();
			}
		}
	});
	AddWidget(&fourwheelCheckbox);

	motorleanCheckbox.Create("Motorcycle lean control: ");
	motorleanCheckbox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.motorcycle.lean_control = args.bValue;
				// Here we don't need to recreate physics!!
			}
		}
	});
	AddWidget(&motorleanCheckbox);

	driveCheckbox.Create(ICON_VEHICLE " Drive in Editor: ");
	driveCheckbox.SetTooltip("Override editor controls to drive this vehicle.\nControls: WASD for driving, left-right for camera rotation, Left Shift: brake, Space: handbrake\nController: R2: forward, L2: backward, Square/X: handbrake.");
	driveCheckbox.OnClick([=](wi::gui::EventArgs args) {
		if (args.bValue)
		{
			driving_entity = entity;
		}
		else
		{
			driving_entity = INVALID_ENTITY;
		}
	});
	AddWidget(&driveCheckbox);


	wheelEntityFrontLeftCombo.Create("Front Left Wheel: ");
	wheelEntityFrontLeftCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityFrontLeftCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_entity_front_left = args.userdata;
			}
		}
	});
	AddWidget(&wheelEntityFrontLeftCombo);

	wheelEntityFrontRightCombo.Create("Front Right Wheel: ");
	wheelEntityFrontRightCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityFrontRightCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_entity_front_right = args.userdata;
			}
		}
		});
	AddWidget(&wheelEntityFrontRightCombo);

	wheelEntityRearLeftCombo.Create("Rear Left Wheel: ");
	wheelEntityRearLeftCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityRearLeftCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_entity_rear_left = args.userdata;
			}
		}
		});
	AddWidget(&wheelEntityRearLeftCombo);

	wheelEntityRearRightCombo.Create("Rear Right Wheel: ");
	wheelEntityRearRightCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityRearRightCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->vehicle.wheel_entity_rear_right = args.userdata;
			}
		}
		});
	AddWidget(&wheelEntityRearRightCombo);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void RigidBodyWindow::RefreshShapeType()
{
	XSlider.SetEnabled(false);
	YSlider.SetEnabled(false);
	ZSlider.SetEnabled(false);
	XSlider.SetText("-");
	YSlider.SetText("-");
	ZSlider.SetText("-");

	Scene& scene = editor->GetCurrentScene();

	const RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);
	if (physicscomponent == nullptr)
		return;

	switch (physicscomponent->shape)
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
	case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
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
		buoyancySlider.SetValue(physicsComponent->buoyancy);
		physicsMeshLODSlider.SetValue(float(physicsComponent->mesh_lod));

		offsetXSlider.SetValue(physicsComponent->local_offset.x);
		offsetYSlider.SetValue(physicsComponent->local_offset.y);
		offsetZSlider.SetValue(physicsComponent->local_offset.z);

		kinematicCheckBox.SetCheck(physicsComponent->IsKinematic());
		disabledeactivationCheckBox.SetCheck(physicsComponent->IsDisableDeactivation());
		startDeactivatedCheckBox.SetCheck(physicsComponent->IsStartDeactivated());

		collisionShapeComboBox.SetSelectedByUserdataWithoutCallback((uint64_t)physicsComponent->shape);

		vehicleCombo.SetSelectedByUserdataWithoutCallback((uint64_t)physicsComponent->vehicle.type);
		vehicleCollisionCombo.SetSelectedByUserdataWithoutCallback((uint64_t)physicsComponent->vehicle.collision_mode);

		wheelRadiusSlider.SetValue(physicsComponent->vehicle.wheel_radius);
		wheelWidthSlider.SetValue(physicsComponent->vehicle.wheel_width);
		chassisHalfWidthSlider.SetValue(physicsComponent->vehicle.chassis_half_width);
		chassisHalfHeightSlider.SetValue(physicsComponent->vehicle.chassis_half_height);
		chassisHalfLengthSlider.SetValue(physicsComponent->vehicle.chassis_half_length);
		frontWheelOffsetSlider.SetValue(physicsComponent->vehicle.front_wheel_offset);
		rearWheelOffsetSlider.SetValue(physicsComponent->vehicle.rear_wheel_offset);
		maxTorqueSlider.SetValue(physicsComponent->vehicle.max_engine_torque);
		clutchStrengthSlider.SetValue(physicsComponent->vehicle.clutch_strength);
		maxRollAngleSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->vehicle.max_roll_angle));
		maxSteeringAngleSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->vehicle.max_steering_angle));
		fourwheelCheckbox.SetCheck(physicsComponent->vehicle.car.four_wheel_drive);
		motorleanCheckbox.SetCheck(physicsComponent->vehicle.motorcycle.lean_control);

		fSuspensionMinSlider.SetValue(physicsComponent->vehicle.front_suspension.min_length);
		fSuspensionMaxSlider.SetValue(physicsComponent->vehicle.front_suspension.max_length);
		fSuspensionFrequencySlider.SetValue(physicsComponent->vehicle.front_suspension.frequency);
		fSuspensionDampingSlider.SetValue(physicsComponent->vehicle.front_suspension.damping);

		rSuspensionMinSlider.SetValue(physicsComponent->vehicle.rear_suspension.min_length);
		rSuspensionMaxSlider.SetValue(physicsComponent->vehicle.rear_suspension.max_length);
		rSuspensionFrequencySlider.SetValue(physicsComponent->vehicle.rear_suspension.frequency);
		rSuspensionDampingSlider.SetValue(physicsComponent->vehicle.rear_suspension.damping);

		motorcycleFBrakeSuspensionAngleSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->vehicle.motorcycle.front_suspension_angle));
		motorcycleFBrakeTorqueSlider.SetValue(physicsComponent->vehicle.motorcycle.front_brake_torque);
		motorcycleRBrakeTorqueSlider.SetValue(physicsComponent->vehicle.motorcycle.rear_brake_torque);


		wheelEntityFrontLeftCombo.ClearItems();
		wheelEntityFrontLeftCombo.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		wheelEntityFrontRightCombo.ClearItems();
		wheelEntityFrontRightCombo.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		wheelEntityRearLeftCombo.ClearItems();
		wheelEntityRearLeftCombo.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		wheelEntityRearRightCombo.ClearItems();
		wheelEntityRearRightCombo.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			Entity wheelEntity = scene.transforms.GetEntity(i);
			if (!scene.Entity_IsDescendant(wheelEntity, entity))
				continue;
			const NameComponent* name = scene.names.GetComponent(wheelEntity);
			if (name != nullptr && wi::helper::toUpper(name->name).find("WHEEL") != std::string::npos)
			{
				wheelEntityFrontLeftCombo.AddItem(name->name, (uint64_t)wheelEntity);
				wheelEntityFrontRightCombo.AddItem(name->name, (uint64_t)wheelEntity);
				wheelEntityRearLeftCombo.AddItem(name->name, (uint64_t)wheelEntity);
				wheelEntityRearRightCombo.AddItem(name->name, (uint64_t)wheelEntity);
			}
		}
		wheelEntityFrontLeftCombo.SetSelectedByUserdataWithoutCallback(physicsComponent->vehicle.wheel_entity_front_left);
		wheelEntityFrontRightCombo.SetSelectedByUserdataWithoutCallback(physicsComponent->vehicle.wheel_entity_front_right);
		wheelEntityRearLeftCombo.SetSelectedByUserdataWithoutCallback(physicsComponent->vehicle.wheel_entity_rear_left);
		wheelEntityRearRightCombo.SetSelectedByUserdataWithoutCallback(physicsComponent->vehicle.wheel_entity_rear_right);

		RefreshShapeType();
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

	const float margin_left = 145;
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
	add(buoyancySlider);
	add(physicsMeshLODSlider);
	add(offsetXSlider);
	add(offsetYSlider);
	add(offsetZSlider);
	add_right(startDeactivatedCheckBox);
	add_right(disabledeactivationCheckBox);
	add_right(kinematicCheckBox);
	add_right(physicsDebugCheckBox);

	y += 20;

	add(vehicleCombo);
	if (vehicleCombo.GetSelected() > 0)
	{
		vehicleLabel.SetVisible(true);
		vehicleCollisionCombo.SetVisible(true);
		wheelRadiusSlider.SetVisible(true);
		wheelWidthSlider.SetVisible(true);
		chassisHalfWidthSlider.SetVisible(true);
		chassisHalfHeightSlider.SetVisible(true);
		chassisHalfLengthSlider.SetVisible(true);
		frontWheelOffsetSlider.SetVisible(true);
		rearWheelOffsetSlider.SetVisible(true);
		maxTorqueSlider.SetVisible(true);
		clutchStrengthSlider.SetVisible(true);
		maxRollAngleSlider.SetVisible(true);
		maxSteeringAngleSlider.SetVisible(true);
		driveCheckbox.SetVisible(true);

		fSuspensionMinSlider.SetVisible(true);
		fSuspensionMaxSlider.SetVisible(true);
		fSuspensionFrequencySlider.SetVisible(true);
		fSuspensionDampingSlider.SetVisible(true);
		rSuspensionMinSlider.SetVisible(true);
		rSuspensionMaxSlider.SetVisible(true);
		rSuspensionFrequencySlider.SetVisible(true);
		rSuspensionDampingSlider.SetVisible(true);

		add_fullwidth(vehicleLabel);

		add_right(driveCheckbox);

		add(vehicleCollisionCombo);
		add(wheelRadiusSlider);
		add(wheelWidthSlider);
		add(chassisHalfWidthSlider);
		add(chassisHalfHeightSlider);
		add(chassisHalfLengthSlider);
		add(frontWheelOffsetSlider);
		add(rearWheelOffsetSlider);
		add(maxTorqueSlider);
		add(clutchStrengthSlider);
		add(maxRollAngleSlider);
		add(maxSteeringAngleSlider);

		add(fSuspensionMinSlider);
		add(fSuspensionMaxSlider);
		add(fSuspensionFrequencySlider);
		add(fSuspensionDampingSlider);

		add(rSuspensionMinSlider);
		add(rSuspensionMaxSlider);
		add(rSuspensionFrequencySlider);
		add(rSuspensionDampingSlider);

		RigidBodyPhysicsComponent::Vehicle::Type type = (RigidBodyPhysicsComponent::Vehicle::Type)vehicleCombo.GetSelectedUserdata();
		switch (type)
		{
		case wi::scene::RigidBodyPhysicsComponent::Vehicle::Type::Car:
			motorcycleFBrakeSuspensionAngleSlider.SetVisible(false);
			motorcycleFBrakeTorqueSlider.SetVisible(false);
			motorcycleRBrakeTorqueSlider.SetVisible(false);

			wheelEntityFrontLeftCombo.SetVisible(true);
			wheelEntityFrontRightCombo.SetVisible(true);
			wheelEntityRearLeftCombo.SetVisible(true);
			wheelEntityRearRightCombo.SetVisible(true);
			add(wheelEntityFrontLeftCombo);
			add(wheelEntityFrontRightCombo);
			add(wheelEntityRearLeftCombo);
			add(wheelEntityRearRightCombo);
			fourwheelCheckbox.SetVisible(true);
			motorleanCheckbox.SetVisible(false);
			add_right(fourwheelCheckbox);
			break;
		case wi::scene::RigidBodyPhysicsComponent::Vehicle::Type::Motorcycle:
			motorcycleFBrakeSuspensionAngleSlider.SetVisible(true);
			motorcycleFBrakeTorqueSlider.SetVisible(true);
			motorcycleRBrakeTorqueSlider.SetVisible(true);
			add(motorcycleFBrakeSuspensionAngleSlider);
			add(motorcycleFBrakeTorqueSlider);
			add(motorcycleRBrakeTorqueSlider);

			wheelEntityFrontLeftCombo.SetVisible(true);
			wheelEntityFrontRightCombo.SetVisible(false);
			wheelEntityRearLeftCombo.SetVisible(true);
			wheelEntityRearRightCombo.SetVisible(false);
			add(wheelEntityFrontLeftCombo);
			add(wheelEntityRearLeftCombo);
			motorleanCheckbox.SetVisible(true);
			fourwheelCheckbox.SetVisible(false);
			add_right(motorleanCheckbox);
			break;
		default:
			break;
		}
	}
	else
	{
		vehicleLabel.SetVisible(false);
		vehicleCollisionCombo.SetVisible(false);
		wheelRadiusSlider.SetVisible(false);
		wheelWidthSlider.SetVisible(false);
		chassisHalfWidthSlider.SetVisible(false);
		chassisHalfHeightSlider.SetVisible(false);
		chassisHalfLengthSlider.SetVisible(false);
		frontWheelOffsetSlider.SetVisible(false);
		rearWheelOffsetSlider.SetVisible(false);
		maxTorqueSlider.SetVisible(false);
		clutchStrengthSlider.SetVisible(false);
		maxRollAngleSlider.SetVisible(false);
		maxSteeringAngleSlider.SetVisible(false);
		wheelEntityFrontLeftCombo.SetVisible(false);
		wheelEntityFrontRightCombo.SetVisible(false);
		wheelEntityRearLeftCombo.SetVisible(false);
		wheelEntityRearRightCombo.SetVisible(false);
		fourwheelCheckbox.SetVisible(false);
		motorleanCheckbox.SetVisible(false);
		driveCheckbox.SetVisible(false);

		fSuspensionMinSlider.SetVisible(false);
		fSuspensionMaxSlider.SetVisible(false);
		fSuspensionFrequencySlider.SetVisible(false);
		fSuspensionDampingSlider.SetVisible(false);
		rSuspensionMinSlider.SetVisible(false);
		rSuspensionMaxSlider.SetVisible(false);
		rSuspensionFrequencySlider.SetVisible(false);
		rSuspensionDampingSlider.SetVisible(false);

		motorcycleFBrakeSuspensionAngleSlider.SetVisible(false);
		motorcycleFBrakeTorqueSlider.SetVisible(false);
		motorcycleRBrakeTorqueSlider.SetVisible(false);
	}

}
