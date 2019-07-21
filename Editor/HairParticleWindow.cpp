#include "stdafx.h"
#include "HairParticleWindow.h"

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;

HairParticleWindow::HairParticleWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	hairWindow = new wiWindow(GUI, "Hair Particle System Window");
	hairWindow->SetSize(XMFLOAT2(800, 600));
	hairWindow->SetEnabled(false);
	GUI->AddWidget(hairWindow);

	float x = 150;
	float y = 20;
	float step = 35;


	addButton = new wiButton("Add Hair Particle System");
	addButton->SetPos(XMFLOAT2(x, y += step));
	addButton->SetSize(XMFLOAT2(200, 30));
	addButton->OnClick([&](wiEventArgs args) {
		Scene& scene = wiSceneSystem::GetScene();
		scene.Entity_CreateHair("editorHair");
	});
	addButton->SetTooltip("Add new hair particle system.");
	hairWindow->AddWidget(addButton);

	meshComboBox = new wiComboBox("Mesh: ");
	meshComboBox->SetSize(XMFLOAT2(300, 25));
	meshComboBox->SetPos(XMFLOAT2(x, y += step));
	meshComboBox->SetEnabled(false);
	meshComboBox->OnSelect([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			if (args.iValue == 0)
			{
				hair->meshID = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiSceneSystem::GetScene();
				hair->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
		}
	});
	meshComboBox->SetTooltip("Choose a mesh where hair will grow from...");
	hairWindow->AddWidget(meshComboBox);

	countSlider = new wiSlider(0, 100000, 1000, 100000, "Strand Count: ");
	countSlider->SetSize(XMFLOAT2(360, 30));
	countSlider->SetPos(XMFLOAT2(x, y += step * 2));
	countSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->strandCount = (uint32_t)args.iValue;
		}
	});
	countSlider->SetEnabled(false);
	countSlider->SetTooltip("Set hair strand count");
	hairWindow->AddWidget(countSlider);

	lengthSlider = new wiSlider(0, 4, 1, 100000, "Particle Length: ");
	lengthSlider->SetSize(XMFLOAT2(360, 30));
	lengthSlider->SetPos(XMFLOAT2(x, y += step));
	lengthSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->length = args.fValue;
		}
	});
	lengthSlider->SetEnabled(false);
	lengthSlider->SetTooltip("Set hair strand length");
	hairWindow->AddWidget(lengthSlider);

	stiffnessSlider = new wiSlider(0, 20, 5, 100000, "Particle Stiffness: ");
	stiffnessSlider->SetSize(XMFLOAT2(360, 30));
	stiffnessSlider->SetPos(XMFLOAT2(x, y += step * 2));
	stiffnessSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->stiffness = args.fValue;
		}
	});
	stiffnessSlider->SetEnabled(false);
	stiffnessSlider->SetTooltip("Set hair strand stiffness, how much it tries to get back to rest position.");
	hairWindow->AddWidget(stiffnessSlider);

	randomnessSlider = new wiSlider(0, 1, 0.2f, 100000, "Particle Randomness: ");
	randomnessSlider->SetSize(XMFLOAT2(360, 30));
	randomnessSlider->SetPos(XMFLOAT2(x, y += step));
	randomnessSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->randomness = args.fValue;
		}
	});
	randomnessSlider->SetEnabled(false);
	randomnessSlider->SetTooltip("Set hair length randomization factor. This will affect randomness of hair lengths.");
	hairWindow->AddWidget(randomnessSlider);

	segmentcountSlider = new wiSlider(1, 10, 1, 9, "Segment Count: ");
	segmentcountSlider->SetSize(XMFLOAT2(360, 30));
	segmentcountSlider->SetPos(XMFLOAT2(x, y += step));
	segmentcountSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->segmentCount = (uint32_t)args.iValue;
		}
	});
	segmentcountSlider->SetEnabled(false);
	segmentcountSlider->SetTooltip("Set hair strand segment count. This will affect simulation quality and performance.");
	hairWindow->AddWidget(segmentcountSlider);

	randomSeedSlider = new wiSlider(1, 12345, 1, 12344, "Random seed: ");
	randomSeedSlider->SetSize(XMFLOAT2(360, 30));
	randomSeedSlider->SetPos(XMFLOAT2(x, y += step));
	randomSeedSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->randomSeed = (uint32_t)args.iValue;
		}
	});
	randomSeedSlider->SetEnabled(false);
	randomSeedSlider->SetTooltip("Set hair system-wide random seed value. This will affect hair patch placement randomization.");
	hairWindow->AddWidget(randomSeedSlider);


	hairWindow->Translate(XMFLOAT3(200, 50, 0));
	hairWindow->SetVisible(false);

	SetEntity(entity);
}

HairParticleWindow::~HairParticleWindow()
{
	hairWindow->RemoveWidgets(true);
	GUI->RemoveWidget(hairWindow);
	SAFE_DELETE(hairWindow);
}

void HairParticleWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	auto hair = GetHair();

	if (hair != nullptr)
	{
		lengthSlider->SetValue(hair->length);
		stiffnessSlider->SetValue(hair->stiffness);
		randomnessSlider->SetValue(hair->randomness);
		countSlider->SetValue((float)hair->strandCount);
		segmentcountSlider->SetValue((float)hair->segmentCount);
		randomSeedSlider->SetValue((float)hair->randomSeed);
	}
	else
	{
	}

}

wiHairParticle* HairParticleWindow::GetHair()
{
	if (entity == INVALID_ENTITY)
	{
		return nullptr;
	}

	Scene& scene = wiSceneSystem::GetScene();
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

	Scene& scene = wiSceneSystem::GetScene();

	meshComboBox->ClearItems();
	meshComboBox->AddItem("NO MESH");
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		Entity entity = scene.meshes.GetEntity(i);
		const NameComponent& name = *scene.names.GetComponent(entity);
		meshComboBox->AddItem(name.name);

		if (emitter->meshID == entity)
		{
			meshComboBox->SetSelected((int)i + 1);
		}
	}
}
