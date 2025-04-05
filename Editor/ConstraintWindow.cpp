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
	infoLabel.SetSize(XMFLOAT2(100, 150));
	infoLabel.SetText("Constraints can be added to bind one or two rigid bodies. If only one body is specified, then it will be bound to the constraint's location. If two bodies are specified, they will be bound together with the constraint acting as the pivot between them at the time of binding.");
	AddWidget(&infoLabel);

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
				default:
					break;
				}
				physicscomponent->physicsobject = nullptr;
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
				default:
					break;
				}
				physicscomponent->physicsobject = nullptr;
			}
		}
	});
	AddWidget(&maxSlider);


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
		if (this->entity == entity)
			return;
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
		case PhysicsConstraintComponent::Type::Hinge:
			minSlider.SetText("Min angle: ");
			maxSlider.SetText("Max angle: ");
			minSlider.SetRange(-180, 0);
			maxSlider.SetRange(0, 180);
			minSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->hinge_constraint.min_angle));
			maxSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->hinge_constraint.max_angle));
			break;
		case PhysicsConstraintComponent::Type::Cone:
			minSlider.SetText("Cone half angle: ");
			minSlider.SetRange(0, 90);
			minSlider.SetValue(wi::math::RadiansToDegrees(physicsComponent->cone_constraint.half_cone_angle));
			break;
		default:
			break;
		}

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

	add_fullwidth(infoLabel);
	add_fullwidth(rebindButton);
	add(typeComboBox);
	add(bodyAComboBox);
	add(bodyBComboBox);


	Scene& scene = editor->GetCurrentScene();
	const PhysicsConstraintComponent* physicsComponent = scene.constraints.GetComponent(entity);
	if (physicsComponent != nullptr)
	{
		switch (physicsComponent->type)
		{
		case PhysicsConstraintComponent::Type::Distance:
		case PhysicsConstraintComponent::Type::Hinge:
			minSlider.SetVisible(true);
			maxSlider.SetVisible(true);
			break;
		case PhysicsConstraintComponent::Type::Cone:
			minSlider.SetVisible(true);
			maxSlider.SetVisible(false);
			break;
		default:
			minSlider.SetVisible(false);
			maxSlider.SetVisible(false);
			break;
		}

		if (minSlider.IsVisible())
		{
			add(minSlider);
		}
		if (maxSlider.IsVisible())
		{
			add(maxSlider);
		}
	}
}
