#include "stdafx.h"
#include "EmitterWindow.h"
#include "MaterialWindow.h"

#include <sstream>

using namespace std;

EmitterWindow::EmitterWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	object = nullptr;
	materialWnd = nullptr;


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	emitterWindow = new wiWindow(GUI, "Emitter Window");
	emitterWindow->SetSize(XMFLOAT2(800, 1024));
	emitterWindow->SetEnabled(false);
	GUI->AddWidget(emitterWindow);

	float x = 150;
	float y = 10;
	float step = 35;


	emitterComboBox = new wiComboBox("Emitter: ");
	emitterComboBox->SetPos(XMFLOAT2(x, y += step));
	emitterComboBox->SetSize(XMFLOAT2(360, 25));
	emitterComboBox->OnSelect([&](wiEventArgs args) {
		if (object != nullptr && args.iValue >= 0)
		{
			SetObject(object);
		}
	});
	emitterComboBox->SetEnabled(false);
	emitterComboBox->SetTooltip("Choose an emitter associated with the selected object");
	emitterWindow->AddWidget(emitterComboBox);


	shaderTypeComboBox = new wiComboBox("ShaderType: ");
	shaderTypeComboBox->SetPos(XMFLOAT2(x, y += step));
	shaderTypeComboBox->SetSize(XMFLOAT2(200, 25));
	shaderTypeComboBox->AddItem("SOFT");
	shaderTypeComboBox->AddItem("SOFT + DISTORTION");
	shaderTypeComboBox->AddItem("SIMPLEST");
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
	sortCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SORTING = args.bValue;
		}
	});
	sortCheckBox->SetCheck(false);
	sortCheckBox->SetTooltip("Enable sorting of the particles. This might slow down performance.");
	emitterWindow->AddWidget(sortCheckBox);


	depthCollisionsCheckBox = new wiCheckBox("Depth Buffer Collisions Enabled: ");
	depthCollisionsCheckBox->SetPos(XMFLOAT2(x + 250, y));
	depthCollisionsCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->DEPTHCOLLISIONS = args.bValue;
		}
	});
	depthCollisionsCheckBox->SetCheck(false);
	depthCollisionsCheckBox->SetTooltip("Enable particle collisions with the depth buffer.");
	emitterWindow->AddWidget(depthCollisionsCheckBox);


	sphCheckBox = new wiCheckBox("SPH - FluidSim: ");
	sphCheckBox->SetPos(XMFLOAT2(x + 400, y));
	sphCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SPH_FLUIDSIMULATION = args.bValue;
		}
	});
	sphCheckBox->SetCheck(false);
	sphCheckBox->SetTooltip("Enable particle collisions with each other. Simulate with Smooth Particle Hydrodynamics (SPH) solver.");
	emitterWindow->AddWidget(sphCheckBox);


	pauseCheckBox = new wiCheckBox("PAUSE: ");
	pauseCheckBox->SetPos(XMFLOAT2(x, y += step));
	pauseCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->PAUSED = args.bValue;
		}
	});
	pauseCheckBox->SetCheck(false);
	pauseCheckBox->SetTooltip("Stop simulation update.");
	emitterWindow->AddWidget(pauseCheckBox);


	debugCheckBox = new wiCheckBox("DEBUG: ");
	debugCheckBox->SetPos(XMFLOAT2(x + 120, y));
	debugCheckBox->OnClick([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->DEBUG = args.bValue;
		}
	});
	debugCheckBox->SetCheck(false);
	debugCheckBox->SetTooltip("Enable debug info for the emitter. This involves reading back GPU data, so rendering can slow down.");
	emitterWindow->AddWidget(debugCheckBox);



	infoLabel = new wiLabel("EmitterInfo");
	infoLabel->SetSize(XMFLOAT2(380, 140));
	infoLabel->SetPos(XMFLOAT2(x, y += step));
	emitterWindow->AddWidget(infoLabel);


	maxParticlesSlider = new wiSlider(100.0f, 1000000.0f, 10000, 100000, "Max particle count: ");
	maxParticlesSlider->SetSize(XMFLOAT2(360, 30));
	maxParticlesSlider->SetPos(XMFLOAT2(x, y += step + 140));
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
	emitCountSlider->SetSize(XMFLOAT2(360, 30));
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
	emitSizeSlider->SetSize(XMFLOAT2(360, 30));
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
	emitRotationSlider->SetSize(XMFLOAT2(360, 30));
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
	emitNormalSlider->SetSize(XMFLOAT2(360, 30));
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
	emitScalingSlider->SetSize(XMFLOAT2(360, 30));
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

	emitLifeSlider = new wiSlider(0.0f, 1000.0f, 1.0f, 100000, "Life span: ");
	emitLifeSlider->SetSize(XMFLOAT2(360, 30));
	emitLifeSlider->SetPos(XMFLOAT2(x, y += step));
	emitLifeSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->life = args.fValue;
		}
	});
	emitLifeSlider->SetEnabled(false);
	emitLifeSlider->SetTooltip("Set the lifespan of the emitted particles.");
	emitterWindow->AddWidget(emitLifeSlider);

	emitRandomnessSlider = new wiSlider(0.0f, 1.0f, 1.0f, 100000, "Randomness: ");
	emitRandomnessSlider->SetSize(XMFLOAT2(360, 30));
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
	emitLifeRandomnessSlider->SetSize(XMFLOAT2(360, 30));
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
	emitMotionBlurSlider->SetSize(XMFLOAT2(360, 30));
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
	emitMassSlider->SetSize(XMFLOAT2(360, 30));
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





	//////////////// SPH ////////////////////////////

	y += step;

	sph_h_Slider = new wiSlider(0.1f, 100.0f, 1.0f, 100000, "SPH Smoothing Radius (h): ");
	sph_h_Slider->SetSize(XMFLOAT2(360, 30));
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
	sph_K_Slider->SetSize(XMFLOAT2(360, 30));
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
	sph_p0_Slider->SetSize(XMFLOAT2(360, 30));
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
	sph_e_Slider->SetSize(XMFLOAT2(360, 30));
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








	////// UTIL /////////////////

	y += step;

	materialSelectButton = new wiButton("Select Material");
	materialSelectButton->SetPos(XMFLOAT2(x, y += step));
	materialSelectButton->SetSize(XMFLOAT2(150, 30));
	materialSelectButton->OnClick([&](wiEventArgs args) {
		if (materialWnd != nullptr)
		{
			auto emitter = GetEmitter();
			if (emitter != nullptr)
			{
				materialWnd->SetMaterial(emitter->material);
			}
		}
	});
	materialSelectButton->SetTooltip("Select corresponding material in the Material Window.");
	emitterWindow->AddWidget(materialSelectButton);



	restartButton = new wiButton("Restart Emitter");
	restartButton->SetPos(XMFLOAT2(x + 200, y));
	restartButton->SetSize(XMFLOAT2(150, 30));
	restartButton->OnClick([&](wiEventArgs args) {
		if (materialWnd != nullptr)
		{
			auto emitter = GetEmitter();
			if (emitter != nullptr)
			{
				emitter->Restart();
			}
		}
	});
	restartButton->SetTooltip("Restart particle system emitter");
	emitterWindow->AddWidget(restartButton);




	emitterWindow->Translate(XMFLOAT3(200, 50, 0));
	emitterWindow->SetVisible(false);

	SetObject(nullptr);
}


