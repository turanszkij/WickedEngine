#include "stdafx.h"
#include "EmitterWindow.h"

#include <sstream>

using namespace std;

EmitterWindow::EmitterWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	object = nullptr;


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	emitterWindow = new wiWindow(GUI, "Emitter Window");
	emitterWindow->SetSize(XMFLOAT2(600, 800));
	emitterWindow->SetEnabled(false);
	GUI->AddWidget(emitterWindow);

	float x = 150;
	float y = 10;
	float step = 35;


	emitterComboBox = new wiComboBox("Emitter: ");
	emitterComboBox->SetPos(XMFLOAT2(x, y += step));
	emitterComboBox->SetSize(XMFLOAT2(300, 25));
	emitterComboBox->OnSelect([&](wiEventArgs args) {
		if (object != nullptr && args.iValue >= 0)
		{
			SetObject(object);
		}
	});
	emitterComboBox->SetEnabled(false);
	emitterComboBox->SetTooltip("Choose an emitter associated with the selected object");
	emitterWindow->AddWidget(emitterComboBox);



	memoryBudgetLabel = new wiLabel("Memory budget: -");
	memoryBudgetLabel->SetSize(XMFLOAT2(200, 25));
	memoryBudgetLabel->SetPos(XMFLOAT2(x, y += step));
	emitterWindow->AddWidget(memoryBudgetLabel);


	maxParticlesSlider = new wiSlider(100.0f, 1000000.0f, 10000, 100000, "Max particle count: ");
	maxParticlesSlider->SetSize(XMFLOAT2(200, 30));
	maxParticlesSlider->SetPos(XMFLOAT2(x, y += step));
	maxParticlesSlider->OnSlide([&](wiEventArgs args) {
		auto emitter = GetEmitter();
		if (emitter != nullptr)
		{
			emitter->SetMaxParticleCount((uint32_t)args.iValue);

			stringstream ss("");
			ss.precision(2);
			ss << "Memort Budget: " << emitter->GetMemorySizeInBytes() / 1024.0f / 1024.0f << " MB";
			memoryBudgetLabel->SetText(ss.str());
		}
	});
	maxParticlesSlider->SetEnabled(false);
	maxParticlesSlider->SetTooltip("Set the maximum amount of particles this system can handle. This has an effect on the memory budget.");
	emitterWindow->AddWidget(maxParticlesSlider);

	y += 30;

	//////////////////////////////////////////////////////////////////////////////////////////////////

	emitCountSlider = new wiSlider(0.0f, 1000.0f, 1.0f, 100000, "Emit count per sec: ");
	emitCountSlider->SetSize(XMFLOAT2(200, 30));
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
	emitSizeSlider->SetSize(XMFLOAT2(200, 30));
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
	emitRotationSlider->SetSize(XMFLOAT2(200, 30));
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
	emitNormalSlider->SetSize(XMFLOAT2(200, 30));
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
	emitScalingSlider->SetSize(XMFLOAT2(200, 30));
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
	emitLifeSlider->SetSize(XMFLOAT2(200, 30));
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

	emitRandomnessSlider = new wiSlider(0.0f, 1000.0f, 1.0f, 100000, "Randomness: ");
	emitRandomnessSlider->SetSize(XMFLOAT2(200, 30));
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
	emitLifeRandomnessSlider->SetSize(XMFLOAT2(200, 30));
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
	emitMotionBlurSlider->SetSize(XMFLOAT2(200, 30));
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
	object = obj;

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
			maxParticlesSlider->SetValue((float)emitter->GetMaxParticleCount());

			stringstream ss("");
			ss.precision(2);
			ss << "Memort Budget: " << emitter->GetMemorySizeInBytes() / 1024.0f / 1024.0f << " MB";
			memoryBudgetLabel->SetText(ss.str());

			emitCountSlider->SetValue(emitter->count);
			emitSizeSlider->SetValue(emitter->size);
			emitRotationSlider->SetValue(emitter->rotation);
			emitNormalSlider->SetValue(emitter->normal_factor);
			emitScalingSlider->SetValue(emitter->scaleX);
			emitLifeSlider->SetValue(emitter->life);
			emitRandomnessSlider->SetValue(emitter->random_factor);
			emitLifeRandomnessSlider->SetValue(emitter->random_life);
			emitMotionBlurSlider->SetValue(emitter->motionBlurAmount);
		}

		emitterWindow->SetEnabled(true);
	}
	else
	{
		emitterComboBox->ClearItems();
		memoryBudgetLabel->SetText("Memory Budget: -");

		emitterWindow->SetEnabled(false);
	}

}

wiEmittedParticle* EmitterWindow::GetEmitter()
{
	if (object == nullptr)
	{
		return nullptr;
	}

	int sel = emitterComboBox->GetSelected();

	if ((int)object->eParticleSystems.size() > sel)
	{
		return object->eParticleSystems[sel];
	}

	return nullptr;
}
