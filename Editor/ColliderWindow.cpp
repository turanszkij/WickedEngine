#include "stdafx.h"
#include "ColliderWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ColliderWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_COLLIDER " Collider", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 340));

	closeButton.SetTooltip("Delete ColliderComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().colliders.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	infoLabel.Create("");
	infoLabel.SetText("Colliders are used for simple fake physics, without using the physics engine. They are only used in specific CPU/GPU systems.");
	infoLabel.SetSize(XMFLOAT2(100, 50));
	AddWidget(&infoLabel);

	cpuCheckBox.Create("CPU: ");
	cpuCheckBox.SetTooltip("Enable for use on the CPU. CPU usage includes: springs.");
	cpuCheckBox.SetSize(XMFLOAT2(hei, hei));
	cpuCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->SetCPUEnabled(args.bValue);
		}
	});
	AddWidget(&cpuCheckBox);

	gpuCheckBox.Create("GPU: ");
	gpuCheckBox.SetTooltip("Enable for use on the GPU. GPU usage includes: emitter and hair particle systems.\nNote that GPU can support only a limited amount of colliders.");
	gpuCheckBox.SetSize(XMFLOAT2(hei, hei));
	gpuCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->SetGPUEnabled(args.bValue);
		}
	});
	AddWidget(&gpuCheckBox);

	shapeCombo.Create("Shape: ");
	shapeCombo.SetSize(XMFLOAT2(wid, hei));
	shapeCombo.SetPos(XMFLOAT2(x, y));
	shapeCombo.AddItem("Sphere", (uint64_t)ColliderComponent::Shape::Sphere);
	shapeCombo.AddItem("Capsule", (uint64_t)ColliderComponent::Shape::Capsule);
	shapeCombo.AddItem("Plane", (uint64_t)ColliderComponent::Shape::Plane);
	shapeCombo.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->shape = (ColliderComponent::Shape)args.userdata;
		}
	});
	AddWidget(&shapeCombo);

	radiusSlider.Create(0, 10, 0, 100000, "Radius: ");
	radiusSlider.SetSize(XMFLOAT2(wid, hei));
	radiusSlider.SetPos(XMFLOAT2(x, y += step));
	radiusSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->radius = args.fValue;
		}
	});
	AddWidget(&radiusSlider);


	y += step;


	offsetX.Create(-10, 10, 0, 10000, "Offset X: ");
	offsetX.SetSize(XMFLOAT2(wid, hei));
	offsetX.SetPos(XMFLOAT2(x, y += step));
	offsetX.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->offset.x = args.fValue;
		}
	});
	AddWidget(&offsetX);

	offsetY.Create(-10, 10, 0, 10000, "Offset Y: ");
	offsetY.SetSize(XMFLOAT2(wid, hei));
	offsetY.SetPos(XMFLOAT2(x, y += step));
	offsetY.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->offset.y = args.fValue;
		}
	});
	AddWidget(&offsetY);

	offsetZ.Create(-10, 10, 0, 10000, "Offset Z: ");
	offsetZ.SetSize(XMFLOAT2(wid, hei));
	offsetZ.SetPos(XMFLOAT2(x, y += step));
	offsetZ.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->offset.z = args.fValue;
		}
	});
	AddWidget(&offsetZ);


	y += step;


	tailX.Create(-10, 10, 0, 10000, "Tail X: ");
	tailX.SetSize(XMFLOAT2(wid, hei));
	tailX.SetPos(XMFLOAT2(x, y += step));
	tailX.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->tail.x = args.fValue;
		}
	});
	AddWidget(&tailX);

	tailY.Create(-10, 10, 0, 10000, "Tail Y: ");
	tailY.SetSize(XMFLOAT2(wid, hei));
	tailY.SetPos(XMFLOAT2(x, y += step));
	tailY.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->tail.y = args.fValue;
		}
	});
	AddWidget(&tailY);

	tailZ.Create(-10, 10, 0, 10000, "Tail Z: ");
	tailZ.SetSize(XMFLOAT2(wid, hei));
	tailZ.SetPos(XMFLOAT2(x, y += step));
	tailZ.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ColliderComponent* collider = scene.colliders.GetComponent(x.entity);
			if (collider == nullptr)
				continue;
			collider->tail.z = args.fValue;
		}
	});
	AddWidget(&tailZ);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ColliderWindow::SetEntity(Entity entity)
{
	Scene& scene = editor->GetCurrentScene();

	const ColliderComponent* collider = scene.colliders.GetComponent(entity);

	if (collider != nullptr)
	{
		if (this->entity == entity)
			return;
		this->entity = entity;
		cpuCheckBox.SetCheck(collider->IsCPUEnabled());
		gpuCheckBox.SetCheck(collider->IsGPUEnabled());
		shapeCombo.SetSelectedByUserdataWithoutCallback((uint64_t)collider->shape);
		radiusSlider.SetValue(collider->radius);
		offsetX.SetValue(collider->offset.x);
		offsetY.SetValue(collider->offset.y);
		offsetZ.SetValue(collider->offset.z);
		tailX.SetValue(collider->tail.x);
		tailY.SetValue(collider->tail.y);
		tailZ.SetValue(collider->tail.z);
	}
	else
	{
		this->entity = INVALID_ENTITY;
	}
}

void ColliderWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 80;
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
	add_right(cpuCheckBox);
	gpuCheckBox.SetPos(XMFLOAT2(cpuCheckBox.GetPos().x - 100, cpuCheckBox.GetPos().y));
	add(shapeCombo);
	add(radiusSlider);

	y += jump;

	add(offsetX);
	add(offsetY);
	add(offsetZ);

	y += jump;

	add(tailX);
	add(tailY);
	add(tailZ);

}
