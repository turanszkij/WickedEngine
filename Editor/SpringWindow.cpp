#include "stdafx.h"
#include "SpringWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void SpringWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Spring", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(460, 200));

	closeButton.SetTooltip("Delete SpringComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().springs.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
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

	debugCheckBox.Create("DEBUG: ");
	debugCheckBox.SetTooltip("Enabling this will visualize springs as small yellow X-es in the scene");
	debugCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugCheckBox.SetSize(XMFLOAT2(hei, hei));
	AddWidget(&debugCheckBox);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->SetDisabled(args.bValue);
		});
	AddWidget(&disabledCheckBox);

	stretchCheckBox.Create("Stretch enabled: ");
	stretchCheckBox.SetTooltip("Stretch means that length from parent transform won't be preserved.");
	stretchCheckBox.SetPos(XMFLOAT2(x, y += step));
	stretchCheckBox.SetSize(XMFLOAT2(hei, hei));
	stretchCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->SetStretchEnabled(args.bValue);
		});
	AddWidget(&stretchCheckBox);

	gravityCheckBox.Create("Gravity enabled: ");
	gravityCheckBox.SetTooltip("Whether global gravity should affect the spring");
	gravityCheckBox.SetPos(XMFLOAT2(x, y += step));
	gravityCheckBox.SetSize(XMFLOAT2(hei, hei));
	gravityCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->SetGravityEnabled(args.bValue);
		});
	AddWidget(&gravityCheckBox);

	stiffnessSlider.Create(0, 1, 0.1f, 100000, "Stiffness: ");
	stiffnessSlider.SetTooltip("The stiffness affects how strongly the spring tries to orient itself to rest pose (higher values increase the jiggliness)");
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.SetSize(XMFLOAT2(siz, hei));
	stiffnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->stiffnessForce = args.fValue;
		});
	AddWidget(&stiffnessSlider);

	dragSlider.Create(0, 1, 0.8f, 100000, "Drag: ");
	dragSlider.SetTooltip("The drag affects how fast energy is lost (higher values make the spring come to rest faster)");
	dragSlider.SetPos(XMFLOAT2(x, y += step));
	dragSlider.SetSize(XMFLOAT2(siz, hei));
	dragSlider.OnSlide([&](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->dragForce = args.fValue;
		});
	AddWidget(&dragSlider);

	windSlider.Create(0, 1, 0, 100000, "Wind affection: ");
	windSlider.SetTooltip("How much the global wind effect affects the spring");
	windSlider.SetPos(XMFLOAT2(x, y += step));
	windSlider.SetSize(XMFLOAT2(siz, hei));
	windSlider.OnSlide([&](wi::gui::EventArgs args) {
		editor->GetCurrentScene().springs.GetComponent(entity)->windForce = args.fValue;
		});
	AddWidget(&windSlider);


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
		stretchCheckBox.SetCheck(spring->IsStretchEnabled());
		gravityCheckBox.SetCheck(spring->IsGravityEnabled());
		stiffnessSlider.SetValue(spring->stiffnessForce);
		dragSlider.SetValue(spring->dragForce);
		windSlider.SetValue(spring->windForce);
	}
	else
	{
		SetEnabled(false);
	}

	debugCheckBox.SetEnabled(true);
}
