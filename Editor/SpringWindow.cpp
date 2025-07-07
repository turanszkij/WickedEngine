#include "stdafx.h"
#include "SpringWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void SpringWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SPRING " Spring", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
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

	auto forEachSelected = [&] (auto func) {
		return [&, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				SpringComponent* spring = scene.springs.GetComponent(x.entity);
				if (spring != nullptr)
				{
					func(spring, args);
				}
			}
		};
	};

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick(forEachSelected([&] (auto spring, auto args) {
		spring->SetDisabled(args.bValue);
	}));
	AddWidget(&disabledCheckBox);

	gravityCheckBox.Create("Gravity enabled: ");
	gravityCheckBox.SetTooltip("Whether global gravity should affect the spring");
	gravityCheckBox.SetPos(XMFLOAT2(x, y += step));
	gravityCheckBox.SetSize(XMFLOAT2(hei, hei));
	gravityCheckBox.OnClick(forEachSelected([&] (auto spring, auto args) {
		spring->SetGravityEnabled(args.bValue);
	}));
	AddWidget(&gravityCheckBox);

	stiffnessSlider.Create(0, 1, 0.1f, 100000, "Stiffness: ");
	stiffnessSlider.SetTooltip("The stiffness affects how strongly the spring tries to orient itself to rest pose (higher values increase the jiggliness)");
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.SetSize(XMFLOAT2(siz, hei));
	stiffnessSlider.OnSlide(forEachSelected([&] (auto spring, auto args) {
		spring->stiffnessForce = args.fValue;
	}));
	AddWidget(&stiffnessSlider);

	dragSlider.Create(0, 1, 0.8f, 100000, "Drag: ");
	dragSlider.SetTooltip("The drag affects how fast energy is lost (higher values make the spring come to rest faster)");
	dragSlider.SetPos(XMFLOAT2(x, y += step));
	dragSlider.SetSize(XMFLOAT2(siz, hei));
	dragSlider.OnSlide(forEachSelected([&] (auto spring, auto args) {
		spring->dragForce = args.fValue;
	}));
	AddWidget(&dragSlider);

	windSlider.Create(0, 1, 0, 100000, "Wind affection: ");
	windSlider.SetTooltip("How much the global wind effect affects the spring");
	windSlider.SetPos(XMFLOAT2(x, y += step));
	windSlider.SetSize(XMFLOAT2(siz, hei));
	windSlider.OnSlide(forEachSelected([&] (auto spring, auto args) {
		spring->windForce = args.fValue;
	}));
	AddWidget(&windSlider);

	gravitySlider.Create(0, 1, 0, 100000, "Gravity affection: ");
	gravitySlider.SetTooltip("How much the global gravity effect affects the spring");
	gravitySlider.SetPos(XMFLOAT2(x, y += step));
	gravitySlider.SetSize(XMFLOAT2(siz, hei));
	gravitySlider.OnSlide(forEachSelected([&] (auto spring, auto args) {
		spring->gravityPower = args.fValue;
	}));
	AddWidget(&gravitySlider);

	hitradiusSlider.Create(0, 1, 0, 100000, "Collision hit radius: ");
	hitradiusSlider.SetTooltip("The radius of the spring's collision sphere, that will be checked against colliders.");
	hitradiusSlider.SetPos(XMFLOAT2(x, y += step));
	hitradiusSlider.SetSize(XMFLOAT2(siz, hei));
	hitradiusSlider.OnSlide(forEachSelected([&] (auto spring, auto args) {
		spring->hitRadius = args.fValue;
	}));
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
	layout.margin_left = 120;

	layout.add_fullwidth(resetAllButton);
	layout.add_right(disabledCheckBox);
	layout.add_right(gravityCheckBox);
	layout.add(stiffnessSlider);
	layout.add(dragSlider);
	layout.add(windSlider);
	layout.add(gravitySlider);
	layout.add(hitradiusSlider);

}
