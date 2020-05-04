#include "stdafx.h"
#include "EmitterWindow.h"
#include "Editor.h"

#include <sstream>

using namespace std;
using namespace wiECS;
using namespace wiScene;

EmitterWindow::EmitterWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	emitterWindow = new wiWindow(GUI, "Emitter Window");
	emitterWindow->SetSize(XMFLOAT2(700, 820));
	GUI->AddWidget(emitterWindow);

	float x = 200;
	float y = 5;
	float step = 25;
	float itemheight = 20;


	emitterNameField = new wiTextInputField("EmitterName");
	emitterNameField->SetPos(XMFLOAT2(x, y += step));
	emitterNameField->SetSize(XMFLOAT2(300, itemheight));
	emitterNameField->OnInputAccepted([&](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;
		}
	});
	emitterWindow->AddWidget(emitterNameField);

	addButton = new wiButton("Add Emitter");
	addButton->SetPos(XMFLOAT2(x, y += step));
	addButton->SetSize(XMFLOAT2(150, itemheight));
	addButton->OnClick([=](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		Entity entity = scene.Entity_CreateEmitter("editorEmitter");
		editor->ClearSelected();
		editor->AddSelected(entity);
		SetEntity(entity);
	});
	addButton->SetTooltip("Add new emitter particle system.");
	emitterWindow->AddWidget(addButton);

	restartButton = new wiButton("Restart Emitter");
	restartButton->SetPos(XMFLOAT2(x + 160, y));
	restartButton->SetSize(XMFLOAT2(150, itemheight));
	restartButton->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->Restart();
		}
	});
	restartButton->SetTooltip("Restart particle system emitter");
	emitterWindow->AddWidget(restartButton);

	meshComboBox = new wiComboBox("Mesh: ");
	meshComboBox->SetSize(XMFLOAT2(300, itemheight));
	meshComboBox->SetPos(XMFLOAT2(x, y += step));
	meshComboBox->SetEnabled(false);
	meshComboBox->OnSelect([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			if (args.iValue == 0)
			{
				emitter->meshID = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiScene::GetScene();
				emitter->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
		}
	});
	meshComboBox->SetTooltip("Choose an mesh that particles will be emitted from...");
	emitterWindow->AddWidget(meshComboBox);

	shaderTypeComboBox = new wiComboBox("ShaderType: ");
	shaderTypeComboBox->SetPos(XMFLOAT2(x, y += step));
	shaderTypeComboBox->SetSize(XMFLOAT2(300, itemheight));
	shaderTypeComboBox->AddItem("SOFT");
	shaderTypeComboBox->AddItem("SOFT + DISTORTION");
	shaderTypeComboBox->AddItem("SIMPLEST");
	shaderTypeComboBox->AddItem("SOFT + LIGHTING");
	shaderTypeComboBox->OnSelect([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->shaderType = (wiEmittedParticle::PARTICLESHADERTYPE)args.iValue;
		}
	});
	shaderTypeComboBox->SetEnabled(false);
	shaderTypeComboBox->SetTooltip("Choose a shader type for the particles. This is responsible of how they will be rendered.");
	emitterWindow->AddWidget(shaderTypeComboBox);


	sortCheckBox = new wiCheckBox("Sorting Enabled: ");
	sortCheckBox->SetPos(XMFLOAT2(x, y += step));
	sortCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	sortCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetSorted(args.bValue);
		}
	});
	sortCheckBox->SetCheck(false);
	sortCheckBox->SetTooltip("Enable sorting of the particles. This might slow down performance.");
	emitterWindow->AddWidget(sortCheckBox);


	depthCollisionsCheckBox = new wiCheckBox("Depth Buffer Collisions Enabled: ");
	depthCollisionsCheckBox->SetPos(XMFLOAT2(x + 250, y));
	depthCollisionsCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	depthCollisionsCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetDepthCollisionEnabled(args.bValue);
		}
	});
	depthCollisionsCheckBox->SetCheck(false);
	depthCollisionsCheckBox->SetTooltip("Enable particle collisions with the depth buffer.");
	emitterWindow->AddWidget(depthCollisionsCheckBox);


	sphCheckBox = new wiCheckBox("SPH - FluidSim: ");
	sphCheckBox->SetPos(XMFLOAT2(x + 400, y));
	sphCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	sphCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetSPHEnabled(args.bValue);
		}
	});
	sphCheckBox->SetCheck(false);
	sphCheckBox->SetTooltip("Enable particle collisions with each other. Simulate with Smooth Particle Hydrodynamics (SPH) solver.");
	emitterWindow->AddWidget(sphCheckBox);


	pauseCheckBox = new wiCheckBox("PAUSE: ");
	pauseCheckBox->SetPos(XMFLOAT2(x, y += step));
	pauseCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	pauseCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetPaused(args.bValue);
		}
	});
	pauseCheckBox->SetCheck(false);
	pauseCheckBox->SetTooltip("Stop simulation update.");
	emitterWindow->AddWidget(pauseCheckBox);


	debugCheckBox = new wiCheckBox("DEBUG: ");
	debugCheckBox->SetPos(XMFLOAT2(x + 120, y));
	debugCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	debugCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetDebug(args.bValue);
		}
	});
	debugCheckBox->SetCheck(false);
	debugCheckBox->SetTooltip("Enable debug info for the emitter. This involves reading back GPU data, so rendering can slow down.");
	emitterWindow->AddWidget(debugCheckBox);


	volumeCheckBox = new wiCheckBox("Volume: ");
	volumeCheckBox->SetPos(XMFLOAT2(x + 250, y));
	volumeCheckBox->SetSize(XMFLOAT2(itemheight, itemheight));
	volumeCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetVolumeEnabled(args.bValue);
		}
		});
	volumeCheckBox->SetCheck(false);
	volumeCheckBox->SetTooltip("Enable volume for the emitter. Particles will be emitted inside volume.");
	emitterWindow->AddWidget(volumeCheckBox);



	infoLabel = new wiLabel("EmitterInfo");
	infoLabel->SetSize(XMFLOAT2(380, 120));
	infoLabel->SetPos(XMFLOAT2(x, y += step));
	emitterWindow->AddWidget(infoLabel);


	maxParticlesSlider = new wiSlider(100.0f, 1000000.0f, 10000, 100000, "Max particle count: ");
	maxParticlesSlider->SetSize(XMFLOAT2(360, itemheight));
	maxParticlesSlider->SetPos(XMFLOAT2(x, y += step + 120));
	maxParticlesSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetMaxParticleCount((uint32_t)args.iValue);
		}
	});
	maxParticlesSlider->SetEnabled(false);
	maxParticlesSlider->SetTooltip("Set the maximum amount of particles this system can handle. This has an effect on the memory budget.");
	emitterWindow->AddWidget(maxParticlesSlider);

	y += 30;

	//////////////////////////////////////////////////////////////////////////////////////////////////

	emitCountSlider = new wiSlider(0.0f, 10000.0f, 1.0f, 100000, "Emit count per sec: ");
	emitCountSlider->SetSize(XMFLOAT2(360, itemheight));
	emitCountSlider->SetPos(XMFLOAT2(x, y += step));
	emitCountSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->count = args.fValue;
		}
	});
	emitCountSlider->SetEnabled(false);
	emitCountSlider->SetTooltip("Set the number of particles to emit per second.");
	emitterWindow->AddWidget(emitCountSlider);

	emitSizeSlider = new wiSlider(0.01f, 10.0f, 1.0f, 100000, "Size: ");
	emitSizeSlider->SetSize(XMFLOAT2(360, itemheight));
	emitSizeSlider->SetPos(XMFLOAT2(x, y += step));
	emitSizeSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->size = args.fValue;
		}
	});
	emitSizeSlider->SetEnabled(false);
	emitSizeSlider->SetTooltip("Set the size of the emitted particles.");
	emitterWindow->AddWidget(emitSizeSlider);

	emitRotationSlider = new wiSlider(0.0f, 1.0f, 0.0f, 100000, "Rotation: ");
	emitRotationSlider->SetSize(XMFLOAT2(360, itemheight));
	emitRotationSlider->SetPos(XMFLOAT2(x, y += step));
	emitRotationSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->rotation = args.fValue;
		}
	});
	emitRotationSlider->SetEnabled(false);
	emitRotationSlider->SetTooltip("Set the rotation velocity of the emitted particles.");
	emitterWindow->AddWidget(emitRotationSlider);

	emitNormalSlider = new wiSlider(0.0f, 100.0f, 1.0f, 100000, "Normal factor: ");
	emitNormalSlider->SetSize(XMFLOAT2(360, itemheight));
	emitNormalSlider->SetPos(XMFLOAT2(x, y += step));
	emitNormalSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->normal_factor = args.fValue;
		}
	});
	emitNormalSlider->SetEnabled(false);
	emitNormalSlider->SetTooltip("Set the velocity of the emitted particles based on the normal vector of the emitter surface.");
	emitterWindow->AddWidget(emitNormalSlider);

	emitScalingSlider = new wiSlider(0.0f, 100.0f, 1.0f, 100000, "Scaling: ");
	emitScalingSlider->SetSize(XMFLOAT2(360, itemheight));
	emitScalingSlider->SetPos(XMFLOAT2(x, y += step));
	emitScalingSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->scaleX = args.fValue;
		}
	});
	emitScalingSlider->SetEnabled(false);
	emitScalingSlider->SetTooltip("Set the scaling of the particles based on their lifetime.");
	emitterWindow->AddWidget(emitScalingSlider);

	emitLifeSlider = new wiSlider(0.0f, 100.0f, 1.0f, 10000, "Life span: ");
	emitLifeSlider->SetSize(XMFLOAT2(360, itemheight));
	emitLifeSlider->SetPos(XMFLOAT2(x, y += step));
	emitLifeSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->life = args.fValue;
		}
	});
	emitLifeSlider->SetEnabled(false);
	emitLifeSlider->SetTooltip("Set the lifespan of the emitted particles (in seconds).");
	emitterWindow->AddWidget(emitLifeSlider);

	emitRandomnessSlider = new wiSlider(0.0f, 1.0f, 1.0f, 100000, "Randomness: ");
	emitRandomnessSlider->SetSize(XMFLOAT2(360, itemheight));
	emitRandomnessSlider->SetPos(XMFLOAT2(x, y += step));
	emitRandomnessSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->random_factor = args.fValue;
		}
	});
	emitRandomnessSlider->SetEnabled(false);
	emitRandomnessSlider->SetTooltip("Set the general randomness of the emitter.");
	emitterWindow->AddWidget(emitRandomnessSlider);

	emitLifeRandomnessSlider = new wiSlider(0.0f, 2.0f, 0.0f, 100000, "Life randomness: ");
	emitLifeRandomnessSlider->SetSize(XMFLOAT2(360, itemheight));
	emitLifeRandomnessSlider->SetPos(XMFLOAT2(x, y += step));
	emitLifeRandomnessSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->random_life = args.fValue;
		}
	});
	emitLifeRandomnessSlider->SetEnabled(false);
	emitLifeRandomnessSlider->SetTooltip("Set the randomness of lifespans for the emitted particles.");
	emitterWindow->AddWidget(emitLifeRandomnessSlider);

	emitMotionBlurSlider = new wiSlider(0.0f, 1.0f, 1.0f, 100000, "Motion blur: ");
	emitMotionBlurSlider->SetSize(XMFLOAT2(360, itemheight));
	emitMotionBlurSlider->SetPos(XMFLOAT2(x, y += step));
	emitMotionBlurSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->motionBlurAmount = args.fValue;
		}
	});
	emitMotionBlurSlider->SetEnabled(false);
	emitMotionBlurSlider->SetTooltip("Set the motion blur amount for the particle system.");
	emitterWindow->AddWidget(emitMotionBlurSlider);

	emitMassSlider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "Mass: ");
	emitMassSlider->SetSize(XMFLOAT2(360, itemheight));
	emitMassSlider->SetPos(XMFLOAT2(x, y += step));
	emitMassSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->mass = args.fValue;
		}
	});
	emitMassSlider->SetEnabled(false);
	emitMassSlider->SetTooltip("Set the mass per particle.");
	emitterWindow->AddWidget(emitMassSlider);



	timestepSlider = new wiSlider(-1, 0.016f, -1, 100000, "Timestep: ");
	timestepSlider->SetSize(XMFLOAT2(360, itemheight));
	timestepSlider->SetPos(XMFLOAT2(x, y += step*2));
	timestepSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->FIXED_TIMESTEP = args.fValue;
		}
	});
	timestepSlider->SetEnabled(false);
	timestepSlider->SetTooltip("Adjust timestep for emitter simulation. -1 means variable timestep, positive means fixed timestep.");
	emitterWindow->AddWidget(timestepSlider);





	//////////////// SPH ////////////////////////////

	y += step;

	sph_h_Slider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "SPH Smoothing Radius (h): ");
	sph_h_Slider->SetSize(XMFLOAT2(360, itemheight));
	sph_h_Slider->SetPos(XMFLOAT2(x, y += step));
	sph_h_Slider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SPH_h = args.fValue;
		}
	});
	sph_h_Slider->SetEnabled(false);
	sph_h_Slider->SetTooltip("Set the SPH parameter: smoothing radius");
	emitterWindow->AddWidget(sph_h_Slider);

	sph_K_Slider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "SPH Pressure Constant (K): ");
	sph_K_Slider->SetSize(XMFLOAT2(360, itemheight));
	sph_K_Slider->SetPos(XMFLOAT2(x, y += step));
	sph_K_Slider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SPH_K = args.fValue;
		}
	});
	sph_K_Slider->SetEnabled(false);
	sph_K_Slider->SetTooltip("Set the SPH parameter: pressure constant");
	emitterWindow->AddWidget(sph_K_Slider);

	sph_p0_Slider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "SPH Reference Density (p0): ");
	sph_p0_Slider->SetSize(XMFLOAT2(360, itemheight));
	sph_p0_Slider->SetPos(XMFLOAT2(x, y += step));
	sph_p0_Slider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SPH_p0 = args.fValue;
		}
	});
	sph_p0_Slider->SetEnabled(false);
	sph_p0_Slider->SetTooltip("Set the SPH parameter: reference density");
	emitterWindow->AddWidget(sph_p0_Slider);

	sph_e_Slider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "SPH Viscosity (e): ");
	sph_e_Slider->SetSize(XMFLOAT2(360, itemheight));
	sph_e_Slider->SetPos(XMFLOAT2(x, y += step));
	sph_e_Slider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SPH_e = args.fValue;
		}
	});
	sph_e_Slider->SetEnabled(false);
	sph_e_Slider->SetTooltip("Set the SPH parameter: viscosity constant");
	emitterWindow->AddWidget(sph_e_Slider);





	emitterWindow->Translate(XMFLOAT3(200, 50, 0));
	emitterWindow->SetVisible(false);

	SetEntity(entity);
}

