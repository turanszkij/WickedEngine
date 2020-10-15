#include "stdafx.h"
#include "HairParticleWindow.h"
#include "Editor.h"

using namespace std;
using namespace wiECS;
using namespace wiScene;

void HairParticleWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Hair Particle System Window");
	SetSize(XMFLOAT2(600, 260));

	float x = 160;
	float y = 10;
	float hei = 18;
	float step = hei + 2;


	addButton.Create("Add Hair Particle System");
	addButton.SetPos(XMFLOAT2(x, y += step));
	addButton.SetSize(XMFLOAT2(200, hei));
	addButton.OnClick([=](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		Entity entity = scene.Entity_CreateHair("editorHair");
		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
	});
	addButton.SetTooltip("Add new hair particle system.");
	AddWidget(&addButton);

	meshComboBox.Create("Mesh: ");
	meshComboBox.SetSize(XMFLOAT2(300, hei));
	meshComboBox.SetPos(XMFLOAT2(x, y += step));
	meshComboBox.SetEnabled(false);
	meshComboBox.OnSelect([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			if (args.iValue == 0)
			{
				hair->meshID = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiScene::GetScene();
				hair->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
		}
	});
	meshComboBox.SetTooltip("Choose a mesh where hair will grow from...");
	AddWidget(&meshComboBox);

	countSlider.Create(0, 100000, 1000, 100000, "Strand Count: ");
	countSlider.SetSize(XMFLOAT2(360, hei));
	countSlider.SetPos(XMFLOAT2(x, y += step));
	countSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->strandCount = (uint32_t)args.iValue;
		}
	});
	countSlider.SetEnabled(false);
	countSlider.SetTooltip("Set hair strand count");
	AddWidget(&countSlider);

	lengthSlider.Create(0, 4, 1, 100000, "Particle Length: ");
	lengthSlider.SetSize(XMFLOAT2(360, hei));
	lengthSlider.SetPos(XMFLOAT2(x, y += step));
	lengthSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->length = args.fValue;
		}
	});
	lengthSlider.SetEnabled(false);
	lengthSlider.SetTooltip("Set hair strand length");
	AddWidget(&lengthSlider);

	stiffnessSlider.Create(0, 20, 5, 100000, "Particle Stiffness: ");
	stiffnessSlider.SetSize(XMFLOAT2(360, hei));
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->stiffness = args.fValue;
		}
	});
	stiffnessSlider.SetEnabled(false);
	stiffnessSlider.SetTooltip("Set hair strand stiffness, how much it tries to get back to rest position.");
	AddWidget(&stiffnessSlider);

	randomnessSlider.Create(0, 1, 0.2f, 100000, "Particle Randomness: ");
	randomnessSlider.SetSize(XMFLOAT2(360, hei));
	randomnessSlider.SetPos(XMFLOAT2(x, y += step));
	randomnessSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->randomness = args.fValue;
		}
	});
	randomnessSlider.SetEnabled(false);
	randomnessSlider.SetTooltip("Set hair length randomization factor. This will affect randomness of hair lengths.");
	AddWidget(&randomnessSlider);

	segmentcountSlider.Create(1, 10, 1, 9, "Segment Count: ");
	segmentcountSlider.SetSize(XMFLOAT2(360, hei));
	segmentcountSlider.SetPos(XMFLOAT2(x, y += step));
	segmentcountSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->segmentCount = (uint32_t)args.iValue;
		}
	});
	segmentcountSlider.SetEnabled(false);
	segmentcountSlider.SetTooltip("Set hair strand segment count. This will affect simulation quality and performance.");
	AddWidget(&segmentcountSlider);

	randomSeedSlider.Create(1, 12345, 1, 12344, "Random seed: ");
	randomSeedSlider.SetSize(XMFLOAT2(360, hei));
	randomSeedSlider.SetPos(XMFLOAT2(x, y += step));
	randomSeedSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->randomSeed = (uint32_t)args.iValue;
		}
	});
	randomSeedSlider.SetEnabled(false);
	randomSeedSlider.SetTooltip("Set hair system-wide random seed value. This will affect hair patch placement randomization.");
	AddWidget(&randomSeedSlider);

	viewDistanceSlider.Create(0, 1000, 100, 10000, "View distance: ");
	viewDistanceSlider.SetSize(XMFLOAT2(360, hei));
	viewDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	viewDistanceSlider.OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->viewDistance = args.fValue;
		}
		});
	viewDistanceSlider.SetEnabled(false);
	viewDistanceSlider.SetTooltip("Set view distance. After this, particles will be faded out.");
	AddWidget(&viewDistanceSlider);

	framesXInput.Create("");
	framesXInput.SetPos(XMFLOAT2(x, y += step));
	framesXInput.SetSize(XMFLOAT2(40, hei));
	framesXInput.SetText("");
	framesXInput.SetTooltip("How many horizontal frames there are in the spritesheet.");
	framesXInput.SetDescription("Frames X: ");
	framesXInput.OnInputAccepted([this](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->framesX = (uint32_t)args.iValue;
		}
	});
	AddWidget(&framesXInput);

	framesYInput.Create("");
	framesYInput.SetPos(XMFLOAT2(x + 250, y));
	framesYInput.SetSize(XMFLOAT2(40, hei));
	framesYInput.SetText("");
	framesYInput.SetTooltip("How many vertical frames there are in the spritesheet.");
	framesYInput.SetDescription("Frames Y: ");
	framesYInput.OnInputAccepted([this](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->framesY = (uint32_t)args.iValue;
		}
		});
	AddWidget(&framesYInput);

	step = 20;

	frameCountInput.Create("");
	frameCountInput.SetPos(XMFLOAT2(x, y += step));
	frameCountInput.SetSize(XMFLOAT2(40, hei));
	frameCountInput.SetText("");
	frameCountInput.SetTooltip("Enter a value to enable the random sprite sheet frame selection's max frame number.");
	frameCountInput.SetDescription("Frame Count: ");
	frameCountInput.OnInputAccepted([this](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->frameCount = (uint32_t)args.iValue;
		}
		});
	AddWidget(&frameCountInput);

	frameStartInput.Create("");
	frameStartInput.SetPos(XMFLOAT2(x + 250, y));
	frameStartInput.SetSize(XMFLOAT2(40, hei));
	frameStartInput.SetText("");
	frameStartInput.SetTooltip("Specifies the first frame of the sheet that can be used.");
	frameStartInput.SetDescription("First Frame: ");
	frameStartInput.OnInputAccepted([this](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->frameStart = (uint32_t)args.iValue;
		}
		});
	AddWidget(&frameStartInput);




	Translate(XMFLOAT3(200, 50, 0));
	SetVisible(false);

	SetEntity(entity);
}

void HairParticleWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	auto hair = GetHair();

	if (hair != nullptr)
	{
		lengthSlider.SetValue(hair->length);
		stiffnessSlider.SetValue(hair->stiffness);
		randomnessSlider.SetValue(hair->randomness);
		countSlider.SetValue((float)hair->strandCount);
		segmentcountSlider.SetValue((float)hair->segmentCount);
		randomSeedSlider.SetValue((float)hair->randomSeed);
		viewDistanceSlider.SetValue(hair->viewDistance);
		framesXInput.SetValue((int)hair->framesX);
		framesYInput.SetValue((int)hair->framesY);
		frameCountInput.SetValue((int)hair->frameCount);
		frameStartInput.SetValue((int)hair->frameStart);

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}

	addButton.SetEnabled(true);
}

wiHairParticle* HairParticleWindow::GetHair()
{
	if (entity == INVALID_ENTITY)
	{
		return nullptr;
	}

	Scene& scene = wiScene::GetScene();
	wiHairParticle* hair = scene.hairs.GetComponent(entity);

	return hair;
}

void HairParticleWindow::UpdateData()
{
	auto emitter = GetHair();
	if (emitter == nullptr)
	{
		return;
	}

	Scene& scene = wiScene::GetScene();

	meshComboBox.ClearItems();
	meshComboBox.AddItem("NO MESH");
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		Entity entity = scene.meshes.GetEntity(i);
		const NameComponent& name = *scene.names.GetComponent(entity);
		meshComboBox.AddItem(name.name);

		if (emitter->meshID == entity)
		{
			meshComboBox.SetSelected((int)i + 1);
		}
	}
}
