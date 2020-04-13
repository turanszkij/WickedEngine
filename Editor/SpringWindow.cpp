#include "stdafx.h"
#include "SpringWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


SpringWindow::SpringWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Spring Window");
	window->SetSize(XMFLOAT2(460, 230));
	GUI->AddWidget(window);

	float x = 150;
	float y = 0;
	float step = 25;
	float siz = 200;
	float hei = 20;

	createButton = new wiButton("Create");
	createButton->SetTooltip("Create/Remove Spring Component to selected entity");
	createButton->SetPos(XMFLOAT2(x, y += step));
	createButton->SetSize(XMFLOAT2(siz, hei));
	window->AddWidget(createButton);

	debugCheckBox = new wiCheckBox("DEBUG: ");
	debugCheckBox->SetTooltip("Enabling this will visualize springs as small yellow X-es in the scene");
	debugCheckBox->SetPos(XMFLOAT2(x, y += step));
	debugCheckBox->SetSize(XMFLOAT2(hei, hei));
	window->AddWidget(debugCheckBox);

	disabledCheckBox = new wiCheckBox("Disabled: ");
	disabledCheckBox->SetTooltip("Disable simulation.");
	disabledCheckBox->SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox->SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox->OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetDisabled(args.bValue);
		});
	window->AddWidget(disabledCheckBox);

	stretchCheckBox = new wiCheckBox("Stretch enabled: ");
	stretchCheckBox->SetTooltip("Stretch means that length from parent transform won't be preserved.");
	stretchCheckBox->SetPos(XMFLOAT2(x, y += step));
	stretchCheckBox->SetSize(XMFLOAT2(hei, hei));
	stretchCheckBox->OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetStretchEnabled(args.bValue);
		});
	window->AddWidget(stretchCheckBox);

	gravityCheckBox = new wiCheckBox("Gravity enabled: ");
	gravityCheckBox->SetTooltip("Whether global gravity should affect the spring");
	gravityCheckBox->SetPos(XMFLOAT2(x, y += step));
	gravityCheckBox->SetSize(XMFLOAT2(hei, hei));
	gravityCheckBox->OnClick([=](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->SetGravityEnabled(args.bValue);
		});
	window->AddWidget(gravityCheckBox);

	stiffnessSlider = new wiSlider(0, 1000, 100, 100000, "Stiffness: ");
	stiffnessSlider->SetTooltip("The stiffness affects how strongly the spring tries to orient itself to rest pose (higher values increase the jiggliness)");
	stiffnessSlider->SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider->SetSize(XMFLOAT2(siz, hei));
	stiffnessSlider->OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->stiffness = args.fValue;
		});
	window->AddWidget(stiffnessSlider);

	dampingSlider = new wiSlider(0, 1, 0.8f, 100000, "Damping: ");
	dampingSlider->SetTooltip("The damping affects how fast energy is lost (higher values make the spring come to rest faster)");
	dampingSlider->SetPos(XMFLOAT2(x, y += step));
	dampingSlider->SetSize(XMFLOAT2(siz, hei));
	dampingSlider->OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->damping = args.fValue;
		});
	window->AddWidget(dampingSlider);

	windSlider = new wiSlider(0, 1, 0, 100000, "Wind affection: ");
	windSlider->SetTooltip("How much the global wind effect affects the spring");
	windSlider->SetPos(XMFLOAT2(x, y += step));
	windSlider->SetSize(XMFLOAT2(siz, hei));
	windSlider->OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().springs.GetComponent(entity)->wind_affection = args.fValue;
		});
	window->AddWidget(windSlider);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 700, 80, 0));
	window->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


SpringWindow::~SpringWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void SpringWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const SpringComponent* spring = wiScene::GetScene().springs.GetComponent(entity);

	if (spring != nullptr)
	{
		window->SetEnabled(true);

		disabledCheckBox->SetCheck(spring->IsDisabled());
		stretchCheckBox->SetCheck(spring->IsStretchEnabled());
		gravityCheckBox->SetCheck(spring->IsGravityEnabled());
		stiffnessSlider->SetValue(spring->stiffness);
		dampingSlider->SetValue(spring->damping);
		windSlider->SetValue(spring->wind_affection);
	}
	else
	{
		window->SetEnabled(false);
	}

	const TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
	if (transform != nullptr)
	{
		createButton->SetEnabled(true);

		if (spring == nullptr)
		{
			createButton->SetText("Create");
			createButton->OnClick([=](wiEventArgs args) {
				wiScene::GetScene().springs.Create(entity);
				SetEntity(entity);
				});
		}
		else
		{
			createButton->SetText("Remove");
			createButton->OnClick([=](wiEventArgs args) {
				wiScene::GetScene().springs.Remove_KeepSorted(entity);
				SetEntity(entity);
				});
		}
	}

	debugCheckBox->SetEnabled(true);
}
