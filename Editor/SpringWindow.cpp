#include "stdafx.h"
#include "SpringWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


void SpringWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Spring Window");
	SetSize(XMFLOAT2(460, 200));

	float x = 150;
	float y = 10;
	float siz = 200;
	float hei = 18;
	float step = hei + 2;

	createButton.Create("Create");
	createButton.SetTooltip("Create/Remove Spring Component to selected entity");
	createButton.SetPos(XMFLOAT2(x, y += step));
	createButton.SetSize(XMFLOAT2(siz, hei));
	AddWidget(&createButton);

	debugCheckBox.Create("DEBUG: ");
	debugCheckBox.SetTooltip("Enabling this will visualize springs as small yellow X-es in the scene");
	debugCheckBox.SetPos(XMFLOAT2(x, y += step));
	debugCheckBox.SetSize(XMFLOAT2(hei, hei));
	AddWidget(&debugCheckBox);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetDisabled(args.bValue);
		});
	AddWidget(&disabledCheckBox);

	stretchCheckBox.Create("Stretch enabled: ");
	stretchCheckBox.SetTooltip("Stretch means that length from parent transform won't be preserved.");
	stretchCheckBox.SetPos(XMFLOAT2(x, y += step));
	stretchCheckBox.SetSize(XMFLOAT2(hei, hei));
	stretchCheckBox.OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetStretchEnabled(args.bValue);
		});
	AddWidget(&stretchCheckBox);

	gravityCheckBox.Create("Gravity enabled: ");
	gravityCheckBox.SetTooltip("Whether global gravity should affect the spring");
	gravityCheckBox.SetPos(XMFLOAT2(x, y += step));
	gravityCheckBox.SetSize(XMFLOAT2(hei, hei));
	gravityCheckBox.OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetGravityEnabled(args.bValue);
		});
	AddWidget(&gravityCheckBox);

	stiffnessSlider.Create(0, 1000, 100, 100000, "Stiffness: ");
	stiffnessSlider.SetTooltip("The stiffness affects how strongly the spring tries to orient itself to rest pose (higher values increase the jiggliness)");
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.SetSize(XMFLOAT2(siz, hei));
	stiffnessSlider.OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->stiffness = args.fValue;
		});
	AddWidget(&stiffnessSlider);

	dampingSlider.Create(0, 1, 0.8f, 100000, "Damping: ");
	dampingSlider.SetTooltip("The damping affects how fast energy is lost (higher values make the spring come to rest faster)");
	dampingSlider.SetPos(XMFLOAT2(x, y += step));
	dampingSlider.SetSize(XMFLOAT2(siz, hei));
	dampingSlider.OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->damping = args.fValue;
		});
	AddWidget(&dampingSlider);

	windSlider.Create(0, 1, 0, 100000, "Wind affection: ");
	windSlider.SetTooltip("How much the global wind effect affects the spring");
	windSlider.SetPos(XMFLOAT2(x, y += step));
	windSlider.SetSize(XMFLOAT2(siz, hei));
	windSlider.OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->wind_affection = args.fValue;
		});
	AddWidget(&windSlider);

	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 700, 80, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void SpringWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const SpringComponent* spring = wiScene::GetScene().springs.GetComponent(entity);

	if (spring != nullptr)
	{
		SetEnabled(true);

		disabledCheckBox.SetCheck(spring->IsDisabled());
		stretchCheckBox.SetCheck(spring->IsStretchEnabled());
		gravityCheckBox.SetCheck(spring->IsGravityEnabled());
		stiffnessSlider.SetValue(spring->stiffness);
		dampingSlider.SetValue(spring->damping);
		windSlider.SetValue(spring->wind_affection);
	}
	else
	{
		SetEnabled(false);
	}

	const TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
	if (transform != nullptr)
	{
		createButton.SetEnabled(true);

		if (spring == nullptr)
		{
			createButton.SetText("Create");
			createButton.OnClick([=](wiEventArgs args) {
				wiScene::GetScene().springs.Create(entity);
				SetEntity(entity);
				});
		}
		else
		{
			createButton.SetText("Remove");
			createButton.OnClick([=](wiEventArgs args) {
				wiScene::GetScene().springs.Remove_KeepSorted(entity);
				SetEntity(entity);
				});
		}
	}

	debugCheckBox.SetEnabled(true);
}