EmitterWindow::~EmitterWindow()
{
	emitterWindow->RemoveWidgets(true);
	GUI->RemoveWidget(emitterWindow);
	SAFE_DELETE(emitterWindow);
}

void EmitterWindow::SetObject(Object* obj)
{
	if (this->object == obj)
		return;

	// first try to turn off any debug readbacks for emitters:
	if (GetEmitter() != nullptr)
	{
		GetEmitter()->DEBUG = false;
	}
	debugCheckBox->SetCheck(false);

	object = obj;

	emitterComboBox->ClearItems();

	if (object != nullptr)
	{
		for (auto& x : object->eParticleSystems)
		{
			emitterComboBox->AddItem(x->name);
			emitterComboBox->SetEnabled(true);
		}

		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			sortCheckBox->SetCheck(emitter->SORTING);
			depthCollisionsCheckBox->SetCheck(emitter->DEPTHCOLLISIONS);
			sphCheckBox->SetCheck(emitter->SPH_FLUIDSIMULATION);
			pauseCheckBox->SetCheck(emitter->PAUSED);
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

			sph_h_Slider->SetValue(emitter->SPH_h);
			sph_K_Slider->SetValue(emitter->SPH_K);
			sph_p0_Slider->SetValue(emitter->SPH_p0);
			sph_e_Slider->SetValue(emitter->SPH_e);
		}

		emitterWindow->SetEnabled(true);
	}
	else
	{
		infoLabel->SetText("No emitter object selected.");

		emitterWindow->SetEnabled(false);
	}

}

void EmitterWindow::SetMaterialWnd(MaterialWindow* wnd)
{
	materialWnd = wnd;
}

wiEmittedParticle* EmitterWindow::GetEmitter()
{
	if (object == nullptr)
	{
		return nullptr;
	}

	int sel = emitterComboBox->GetSelected();

	if (sel < 0)
	{
		return nullptr;
	}

	if ((int)object->eParticleSystems.size() > sel)
	{
		return object->eParticleSystems[sel];
	}

	return nullptr;
}

void EmitterWindow::UpdateData()
{
	auto emitter = GetEmitter();
	if (emitter == nullptr)
	{
		return;
	}


	stringstream ss("");
	ss.precision(2);
	ss << "Emitter name: " << emitter->name << endl;
	ss << "Emitter Object: " << (emitter->object != nullptr ? emitter->object->name : "ERROR: NO EMITTER OBJECT") << endl;
	ss << "Emitter Material: " << (emitter->material != nullptr ? emitter->material->name : "ERROR: NO EMITTER MATERIAL") << endl;
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
}