EmitterWindow::~EmitterWindow()
{
	emitterWindow->RemoveWidgets(true);
	GUI->RemoveWidget(emitterWindow);
	delete emitterWindow;
}

void EmitterWindow::SetEntity(Entity entity)
{
	// first try to turn off any debug readbacks for emitters:
	if (GetEmitter() != nullptr)
	{
		GetEmitter()->SetDebug(false);
	}
	debugCheckBox->SetCheck(false);

	this->entity = entity;

	auto emitter = GetEmitter();

	if (emitter != nullptr)
	{
		emitterNameField->SetEnabled(true);
		restartButton->SetEnabled(true);
		shaderTypeComboBox->SetEnabled(true);
		meshComboBox->SetEnabled(true);
		debugCheckBox->SetEnabled(true);
		volumeCheckBox->SetEnabled(true);
		sortCheckBox->SetEnabled(true);
		depthCollisionsCheckBox->SetEnabled(true);
		sphCheckBox->SetEnabled(true);
		pauseCheckBox->SetEnabled(true);
		maxParticlesSlider->SetEnabled(true);
		emitCountSlider->SetEnabled(true);
		emitSizeSlider->SetEnabled(true);
		emitRotationSlider->SetEnabled(true);
		emitNormalSlider->SetEnabled(true);
		emitScalingSlider->SetEnabled(true);
		emitLifeSlider->SetEnabled(true);
		emitRandomnessSlider->SetEnabled(true);
		emitLifeRandomnessSlider->SetEnabled(true);
		emitMotionBlurSlider->SetEnabled(true);
		emitMassSlider->SetEnabled(true);
		timestepSlider->SetEnabled(true);
		sph_h_Slider->SetEnabled(true);
		sph_K_Slider->SetEnabled(true);
		sph_p0_Slider->SetEnabled(true);
		sph_e_Slider->SetEnabled(true);

		shaderTypeComboBox->SetSelected((int)emitter->shaderType);

		sortCheckBox->SetCheck(emitter->IsSorted());
		depthCollisionsCheckBox->SetCheck(emitter->IsDepthCollisionEnabled());
		sphCheckBox->SetCheck(emitter->IsSPHEnabled());
		pauseCheckBox->SetCheck(emitter->IsPaused());
		volumeCheckBox->SetCheck(emitter->IsVolumeEnabled());
		maxParticlesSlider->SetValue((float)emitter->GetMaxParticleCount());

		emitCountSlider->SetValue(emitter->count);
		emitSizeSlider->SetValue(emitter->size);
		emitRotationSlider->SetValue(emitter->rotation);
		emitNormalSlider->SetValue(emitter->normal_factor);
		emitScalingSlider->SetValue(emitter->scaleX);
		emitLifeSlider->SetValue(emitter->life);
		emitRandomnessSlider->SetValue(emitter->random_factor);
		emitLifeRandomnessSlider->SetValue(emitter->random_life);
		emitMotionBlurSlider->SetValue(emitter->motionBlurAmount);
		emitMassSlider->SetValue(emitter->mass);
		timestepSlider->SetValue(emitter->FIXED_TIMESTEP);

		sph_h_Slider->SetValue(emitter->SPH_h);
		sph_K_Slider->SetValue(emitter->SPH_K);
		sph_p0_Slider->SetValue(emitter->SPH_p0);
		sph_e_Slider->SetValue(emitter->SPH_e);
	}
	else
	{
		infoLabel->SetText("No emitter object selected.");

		emitterNameField->SetEnabled(false);
		restartButton->SetEnabled(false);
		shaderTypeComboBox->SetEnabled(false);
		meshComboBox->SetEnabled(false);
		debugCheckBox->SetEnabled(false);
		volumeCheckBox->SetEnabled(false);
		sortCheckBox->SetEnabled(false);
		depthCollisionsCheckBox->SetEnabled(false);
		sphCheckBox->SetEnabled(false);
		pauseCheckBox->SetEnabled(false);
		maxParticlesSlider->SetEnabled(false);
		emitCountSlider->SetEnabled(false);
		emitSizeSlider->SetEnabled(false);
		emitRotationSlider->SetEnabled(false);
		emitNormalSlider->SetEnabled(false);
		emitScalingSlider->SetEnabled(false);
		emitLifeSlider->SetEnabled(false);
		emitRandomnessSlider->SetEnabled(false);
		emitLifeRandomnessSlider->SetEnabled(false);
		emitMotionBlurSlider->SetEnabled(false);
		emitMassSlider->SetEnabled(false);
		timestepSlider->SetEnabled(false);
		sph_h_Slider->SetEnabled(false);
		sph_K_Slider->SetEnabled(false);
		sph_p0_Slider->SetEnabled(false);
		sph_e_Slider->SetEnabled(false);
	}

}

