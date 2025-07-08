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

	auto forEachSelectedPhysicsComponent = [&](auto /* void(RigidBodyPhysicsComponent*, wi::gui::EventArgs) */ func) {
		return [&, func](wi::gui::EventArgs args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(x.entity);
				if (physicscomponent != nullptr)
				{
					func(physicscomponent, args);
				}
			}
		};
	};

	auto forEachSelectedPhysicsComponentWithRefresh = [&](auto /* void(RigidBodyPhysicsComponent*, wi::gui::EventArgs) */ func) {
		return forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
			func(physicscomponent, args);
			physicscomponent->SetRefreshParametersNeeded();
		});
	};

	collisionShapeComboBox.Create("Collision Shape: ");
	collisionShapeComboBox.AddItem("Box", RigidBodyPhysicsComponent::CollisionShape::BOX);
	collisionShapeComboBox.AddItem("Sphere", RigidBodyPhysicsComponent::CollisionShape::SPHERE);
	collisionShapeComboBox.AddItem("Capsule", RigidBodyPhysicsComponent::CollisionShape::CAPSULE);
	collisionShapeComboBox.AddItem("Cylinder", RigidBodyPhysicsComponent::CollisionShape::CYLINDER);
	collisionShapeComboBox.AddItem("Convex Hull", RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL);
	collisionShapeComboBox.AddItem("Triangle Mesh", RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH);
	collisionShapeComboBox.AddItem("Height Field", RigidBodyPhysicsComponent::CollisionShape::HEIGHTFIELD);
	collisionShapeComboBox.OnSelect([=](wi::gui::EventArgs args) {
		forEachSelectedPhysicsComponent([](auto physicscomponent, auto args) {
			RigidBodyPhysicsComponent::CollisionShape shape = (RigidBodyPhysicsComponent::CollisionShape)args.userdata;
			if (physicscomponent->shape != shape)
			{
				physicscomponent->physicsobject = nullptr;
				physicscomponent->shape = shape;
			}
		})(args);
		RefreshShapeType();
	});

	collisionShapeComboBox.SetSelected(0);
	collisionShapeComboBox.SetEnabled(true);
	collisionShapeComboBox.SetTooltip("Set rigid body collision shape.");
	AddWidget(&collisionShapeComboBox);

	XSlider.Create(0, 10, 1, 100000, "XSlider");
	XSlider.OnSlide(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
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
	}));

	ZSlider.SetLocalizationEnabled(false);
	AddWidget(&XSlider);

	YSlider.Create(0, 10, 1, 100000, "YSlider");
	YSlider.OnSlide(forEachSelectedPhysicsComponent([&](auto physicsComponent, auto args) {
		switch (physicsComponent->shape)
		{
		default:
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
			physicsComponent->box.halfextents.y = args.fValue;
			break;
		case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
		case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
			physicsComponent->capsule.radius = args.fValue;
			break;
		}
		physicsComponent->physicsobject = nullptr;
	}));
	ZSlider.SetLocalizationEnabled(false);
	AddWidget(&YSlider);

	ZSlider.Create(0, 10, 1, 100000, "ZSlider");
	ZSlider.OnSlide(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		switch (physicscomponent->shape)
		{
		default:
		case RigidBodyPhysicsComponent::CollisionShape::BOX:
		case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
			physicscomponent->box.halfextents.z = args.fValue;
			break;
		}
		physicscomponent->physicsobject = nullptr;
	}));
	ZSlider.SetLocalizationEnabled(false);
	AddWidget(&ZSlider);

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->mass = args.fValue;
	}));
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.OnSlide(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->friction = args.fValue;
	}));
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.OnSlide(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->restitution = args.fValue;
	}));
	AddWidget(&restitutionSlider);

	lineardampingSlider.Create(0, 1, 0, 100000, "Linear Damping: ");
	lineardampingSlider.SetTooltip("Set the linear damping amount for the physics engine.");
	lineardampingSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->damping_linear = args.fValue;
	}));
	AddWidget(&lineardampingSlider);

	angulardampingSlider.Create(0, 1, 0, 100000, "Angular Damping: ");
	angulardampingSlider.SetTooltip("Set the angular damping amount for the physics engine.");
	angulardampingSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->damping_angular = args.fValue;
	}));
	AddWidget(&angulardampingSlider);

	buoyancySlider.Create(0, 2, 0, 1000, "Buoyancy: ");
	buoyancySlider.SetTooltip("Higher buoyancy will make the bodies float up faster in water.");
	buoyancySlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->buoyancy = args.fValue;
	}));
	AddWidget(&buoyancySlider);

	physicsMeshLODSlider.Create(0, 6, 0, 6, "Use Mesh LOD: ");
	physicsMeshLODSlider.SetTooltip("Specify which LOD to use for triangle mesh physics.");
	physicsMeshLODSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		if (physicscomponent->mesh_lod != uint32_t(args.iValue))
		{
			physicscomponent->physicsobject = nullptr; // will be recreated automatically
			physicscomponent->mesh_lod = uint32_t(args.iValue);
		}
		physicscomponent->mesh_lod = uint32_t(args.iValue);
	}));
	AddWidget(&physicsMeshLODSlider);

	kinematicCheckBox.Create("Kinematic: ");
	kinematicCheckBox.SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox.SetCheck(false);
	kinematicCheckBox.OnClick(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->SetKinematic(args.bValue);
	}));
	AddWidget(&kinematicCheckBox);

	disabledeactivationCheckBox.Create("Disable Deactivation: ");
	disabledeactivationCheckBox.SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox.SetCheck(false);
	disabledeactivationCheckBox.OnClick(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->SetDisableDeactivation(args.bValue);
	}));
	AddWidget(&disabledeactivationCheckBox);

	startDeactivatedCheckBox.Create("Start deactivated: ");
	startDeactivatedCheckBox.SetTooltip("If enabled, the rigid body will start in a deactivated state.\nEven if the body is dynamic (non-kinematic, mass > 0), it will not fall unless interacted with.");
	startDeactivatedCheckBox.SetCheck(false);
	startDeactivatedCheckBox.OnClick(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->SetStartDeactivated(args.bValue);
	}));
	AddWidget(&startDeactivatedCheckBox);

	offsetXSlider.Create(-10, 10, 0, 100000, "Local Offset X: ");
	offsetXSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetXSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->local_offset.x = args.fValue;
	}));
	AddWidget(&offsetXSlider);

	offsetYSlider.Create(-10, 10, 0, 100000, "Local Offset Y: ");
	offsetYSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetYSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->local_offset.y = args.fValue;
	}));
	AddWidget(&offsetYSlider);

	offsetZSlider.Create(-10, 10, 0, 100000, "Local Offset Z: ");
	offsetZSlider.SetTooltip("Set a local offset relative to the object transform");
	offsetZSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->local_offset.z = args.fValue;
	}));
	AddWidget(&offsetZSlider);

	physicsDebugCheckBox.Create(ICON_EYE " Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
		editor->generalWnd.physicsDebugCheckBox.SetCheck(args.bValue);
		editor->componentsWnd.constraintWnd.physicsDebugCheckBox.SetCheck(args.bValue);
	});
	physicsDebugCheckBox.SetCheck(wi::physics::IsDebugDrawEnabled());
	AddWidget(&physicsDebugCheckBox);



	characterCheckBox.Create(ICON_PHYSICS_CHARACTER " Character physics: ");
	characterCheckBox.SetTooltip("Enable physics-driven character");
	characterCheckBox.OnClick(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
				physicscomponent->SetCharacterPhysics(args.bValue);
	}));
	AddWidget(&characterCheckBox);

	characterLabel.Create("CharacterLabel");
	characterLabel.SetText("Notes about physics-driven character:\n- The capsule shape is recommended for a physics character.\n- The friction and mass of the physics character are taken from the standard rigid body parameters.\n- This is different from CharacterComponent which uses custom character movement logic.");
	characterLabel.SetFitTextEnabled(true);
	AddWidget(&characterLabel);

	characterSlopeSlider.Create(0, 90, 50, 90, "Max slope angle: ");
	characterSlopeSlider.SetTooltip("Specify the slope angle that the character can stand on.");
	characterSlopeSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->character.maxSlopeAngle = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&characterSlopeSlider);

	characterGravitySlider.Create(0, 4, 1, 100, "Gravity factor: ");
	characterGravitySlider.SetTooltip("Specify the strength of gravity acting on the character.");
	characterGravitySlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->character.gravityFactor = args.fValue;
	}));
	AddWidget(&characterGravitySlider);



	vehicleLabel.Create("VehicleLabel");
	std::string tips;
	tips += "Vehicle physics tips:\n";
	tips += "- The vehicle's base shape can be configured in the standard shape settings above\n";
	tips += "- A vehicle is always facing forward in +Z direction by default\n";
	tips += "- Check that mass is set higher than the default 1kg, somewhere around ~1000 kg\n";
	tips += "- Enable physics visualizer while editing vehicle parameters\n";
	tips += "- You can reset vehicles by Ctrl + left click on the physics toggle button\n";
	vehicleLabel.SetText(tips);
	vehicleLabel.SetFitTextEnabled(true);
	AddWidget(&vehicleLabel);

	vehicleCombo.Create(ICON_VEHICLE " Vehicle physics: ");
	vehicleCombo.AddItem("None", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::None);
	vehicleCombo.AddItem("Car", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::Car);
	vehicleCombo.AddItem("Motorcycle", (uint64_t)RigidBodyPhysicsComponent::Vehicle::Type::Motorcycle);
	vehicleCombo.OnSelect(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.type = (RigidBodyPhysicsComponent::Vehicle::Type)args.userdata;
	}));
	AddWidget(&vehicleCombo);


	vehicleCollisionCombo.Create("Vehicle collision: ");
	vehicleCollisionCombo.AddItem("Ray", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Ray);
	vehicleCollisionCombo.AddItem("Sphere", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Sphere);
	vehicleCollisionCombo.AddItem("Cylinder", (uint64_t)RigidBodyPhysicsComponent::Vehicle::CollisionMode::Cylinder);
	vehicleCollisionCombo.OnSelect(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.collision_mode = (RigidBodyPhysicsComponent::Vehicle::CollisionMode)args.userdata;
	}));
	AddWidget(&vehicleCollisionCombo);


	wheelRadiusSlider.Create(0.001f, 10, 1, 1000, "Wheel radius: ");
	wheelRadiusSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_radius = args.fValue;
	}));
	AddWidget(&wheelRadiusSlider);

	wheelWidthSlider.Create(0.001f, 10, 1, 1000, "Wheel width: ");
	wheelWidthSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_width = args.fValue;
	}));
	AddWidget(&wheelWidthSlider);

	chassisHalfWidthSlider.Create(0, 10, 1, 1000, "Chassis width: ");
	chassisHalfWidthSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.chassis_half_width = args.fValue;
	}));
	AddWidget(&chassisHalfWidthSlider);

	chassisHalfHeightSlider.Create(-10, 10, 1, 1000, "Chassis height: ");
	chassisHalfHeightSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.chassis_half_height = args.fValue;
	}));
	AddWidget(&chassisHalfHeightSlider);

	chassisHalfLengthSlider.Create(0, 10, 1, 1000, "Chassis length: ");
	chassisHalfLengthSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.chassis_half_length = args.fValue;
	}));
	AddWidget(&chassisHalfLengthSlider);

	frontWheelOffsetSlider.Create(-10, 10, 0, 1000, "Front wheel offset: ");
	frontWheelOffsetSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.front_wheel_offset = args.fValue;
	}));
	AddWidget(&frontWheelOffsetSlider);

	rearWheelOffsetSlider.Create(-10, 10, 0, 1000, "Rear wheel offset: ");
	rearWheelOffsetSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.rear_wheel_offset = args.fValue;
	}));
	AddWidget(&rearWheelOffsetSlider);

	maxTorqueSlider.Create(0, 1000, 0, 1000, "Max Torque: ");
	maxTorqueSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.max_engine_torque = args.fValue;
	}));
	AddWidget(&maxTorqueSlider);

	clutchStrengthSlider.Create(0, 100, 0, 1000, "Clutch Strength: ");
	clutchStrengthSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.clutch_strength = args.fValue;
	}));
	AddWidget(&clutchStrengthSlider);

	maxRollAngleSlider.Create(0, 180, 0, 180, "Max Roll Angle: ");
	maxRollAngleSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.max_roll_angle = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&maxRollAngleSlider);

	maxSteeringAngleSlider.Create(0, 90, 0, 90, "Max Steering Angle: ");
	maxSteeringAngleSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.max_steering_angle = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&maxSteeringAngleSlider);

	fSuspensionMinSlider.Create(0, 2, 0, 200, "F. Susp. Min Length: ");
	fSuspensionMinSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.front_suspension.min_length = args.fValue;
	}));
	AddWidget(&fSuspensionMinSlider);

	fSuspensionMaxSlider.Create(0, 2, 0, 200, "F. Susp. Max Length: ");
	fSuspensionMaxSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.front_suspension.max_length = args.fValue;
		physicscomponent->SetRefreshParametersNeeded();
	}));
	AddWidget(&fSuspensionMaxSlider);

	fSuspensionFrequencySlider.Create(0, 2, 0, 200, "F. Susp. Frequency: ");
	fSuspensionFrequencySlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.front_suspension.frequency = args.fValue;
	}));
	AddWidget(&fSuspensionFrequencySlider);

	fSuspensionDampingSlider.Create(0, 2, 0, 200, "F. Susp. Damping: ");
	fSuspensionDampingSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.front_suspension.damping = args.fValue;
	}));
	AddWidget(&fSuspensionDampingSlider);

	rSuspensionMinSlider.Create(0, 2, 0, 200, "R. Susp. Min Length: ");
	rSuspensionMinSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.rear_suspension.min_length = args.fValue;
	}));
	AddWidget(&rSuspensionMinSlider);

	rSuspensionMaxSlider.Create(0, 2, 0, 200, "R. Susp. Max Length: ");
	rSuspensionMaxSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.rear_suspension.max_length = args.fValue;
	}));
	AddWidget(&rSuspensionMaxSlider);

	rSuspensionFrequencySlider.Create(0, 2, 0, 200, "R. Susp. Frequency: ");
	rSuspensionFrequencySlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.rear_suspension.frequency = args.fValue;
	}));
	AddWidget(&rSuspensionFrequencySlider);

	rSuspensionDampingSlider.Create(0, 2, 0, 200, "R. Susp. Damping: ");
	rSuspensionDampingSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.rear_suspension.damping = args.fValue;
	}));
	AddWidget(&rSuspensionDampingSlider);

	motorcycleFBrakeSuspensionAngleSlider.Create(0, 90, 0, 90, "F. Brake Susp. Angle: ");
	motorcycleFBrakeSuspensionAngleSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.motorcycle.front_suspension_angle = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&motorcycleFBrakeSuspensionAngleSlider);

	motorcycleFBrakeTorqueSlider.Create(0, 2000, 0, 2000, "F. Brake Torque: ");
	motorcycleFBrakeTorqueSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.motorcycle.front_brake_torque = args.fValue;
	}));
	AddWidget(&motorcycleFBrakeTorqueSlider);

	motorcycleRBrakeTorqueSlider.Create(0, 2000, 0, 2000, "R. Brake Torque: ");
	motorcycleRBrakeTorqueSlider.OnSlide(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.motorcycle.rear_brake_torque = args.fValue;
	}));
	AddWidget(&motorcycleRBrakeTorqueSlider);

	fourwheelCheckbox.Create("4-wheel drive: ");
	fourwheelCheckbox.OnClick(forEachSelectedPhysicsComponentWithRefresh([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.car.four_wheel_drive = args.bValue;
	}));
	AddWidget(&fourwheelCheckbox);

	motorleanCheckbox.Create("Motorcycle lean control: ");
	// Here we don't need to recreate physics!!
	motorleanCheckbox.OnClick(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.motorcycle.lean_control = args.bValue;
	}));
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
	wheelEntityFrontLeftCombo.OnSelect(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_entity_front_left = args.userdata;
	}));
	AddWidget(&wheelEntityFrontLeftCombo);

	wheelEntityFrontRightCombo.Create("Front Right Wheel: ");
	wheelEntityFrontRightCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityFrontRightCombo.OnSelect(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_entity_front_right = args.userdata;
	}));
	AddWidget(&wheelEntityFrontRightCombo);

	wheelEntityRearLeftCombo.Create("Rear Left Wheel: ");
	wheelEntityRearLeftCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityRearLeftCombo.OnSelect(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_entity_rear_left = args.userdata;
	}));
	AddWidget(&wheelEntityRearLeftCombo);

	wheelEntityRearRightCombo.Create("Rear Right Wheel: ");
	wheelEntityRearRightCombo.SetTooltip("Map an entity transform to this wheel, so the wheel graphics can be animated by physics (optional).\nThe entity name must contain the word wheel to be displayed here.");
	wheelEntityRearRightCombo.OnSelect(forEachSelectedPhysicsComponent([&](auto physicscomponent, auto args) {
		physicscomponent->vehicle.wheel_entity_rear_right = args.userdata;
	}));
	AddWidget(&wheelEntityRearRightCombo);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void RigidBodyWindow::RefreshShapeType()
{
	Scene& scene = editor->GetCurrentScene();

	const RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);
	if (physicscomponent == nullptr)
		return;

	switch (physicscomponent->shape)
	{
	case RigidBodyPhysicsComponent::CollisionShape::BOX:
		XSlider.SetVisible(true);
		YSlider.SetVisible(true);
		ZSlider.SetVisible(true);
		XSlider.SetText("Width: ");
		YSlider.SetText("Height: ");
		ZSlider.SetText("Depth: ");
		XSlider.SetValue(physicscomponent->box.halfextents.x);
		YSlider.SetValue(physicscomponent->box.halfextents.y);
		ZSlider.SetValue(physicscomponent->box.halfextents.z);
		break;
	case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
		XSlider.SetVisible(true);
		YSlider.SetVisible(false);
		ZSlider.SetVisible(false);
		XSlider.SetText("Radius: ");
		XSlider.SetValue(physicscomponent->sphere.radius);
		break;
	case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
	case RigidBodyPhysicsComponent::CollisionShape::CYLINDER:
		XSlider.SetVisible(true);
		YSlider.SetVisible(true);
		ZSlider.SetVisible(false);
		XSlider.SetText("Height: ");
		YSlider.SetText("Radius: ");
		XSlider.SetValue(physicscomponent->capsule.height);
		YSlider.SetValue(physicscomponent->capsule.radius);
		break;
	case RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL:
	case RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH:
	default:
		XSlider.SetVisible(false);
		YSlider.SetVisible(false);
		ZSlider.SetVisible(false);
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

		characterCheckBox.SetCheck(physicsComponent->IsCharacterPhysics());
		characterSlopeSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->character.maxSlopeAngle));
		characterGravitySlider.SetValue(physicsComponent->character.gravityFactor);

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
	RefreshShapeType();
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 145;

	layout.add(collisionShapeComboBox);
	layout.add(XSlider);
	layout.add(YSlider);
	layout.add(ZSlider);
	layout.add(massSlider);
	layout.add(frictionSlider);
	layout.add(restitutionSlider);
	layout.add(lineardampingSlider);
	layout.add(angulardampingSlider);
	layout.add(buoyancySlider);
	layout.add(physicsMeshLODSlider);
	layout.add(offsetXSlider);
	layout.add(offsetYSlider);
	layout.add(offsetZSlider);
	layout.add_right(startDeactivatedCheckBox);
	layout.add_right(disabledeactivationCheckBox);
	layout.add_right(kinematicCheckBox);
	layout.add_right(physicsDebugCheckBox);


	layout.jump();

	layout.add_right(characterCheckBox);
	if (characterCheckBox.GetCheck())
	{
		characterLabel.SetVisible(true);
		characterSlopeSlider.SetVisible(true);
		characterGravitySlider.SetVisible(true);
		layout.add_fullwidth(characterLabel);
		layout.add(characterSlopeSlider);
		layout.add(characterGravitySlider);
	}
	else
	{
		characterLabel.SetVisible(false);
		characterSlopeSlider.SetVisible(false);
		characterGravitySlider.SetVisible(false);
	}

	layout.jump();

	layout.add(vehicleCombo);
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

		layout.add_fullwidth(vehicleLabel);

		layout.add_right(driveCheckbox);

		layout.add(vehicleCollisionCombo);
		layout.add(wheelRadiusSlider);
		layout.add(wheelWidthSlider);
		layout.add(chassisHalfWidthSlider);
		layout.add(chassisHalfHeightSlider);
		layout.add(chassisHalfLengthSlider);
		layout.add(frontWheelOffsetSlider);
		layout.add(rearWheelOffsetSlider);
		layout.add(maxTorqueSlider);
		layout.add(clutchStrengthSlider);
		layout.add(maxRollAngleSlider);
		layout.add(maxSteeringAngleSlider);

		layout.add(fSuspensionMinSlider);
		layout.add(fSuspensionMaxSlider);
		layout.add(fSuspensionFrequencySlider);
		layout.add(fSuspensionDampingSlider);

		layout.add(rSuspensionMinSlider);
		layout.add(rSuspensionMaxSlider);
		layout.add(rSuspensionFrequencySlider);
		layout.add(rSuspensionDampingSlider);

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
			layout.add(wheelEntityFrontLeftCombo);
			layout.add(wheelEntityFrontRightCombo);
			layout.add(wheelEntityRearLeftCombo);
			layout.add(wheelEntityRearRightCombo);
			fourwheelCheckbox.SetVisible(true);
			motorleanCheckbox.SetVisible(false);
			layout.add_right(fourwheelCheckbox);
			break;
		case wi::scene::RigidBodyPhysicsComponent::Vehicle::Type::Motorcycle:
			motorcycleFBrakeSuspensionAngleSlider.SetVisible(true);
			motorcycleFBrakeTorqueSlider.SetVisible(true);
			motorcycleRBrakeTorqueSlider.SetVisible(true);
			layout.add(motorcycleFBrakeSuspensionAngleSlider);
			layout.add(motorcycleFBrakeTorqueSlider);
			layout.add(motorcycleRBrakeTorqueSlider);

			wheelEntityFrontLeftCombo.SetVisible(true);
			wheelEntityFrontRightCombo.SetVisible(false);
			wheelEntityRearLeftCombo.SetVisible(true);
			wheelEntityRearRightCombo.SetVisible(false);
			layout.add(wheelEntityFrontLeftCombo);
			layout.add(wheelEntityRearLeftCombo);
			motorleanCheckbox.SetVisible(true);
			fourwheelCheckbox.SetVisible(false);
			layout.add_right(motorleanCheckbox);
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
