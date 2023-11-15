#include "stdafx.h"
#include "HairParticleWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void HairParticleWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_HAIR " Hair Particle System", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(600, 350));

	closeButton.SetTooltip("Delete HairParticleSystem");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().hairs.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 120;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 150;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(wid, 80));
	AddWidget(&infoLabel);

	meshComboBox.Create("Mesh: ");
	meshComboBox.SetSize(XMFLOAT2(wid, hei));
	meshComboBox.SetPos(XMFLOAT2(x, y));
	meshComboBox.SetEnabled(false);
	meshComboBox.OnSelect([&](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			if (args.iValue == 0)
			{
				hair->meshID = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = editor->GetCurrentScene();
				hair->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
		}
	});
	meshComboBox.SetTooltip("Choose a mesh where hair will grow from...");
	AddWidget(&meshComboBox);

	countSlider.Create(0, 100000, 1000, 100000, "Strand Count: ");
	countSlider.SetSize(XMFLOAT2(wid, hei));
	countSlider.SetPos(XMFLOAT2(x, y += step));
	countSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->strandCount = (uint32_t)args.iValue;
		}
	});
	countSlider.SetEnabled(false);
	countSlider.SetTooltip("Set hair strand count");
	AddWidget(&countSlider);

	lengthSlider.Create(0, 4, 1, 100000, "Length: ");
	lengthSlider.SetSize(XMFLOAT2(wid, hei));
	lengthSlider.SetPos(XMFLOAT2(x, y += step));
	lengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->length = args.fValue;
		}
	});
	lengthSlider.SetEnabled(false);
	lengthSlider.SetTooltip("Set hair strand length");
	AddWidget(&lengthSlider);

	stiffnessSlider.Create(0, 20, 5, 100000, "Stiffness: ");
	stiffnessSlider.SetSize(XMFLOAT2(wid, hei));
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->stiffness = args.fValue;
		}
	});
	stiffnessSlider.SetEnabled(false);
	stiffnessSlider.SetTooltip("Set hair strand stiffness, how much it tries to get back to rest position.");
	AddWidget(&stiffnessSlider);

	randomnessSlider.Create(0, 1, 0.2f, 100000, "Randomness: ");
	randomnessSlider.SetSize(XMFLOAT2(wid, hei));
	randomnessSlider.SetPos(XMFLOAT2(x, y += step));
	randomnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->randomness = args.fValue;
		}
	});
	randomnessSlider.SetEnabled(false);
	randomnessSlider.SetTooltip("Set hair length randomization factor. This will affect randomness of hair lengths.");
	AddWidget(&randomnessSlider);

	//segmentcountSlider.Create(1, 10, 1, 9, "Segment Count: ");
	//segmentcountSlider.SetSize(XMFLOAT2(wid, hei));
	//segmentcountSlider.SetPos(XMFLOAT2(x, y += step));
	//segmentcountSlider.OnSlide([&](wi::gui::EventArgs args) {
	//	auto hair = GetHair();
	//	if (hair != nullptr)
	//	{
	//		hair->segmentCount = (uint32_t)args.iValue;
	//	}
	//});
	//segmentcountSlider.SetEnabled(false);
	//segmentcountSlider.SetTooltip("Set hair strand segment count. This will affect simulation quality and performance.");
	//AddWidget(&segmentcountSlider);

	randomSeedSlider.Create(1, 12345, 1, 12344, "Random seed: ");
	randomSeedSlider.SetSize(XMFLOAT2(wid, hei));
	randomSeedSlider.SetPos(XMFLOAT2(x, y += step));
	randomSeedSlider.OnSlide([&](wi::gui::EventArgs args) {
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
	viewDistanceSlider.SetSize(XMFLOAT2(wid, hei));
	viewDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	viewDistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
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
	framesXInput.SetDescription("Frames: ");
	framesXInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->framesX = (uint32_t)args.iValue;
		}
	});
	AddWidget(&framesXInput);

	framesYInput.Create("");
	framesYInput.SetPos(XMFLOAT2(x, y += step));
	framesYInput.SetSize(XMFLOAT2(40, hei));
	framesYInput.SetText("");
	framesYInput.SetTooltip("How many vertical frames there are in the spritesheet.");
	framesYInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	frameCountInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->frameCount = (uint32_t)args.iValue;
		}
		});
	AddWidget(&frameCountInput);

	frameStartInput.Create("");
	frameStartInput.SetPos(XMFLOAT2(x, y += step));
	frameStartInput.SetSize(XMFLOAT2(40, hei));
	frameStartInput.SetText("");
	frameStartInput.SetTooltip("Specifies the first frame of the sheet that can be used.");
	frameStartInput.SetDescription("First Frame: ");
	frameStartInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		auto hair = GetHair();
		if (hair != nullptr)
		{
			hair->frameStart = (uint32_t)args.iValue;
		}
		});
	AddWidget(&frameStartInput);




	SetMinimized(true);
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
		//segmentcountSlider.SetValue((float)hair->segmentCount);
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

}

wi::HairParticleSystem* HairParticleWindow::GetHair()
{
	if (entity == INVALID_ENTITY)
	{
		return nullptr;
	}

	Scene& scene = editor->GetCurrentScene();
	wi::HairParticleSystem* hair = scene.hairs.GetComponent(entity);

	return hair;
}

void HairParticleWindow::UpdateData()
{
	auto hair = GetHair();
	if (hair == nullptr)
	{
		return;
	}

	Scene& scene = editor->GetCurrentScene();

	std::string ss;
	ss += "To use hair particle system, first you must select a surface mesh to spawn particles on.\n\n";
	ss += "Position format: " + std::string(wi::graphics::GetFormatString(hair->position_format)) + "\n";
	ss += "Memory usage: " + wi::helper::GetMemorySizeText(hair->GetMemorySizeInBytes()) + "\n";
	infoLabel.SetText(ss);

	meshComboBox.ClearItems();
	meshComboBox.AddItem("NO MESH");
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		Entity entity = scene.meshes.GetEntity(i);
		const NameComponent& name = *scene.names.GetComponent(entity);
		meshComboBox.AddItem(name.name);

		if (hair->meshID == entity)
		{
			meshComboBox.SetSelected((int)i + 1);
		}
	}
}

void HairParticleWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 100;
	const float margin_right = 40;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(infoLabel);
	add(meshComboBox);
	add(lengthSlider);
	add(stiffnessSlider);
	add(randomnessSlider);
	add(countSlider);
	add(randomSeedSlider);
	add(viewDistanceSlider);

	const float l = margin_left;
	const float r = width - margin_right;
	const float w = ((r - l) - padding) / 2.0f;
	framesXInput.SetSize(XMFLOAT2(w, framesXInput.GetSize().y));
	framesYInput.SetSize(XMFLOAT2(w, framesYInput.GetSize().y));
	framesXInput.SetPos(XMFLOAT2(margin_left, y));
	framesYInput.SetPos(XMFLOAT2(framesXInput.GetPos().x + w + padding, y));

	y += framesYInput.GetSize().y;
	y += padding;

	add(frameCountInput);
	add(frameStartInput);

}