wiEmittedParticle* EmitterWindow::GetEmitter()
{
	if (entity == INVALID_ENTITY)
	{
		return nullptr;
	}

	Scene& scene = wiScene::GetScene();
	wiEmittedParticle* emitter = scene.emitters.GetComponent(entity);

	return emitter;
}

void EmitterWindow::UpdateData()
{
	auto emitter = GetEmitter();
	if (emitter == nullptr)
	{
		return;
	}

	Scene& scene = wiScene::GetScene();

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

	NameComponent* name = scene.names.GetComponent(entity);
	NameComponent* meshName = scene.names.GetComponent(emitter->meshID);

	stringstream ss("");
	ss.precision(2);
	ss << "Emitter Mesh: " << (meshName != nullptr ? meshName->name : "NO EMITTER MESH") << " (" << emitter->meshID << ")" << endl;
	ss << "Memort Budget: " << emitter->GetMemorySizeInBytes() / 1024.0f / 1024.0f << " MB" << endl;
	ss << endl;

	if (emitter->DEBUG)
	{
		auto data = emitter->GetDebugData();

		ss << "Alive Particle Count = " << data.aliveCount << endl;
		ss << "Dead Particle Count = " << data.deadCount << endl;
		ss << "GPU Emit count = " << data.realEmitCount << endl;
	}
	else
	{
		ss << "For additional data, enable [DEBUG]" << endl;
	}

	infoLabel->SetText(ss.str());

	emitterNameField->SetText(name->name);
}
