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
		Scene& scene = wiRenderer::GetScene();
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
				Scene& scene = wiRenderer::GetScene();
				hair->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
		}
	});
	meshComboBox->SetTooltip("Choose an animation clip...");
	hairWindow->AddWidget(meshComboBox);

	countSlider = new wiSlider(0, 100000, 1000, 100000, "Particle Count: ");
	countSlider->SetSize(XMFLOAT2(360, 30));
	countSlider->SetPos(XMFLOAT2(x, y += step * 2));
	countSlider->OnSlide([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->particleCount = (size_t)args.iValue;
		}
	});
	countSlider->SetEnabled(false);
	countSlider->SetTooltip("Set hair strand count");
	hairWindow->AddWidget(countSlider);

	generateButton = new wiButton("Generate Hair");
	generateButton->SetPos(XMFLOAT2(x, y += step));
	generateButton->SetSize(XMFLOAT2(150, 30));
	generateButton->OnClick([&](wiEventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			const MeshComponent* mesh = wiRenderer::GetScene().meshes.GetComponent(hair->meshID);
			if (mesh != nullptr)
			{
				hair->Generate(*mesh);
			}
			else
			{
				wiHelper::messageBox("Please choose a mesh first!");
			}
		}
	});
	generateButton->SetTooltip("Generate hair particles on a mesh surface");
	hairWindow->AddWidget(generateButton);


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
		countSlider->SetValue((float)hair->particleCount);
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

	Scene& scene = wiRenderer::GetScene();
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

	Scene& scene = wiRenderer::GetScene();

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
