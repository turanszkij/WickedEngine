#include "stdafx.h"
#include "ConstraintWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ConstraintWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_CONSTRAINT " Constraint", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 440));

	closeButton.SetTooltip("Delete PhysicsConstraintComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().constraints.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	infoLabel.Create("ConstraintInfo");
	infoLabel.SetText("Constraints can be added to bind one or two rigid bodies. If only one body is specified, then it will be bound to the constraint's location. If two bodies are specified, they will be bound together with the constraint acting as the pivot between them at the time of binding.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	physicsDebugCheckBox.Create(ICON_EYE " Physics visualizer: ");
	physicsDebugCheckBox.SetTooltip("Visualize the physics world");
	physicsDebugCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::physics::SetDebugDrawEnabled(args.bValue);
		editor->generalWnd.physicsDebugCheckBox.SetCheck(args.bValue);
		editor->componentsWnd.rigidWnd.physicsDebugCheckBox.SetCheck(args.bValue);
	});
	physicsDebugCheckBox.SetCheck(wi::physics::IsDebugDrawEnabled());
	AddWidget(&physicsDebugCheckBox);

	collisionCheckBox.Create("Disable self collision: ");
	collisionCheckBox.SetTooltip("Disable collision between the two bodies that this constraint targets.\nNote: changing this will recreate the constraint in the current pose relative to the bodies.");
	collisionCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->SetDisableSelfCollision(args.bValue);
				physicscomponent->physicsobject = nullptr;
			}
		}
		});
	AddWidget(&collisionCheckBox);

	constraintDebugSlider.Create(0, 10, 1, 100, "Debug size: ");
	constraintDebugSlider.SetValue(wi::physics::GetConstraintDebugSize());
	constraintDebugSlider.OnSlide([](wi::gui::EventArgs args) {
		wi::physics::SetConstraintDebugSize(args.fValue);
		});
	AddWidget(&constraintDebugSlider);

	rebindButton.Create("Rebind Constraint");
	rebindButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&rebindButton);

	typeComboBox.Create("Type: ");
	typeComboBox.AddItem("Fixed", (uint64_t)PhysicsConstraintComponent::Type::Fixed);
	typeComboBox.AddItem("Point", (uint64_t)PhysicsConstraintComponent::Type::Point);
	typeComboBox.AddItem("Distance", (uint64_t)PhysicsConstraintComponent::Type::Distance);
	typeComboBox.AddItem("Hinge", (uint64_t)PhysicsConstraintComponent::Type::Hinge);
	typeComboBox.AddItem("Cone", (uint64_t)PhysicsConstraintComponent::Type::Cone);
	typeComboBox.AddItem("Six DOF", (uint64_t)PhysicsConstraintComponent::Type::SixDOF);
	typeComboBox.AddItem("Swing Twist", (uint64_t)PhysicsConstraintComponent::Type::SwingTwist);
	typeComboBox.AddItem("Slider", (uint64_t)PhysicsConstraintComponent::Type::Slider);
	typeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				PhysicsConstraintComponent::Type type = (PhysicsConstraintComponent::Type)args.userdata;
				if (physicscomponent->type != type)
				{
					physicscomponent->physicsobject = nullptr;
					physicscomponent->type = type;
				}
			}
		}
	});
	typeComboBox.SetSelected(0);
	typeComboBox.SetEnabled(true);
	typeComboBox.SetTooltip("Set constraint type.");
	AddWidget(&typeComboBox);

	bodyAComboBox.Create("Body A: ");
	bodyAComboBox.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->bodyA = args.userdata;
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&bodyAComboBox);

	bodyBComboBox.Create("Body B: ");
	bodyBComboBox.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->bodyB = args.userdata;
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&bodyBComboBox);

	minSlider.Create(0, 10, 1, 100000, "minSlider");
	minSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->type)
				{
				case PhysicsConstraintComponent::Type::Distance:
					physicscomponent->distance_constraint.min_distance = args.fValue;
					break;
				case PhysicsConstraintComponent::Type::Hinge:
					physicscomponent->hinge_constraint.min_angle = wi::math::DegreesToRadians(args.fValue);
					break;
				case PhysicsConstraintComponent::Type::Cone:
					physicscomponent->cone_constraint.half_cone_angle = wi::math::DegreesToRadians(args.fValue);
					break;
				case PhysicsConstraintComponent::Type::SwingTwist:
					physicscomponent->swing_twist.min_twist_angle = wi::math::DegreesToRadians(args.fValue);
					break;
				case PhysicsConstraintComponent::Type::Slider:
					physicscomponent->slider_constraint.min_limit = args.fValue;
					break;
				default:
					break;
				}
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
	});
	AddWidget(&minSlider);

	maxSlider.Create(0, 10, 1, 100000, "maxSlider");
	maxSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->type)
				{
				case PhysicsConstraintComponent::Type::Distance:
					physicscomponent->distance_constraint.max_distance = args.fValue;
					break;
				case PhysicsConstraintComponent::Type::Hinge:
					physicscomponent->hinge_constraint.max_angle = wi::math::DegreesToRadians(args.fValue);
					break;
				case PhysicsConstraintComponent::Type::SwingTwist:
					physicscomponent->swing_twist.max_twist_angle = wi::math::DegreesToRadians(args.fValue);
					break;
				case PhysicsConstraintComponent::Type::Slider:
					physicscomponent->slider_constraint.max_limit = args.fValue;
					break;
				default:
					break;
				}
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
	});
	AddWidget(&maxSlider);

	breakSlider.Create(0, 10, 1, 1000, "Break distance: ");
	breakSlider.SetTooltip("How much the constraint is allowed to be exerted before breaking, calculated as relative distance. Set to FLT_MAX to disable breaking.");
	breakSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->break_distance = args.fValue;
			}
		}
	});
	AddWidget(&breakSlider);

	motorSlider1.Create(0, 10, 1, 100000, "motorSlider1");
	motorSlider1.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->type)
				{
				case PhysicsConstraintComponent::Type::Hinge:
					physicscomponent->hinge_constraint.target_angular_velocity = args.fValue;
					break;
				case PhysicsConstraintComponent::Type::Slider:
					physicscomponent->slider_constraint.target_velocity = args.fValue;
					break;
				default:
					break;
				}
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
	});
	AddWidget(&motorSlider1);

	motorSlider2.Create(0, 10, 1, 100000, "motorSlider2");
	motorSlider2.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				switch (physicscomponent->type)
				{
				case PhysicsConstraintComponent::Type::Slider:
					physicscomponent->slider_constraint.max_force = args.fValue;
					break;
				default:
					break;
				}
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&motorSlider2);


	normalConeSlider.Create(0, 90, 1, 90, "Normal Angle: ");
	normalConeSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->swing_twist.normal_half_cone_angle = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&normalConeSlider);

	planeConeSlider.Create(0, 90, 1, 90, "Plane Angle: ");
	planeConeSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->swing_twist.plane_half_cone_angle = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&planeConeSlider);




	fixedXButton.Create("Fix X");
	fixedXButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedX();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
		});
	AddWidget(&fixedXButton);

	fixedYButton.Create("Fix Y");
	fixedYButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedY();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
		});
	AddWidget(&fixedYButton);

	fixedZButton.Create("Fix Z");
	fixedZButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedZ();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
		});
	AddWidget(&fixedZButton);

	fixedXRotationButton.Create("Fix Rot X");
	fixedXRotationButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedRotationX();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
		});
	AddWidget(&fixedXRotationButton);

	fixedYRotationButton.Create("Fix Rot Y");
	fixedYRotationButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedRotationY();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
	});
	AddWidget(&fixedYRotationButton);

	fixedZRotationButton.Create("Fix Rot Z");
	fixedZRotationButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.SetFixedRotationZ();
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		SetEntity(entity);
	});
	AddWidget(&fixedZRotationButton);


	minTranslationXSlider.Create(-10, 0, 1, 100000, "Min Translation X: ");
	minTranslationXSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minTranslationAxes.x = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minTranslationXSlider);

	minTranslationYSlider.Create(-10, 0, 1, 100000, "Min Translation Y: ");
	minTranslationYSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minTranslationAxes.y = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minTranslationYSlider);

	minTranslationZSlider.Create(-10, 0, 1, 100000, "Min Translation Z: ");
	minTranslationZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minTranslationAxes.z = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minTranslationZSlider);

	maxTranslationXSlider.Create(0, 10, 1, 100000, "Max Translation X: ");
	maxTranslationXSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxTranslationAxes.x = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxTranslationXSlider);

	maxTranslationYSlider.Create(0, 10, 1, 100000, "Max Translation Y: ");
	maxTranslationYSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxTranslationAxes.y = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxTranslationYSlider);

	maxTranslationZSlider.Create(0, 10, 1, 100000, "Max Translation Z: ");
	maxTranslationZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxTranslationAxes.z = args.fValue;
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxTranslationZSlider);



	minRotationXSlider.Create(-180, 0, 1, 100000, "Min Rotation X: ");
	minRotationXSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minRotationAxes.x = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minRotationXSlider);

	minRotationYSlider.Create(-180, 0, 1, 100000, "Min Rotation Y: ");
	minRotationYSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minRotationAxes.y = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minRotationYSlider);

	minRotationZSlider.Create(-180, 0, 1, 100000, "Min Rotation Z: ");
	minRotationZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.minRotationAxes.z = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&minRotationZSlider);

	maxRotationXSlider.Create(0, 180, 1, 100000, "Max Rotation X: ");
	maxRotationXSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxRotationAxes.x = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxRotationXSlider);

	maxRotationYSlider.Create(0, 180, 1, 100000, "Max Rotation Y: ");
	maxRotationYSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxRotationAxes.y = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxRotationYSlider);

	maxRotationZSlider.Create(0, 180, 1, 100000, "Max Rotation Z: ");
	maxRotationZSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			PhysicsConstraintComponent* physicscomponent = scene.constraints.GetComponent(x.entity);
			if (physicscomponent != nullptr)
			{
				physicscomponent->six_dof.maxRotationAxes.z = wi::math::DegreesToRadians(args.fValue);
				physicscomponent->SetRefreshParametersNeeded(true);
			}
		}
		});
	AddWidget(&maxRotationZSlider);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ConstraintWindow::SetEntity(Entity entity)
{
	Scene& scene = editor->GetCurrentScene();

	const PhysicsConstraintComponent* physicsComponent = scene.constraints.GetComponent(entity);

	if (physicsComponent != nullptr)
	{
		this->entity = entity;

		typeComboBox.SetSelectedByUserdataWithoutCallback((uint64_t)physicsComponent->type);

		switch (physicsComponent->type)
		{
		case PhysicsConstraintComponent::Type::Distance:
			minSlider.SetText("Min distance: ");
			maxSlider.SetText("Max distance: ");
			minSlider.SetRange(0, 10);
			maxSlider.SetRange(0, 10);
			minSlider.SetValue(physicsComponent->distance_constraint.min_distance);
			maxSlider.SetValue(physicsComponent->distance_constraint.max_distance);
			break;
		case PhysicsConstraintComponent::Type::Slider:
			minSlider.SetText("Min limit: ");
			maxSlider.SetText("Max limit: ");
			minSlider.SetRange(-10, 0);
			maxSlider.SetRange(0, 10);
			minSlider.SetValue(physicsComponent->slider_constraint.min_limit);
			maxSlider.SetValue(physicsComponent->slider_constraint.max_limit);
			motorSlider1.SetText("Slider Velocity: ");
			motorSlider1.SetRange(-10, 10);
			motorSlider1.SetValue(physicsComponent->slider_constraint.target_velocity);
			motorSlider2.SetText("Force limit: ");
			motorSlider2.SetRange(0, 1000);
			motorSlider2.SetValue(physicsComponent->slider_constraint.max_force);
			break;
		case PhysicsConstraintComponent::Type::Hinge:
			minSlider.SetText("Min angle: ");
			maxSlider.SetText("Max angle: ");
			minSlider.SetRange(-180, 0);
			maxSlider.SetRange(0, 180);
			minSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->hinge_constraint.min_angle));
			maxSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->hinge_constraint.max_angle));
			motorSlider1.SetText("Hinge Velocity: ");
			motorSlider1.SetRange(-10, 10);
			motorSlider1.SetValue(physicsComponent->hinge_constraint.target_angular_velocity);
			break;
		case PhysicsConstraintComponent::Type::Cone:
			minSlider.SetText("Cone half angle: ");
			minSlider.SetRange(0, 90);
			minSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->cone_constraint.half_cone_angle));
			break;
		case PhysicsConstraintComponent::Type::SwingTwist:
			minSlider.SetText("Min twist angle: ");
			maxSlider.SetText("Max twist angle: ");
			minSlider.SetRange(-180, 0);
			maxSlider.SetRange(0, 180);
			minSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->swing_twist.min_twist_angle));
			maxSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->swing_twist.max_twist_angle));
			normalConeSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->swing_twist.normal_half_cone_angle));
			planeConeSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->swing_twist.plane_half_cone_angle));
			break;
		default:
			break;
		}

		breakSlider.SetValue(physicsComponent->break_distance);

		bodyAComboBox.ClearItems();
		bodyAComboBox.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		bodyBComboBox.ClearItems();
		bodyBComboBox.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		for (size_t i = 0; i < scene.rigidbodies.GetCount(); ++i)
		{
			Entity bodyEntity = scene.rigidbodies.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(bodyEntity);
			if (name != nullptr)
			{
				bodyAComboBox.AddItem(name->name, (uint64_t)bodyEntity);
				bodyBComboBox.AddItem(name->name, (uint64_t)bodyEntity);
			}
		}
		bodyAComboBox.SetSelectedByUserdataWithoutCallback(physicsComponent->bodyA);
		bodyBComboBox.SetSelectedByUserdataWithoutCallback(physicsComponent->bodyB);

		collisionCheckBox.SetCheck(physicsComponent->IsDisableSelfCollision());

		minTranslationXSlider.SetValue(physicsComponent->six_dof.minTranslationAxes.x);
		minTranslationYSlider.SetValue(physicsComponent->six_dof.minTranslationAxes.y);
		minTranslationZSlider.SetValue(physicsComponent->six_dof.minTranslationAxes.z);
		maxTranslationXSlider.SetValue(physicsComponent->six_dof.maxTranslationAxes.x);
		maxTranslationYSlider.SetValue(physicsComponent->six_dof.maxTranslationAxes.y);
		maxTranslationZSlider.SetValue(physicsComponent->six_dof.maxTranslationAxes.z);
		minRotationXSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.minRotationAxes.x));
		minRotationYSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.minRotationAxes.y));
		minRotationZSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.minRotationAxes.z));
		maxRotationXSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.maxRotationAxes.x));
		maxRotationYSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.maxRotationAxes.y));
		maxRotationZSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->six_dof.maxRotationAxes.z));
	}
	else
	{
		this->entity = INVALID_ENTITY;
	}

}


void ConstraintWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 145;
	float margin_right = 40;

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

	add_fullwidth(infoLabel);
	add_fullwidth(rebindButton);
	add(typeComboBox);
	add(bodyAComboBox);
	add(bodyBComboBox);
	add_right(collisionCheckBox);


	margin_right = 80;
	add(breakSlider);
	margin_right = 40;


	Scene& scene = editor->GetCurrentScene();
	const PhysicsConstraintComponent* physicsComponent = scene.constraints.GetComponent(entity);
	if (physicsComponent != nullptr)
	{
		motorSlider1.SetVisible(physicsComponent->type == PhysicsConstraintComponent::Type::Hinge || physicsComponent->type == PhysicsConstraintComponent::Type::Slider);
		motorSlider2.SetVisible(physicsComponent->type == PhysicsConstraintComponent::Type::Slider);

		switch (physicsComponent->type)
		{
		case PhysicsConstraintComponent::Type::Distance:
		case PhysicsConstraintComponent::Type::Slider:
		case PhysicsConstraintComponent::Type::Hinge:
			minSlider.SetVisible(true);
			maxSlider.SetVisible(true);

			normalConeSlider.SetVisible(false);
			planeConeSlider.SetVisible(false);

			fixedXButton.SetVisible(false);
			fixedYButton.SetVisible(false);
			fixedZButton.SetVisible(false);
			fixedXRotationButton.SetVisible(false);
			fixedYRotationButton.SetVisible(false);
			fixedZRotationButton.SetVisible(false);
			minTranslationXSlider.SetVisible(false);
			minTranslationYSlider.SetVisible(false);
			minTranslationZSlider.SetVisible(false);
			maxTranslationXSlider.SetVisible(false);
			maxTranslationYSlider.SetVisible(false);
			maxTranslationZSlider.SetVisible(false);
			minRotationXSlider.SetVisible(false);
			minRotationYSlider.SetVisible(false);
			minRotationZSlider.SetVisible(false);
			maxRotationXSlider.SetVisible(false);
			maxRotationYSlider.SetVisible(false);
			maxRotationZSlider.SetVisible(false);
			break;
		case PhysicsConstraintComponent::Type::SwingTwist:
			minSlider.SetVisible(true);
			maxSlider.SetVisible(true);
			normalConeSlider.SetVisible(true);
			planeConeSlider.SetVisible(true);

			fixedXButton.SetVisible(false);
			fixedYButton.SetVisible(false);
			fixedZButton.SetVisible(false);
			fixedXRotationButton.SetVisible(false);
			fixedYRotationButton.SetVisible(false);
			fixedZRotationButton.SetVisible(false);
			minTranslationXSlider.SetVisible(false);
			minTranslationYSlider.SetVisible(false);
			minTranslationZSlider.SetVisible(false);
			maxTranslationXSlider.SetVisible(false);
			maxTranslationYSlider.SetVisible(false);
			maxTranslationZSlider.SetVisible(false);
			minRotationXSlider.SetVisible(false);
			minRotationYSlider.SetVisible(false);
			minRotationZSlider.SetVisible(false);
			maxRotationXSlider.SetVisible(false);
			maxRotationYSlider.SetVisible(false);
			maxRotationZSlider.SetVisible(false);
			break;
		case PhysicsConstraintComponent::Type::Cone:
			minSlider.SetVisible(true);
			maxSlider.SetVisible(false);

			normalConeSlider.SetVisible(false);
			planeConeSlider.SetVisible(false);

			fixedXButton.SetVisible(false);
			fixedYButton.SetVisible(false);
			fixedZButton.SetVisible(false);
			fixedXRotationButton.SetVisible(false);
			fixedYRotationButton.SetVisible(false);
			fixedZRotationButton.SetVisible(false);
			minTranslationXSlider.SetVisible(false);
			minTranslationYSlider.SetVisible(false);
			minTranslationZSlider.SetVisible(false);
			maxTranslationXSlider.SetVisible(false);
			maxTranslationYSlider.SetVisible(false);
			maxTranslationZSlider.SetVisible(false);
			minRotationXSlider.SetVisible(false);
			minRotationYSlider.SetVisible(false);
			minRotationZSlider.SetVisible(false);
			maxRotationXSlider.SetVisible(false);
			maxRotationYSlider.SetVisible(false);
			maxRotationZSlider.SetVisible(false);
			break;
		case PhysicsConstraintComponent::Type::SixDOF:
			minSlider.SetVisible(false);
			maxSlider.SetVisible(false);

			normalConeSlider.SetVisible(false);
			planeConeSlider.SetVisible(false);

			fixedXButton.SetVisible(true);
			fixedYButton.SetVisible(true);
			fixedZButton.SetVisible(true);
			fixedXRotationButton.SetVisible(true);
			fixedYRotationButton.SetVisible(true);
			fixedZRotationButton.SetVisible(true);
			minTranslationXSlider.SetVisible(true);
			minTranslationYSlider.SetVisible(true);
			minTranslationZSlider.SetVisible(true);
			maxTranslationXSlider.SetVisible(true);
			maxTranslationYSlider.SetVisible(true);
			maxTranslationZSlider.SetVisible(true);
			minRotationXSlider.SetVisible(true);
			minRotationYSlider.SetVisible(true);
			minRotationZSlider.SetVisible(true);
			maxRotationXSlider.SetVisible(true);
			maxRotationYSlider.SetVisible(true);
			maxRotationZSlider.SetVisible(true);
			add_fullwidth(fixedXButton);
			add_fullwidth(fixedYButton);
			add_fullwidth(fixedZButton);
			add_fullwidth(fixedXRotationButton);
			add_fullwidth(fixedYRotationButton);
			add_fullwidth(fixedZRotationButton);
			margin_right = 80;
			add(minTranslationXSlider);
			add(minTranslationYSlider);
			add(minTranslationZSlider);
			add(maxTranslationXSlider);
			add(maxTranslationYSlider);
			add(maxTranslationZSlider);
			add(minRotationXSlider);
			add(minRotationYSlider);
			add(minRotationZSlider);
			add(maxRotationXSlider);
			add(maxRotationYSlider);
			add(maxRotationZSlider);
			break;
		default:
			break;
		}

		margin_right = 40;

		if (normalConeSlider.IsVisible())
		{
			add(normalConeSlider);
		}
		if (planeConeSlider.IsVisible())
		{
			add(planeConeSlider);
		}

		if (minSlider.IsVisible())
		{
			add(minSlider);
		}
		if (maxSlider.IsVisible())
		{
			add(maxSlider);
		}

		if (motorSlider1.IsVisible())
		{
			add(motorSlider1);
		}
		if (motorSlider2.IsVisible())
		{
			add(motorSlider2);
		}

		add_right(physicsDebugCheckBox);
		add(constraintDebugSlider);
	}
}
