#include "stdafx.h"
#include "SpringWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void SpringWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SPRING " Spring", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(460, 220));

	closeButton.SetTooltip("Delete SpringComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().springs.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 120;
	float y = 0;
	float siz = 140;
	float hei = 18;
	float step = hei + 2;

	resetAllButton.Create("Reset All");
	resetAllButton.SetTooltip("Reset all springs in the scene to initial pose.");
	resetAllButton.SetPos(XMFLOAT2(x, y));
	resetAllButton.SetSize(XMFLOAT2(siz, hei));
	resetAllButton.OnClick([&](wi::gui::EventArgs args) {
		auto& scene = editor->GetCurrentScene();
		for (size_t i = 0; i < scene.springs.GetCount(); ++i)
		{
			scene.springs[i].Reset();
		}
	});
	AddWidget(&resetAllButton);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->SetDisabled(args.bValue);
		}
	});
	AddWidget(&disabledCheckBox);

	gravityCheckBox.Create("Gravity enabled: ");
	gravityCheckBox.SetTooltip("Whether global gravity should affect the spring");
	gravityCheckBox.SetPos(XMFLOAT2(x, y += step));
	gravityCheckBox.SetSize(XMFLOAT2(hei, hei));
	gravityCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->SetGravityEnabled(args.bValue);
		}
	});
	AddWidget(&gravityCheckBox);

	stiffnessSlider.Create(0, 1, 0.1f, 100000, "Stiffness: ");
	stiffnessSlider.SetTooltip("The stiffness affects how strongly the spring tries to orient itself to rest pose (higher values increase the jiggliness)");
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.SetSize(XMFLOAT2(siz, hei));
	stiffnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->stiffnessForce = args.fValue;
		}
	});
	AddWidget(&stiffnessSlider);

	dragSlider.Create(0, 1, 0.8f, 100000, "Drag: ");
	dragSlider.SetTooltip("The drag affects how fast energy is lost (higher values make the spring come to rest faster)");
	dragSlider.SetPos(XMFLOAT2(x, y += step));
	dragSlider.SetSize(XMFLOAT2(siz, hei));
	dragSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->dragForce = args.fValue;
		}
	});
	AddWidget(&dragSlider);

	windSlider.Create(0, 1, 0, 100000, "Wind affection: ");
	windSlider.SetTooltip("How much the global wind effect affects the spring");
	windSlider.SetPos(XMFLOAT2(x, y += step));
	windSlider.SetSize(XMFLOAT2(siz, hei));
	windSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->windForce = args.fValue;
		}
	});
	AddWidget(&windSlider);

	gravitySlider.Create(0, 1, 0, 100000, "Gravity affection: ");
	gravitySlider.SetTooltip("How much the global gravity effect affects the spring");
	gravitySlider.SetPos(XMFLOAT2(x, y += step));
	gravitySlider.SetSize(XMFLOAT2(siz, hei));
	gravitySlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->gravityPower = args.fValue;
		}
	});
	AddWidget(&gravitySlider);

	hitradiusSlider.Create(0, 1, 0, 100000, "Collision hit radius: ");
	hitradiusSlider.SetTooltip("The radius of the spring's collision sphere, that will be checked against colliders.");
	hitradiusSlider.SetPos(XMFLOAT2(x, y += step));
	hitradiusSlider.SetSize(XMFLOAT2(siz, hei));
	hitradiusSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SpringComponent* spring = scene.springs.GetComponent(x.entity);
			if (spring == nullptr)
				continue;
			spring->hitRadius = args.fValue;
		}
	});
	AddWidget(&hitradiusSlider);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void SpringWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const SpringComponent* spring = editor->GetCurrentScene().springs.GetComponent(entity);

	if (spring != nullptr)
	{
		SetEnabled(true);

		disabledCheckBox.SetCheck(spring->IsDisabled());
		gravityCheckBox.SetCheck(spring->IsGravityEnabled());
		stiffnessSlider.SetValue(spring->stiffnessForce);
		dragSlider.SetValue(spring->dragForce);
		windSlider.SetValue(spring->windForce);
		gravitySlider.SetValue(spring->gravityPower);
		hitradiusSlider.SetValue(spring->hitRadius);
	}
	else
	{
		SetEnabled(false);
	}
}

void SpringWindow::ResizeLayout()
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

	add_fullwidth(resetAllButton);
	add_right(disabledCheckBox);
	add_right(gravityCheckBox);
	add(stiffnessSlider);
	add(dragSlider);
	add(windSlider);
	add(gravitySlider);
	add(hitradiusSlider);

}
