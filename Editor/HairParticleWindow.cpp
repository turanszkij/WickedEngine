#include "stdafx.h"
#include "HairParticleWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void HairParticleWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_HAIR " Hair Particle System", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(600, 1000));

	closeButton.SetTooltip("Delete HairParticleSystem");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().hairs.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 120;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 150;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(wid, 200));
	AddWidget(&infoLabel);

	meshComboBox.Create("Mesh: ");
	meshComboBox.SetSize(XMFLOAT2(wid, hei));
	meshComboBox.SetPos(XMFLOAT2(x, y));
	meshComboBox.SetEnabled(false);
	meshComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			if (args.iValue == 0)
			{
				hair->meshID = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = editor->GetCurrentScene();
				hair->meshID = scene.meshes.GetEntity(args.iValue - 1);
			}
			hair->SetDirty();
		}
	});
	meshComboBox.SetTooltip("Choose a mesh where hair will grow from...");
	AddWidget(&meshComboBox);

	countSlider.Create(0, 100000, 1000, 100000, "Strand Count: ");
	countSlider.SetSize(XMFLOAT2(wid, hei));
	countSlider.SetPos(XMFLOAT2(x, y += step));
	countSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->strandCount = (uint32_t)args.iValue;
			hair->SetDirty();
		}
	});
	countSlider.SetEnabled(false);
	countSlider.SetTooltip("Set hair strand count");
	AddWidget(&countSlider);

	lengthSlider.Create(0, 4, 1, 1000, "Length: ");
	lengthSlider.SetSize(XMFLOAT2(wid, hei));
	lengthSlider.SetPos(XMFLOAT2(x, y += step));
	lengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->length = args.fValue;
			hair->SetDirty();
		}
	});
	lengthSlider.SetEnabled(false);
	lengthSlider.SetTooltip("Set hair strand length. This uniformly scales hair particles.");
	AddWidget(&lengthSlider);

	widthSlider.Create(0, 2, 1, 1000, "Width: ");
	widthSlider.SetSize(XMFLOAT2(wid, hei));
	widthSlider.SetPos(XMFLOAT2(x, y += step));
	widthSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->width = args.fValue;
			hair->SetDirty();
		}
		});
	widthSlider.SetEnabled(false);
	widthSlider.SetTooltip("Set hair strand width multiplier. This only scales the hair particles horizontally.");
	AddWidget(&widthSlider);

	stiffnessSlider.Create(0, 20, 5, 1000, "Stiffness: ");
	stiffnessSlider.SetSize(XMFLOAT2(wid, hei));
	stiffnessSlider.SetPos(XMFLOAT2(x, y += step));
	stiffnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->stiffness = args.fValue;
			hair->SetDirty();
		}
	});
	stiffnessSlider.SetEnabled(false);
	stiffnessSlider.SetTooltip("Set hair strand stiffness, how much it tries to get back to rest position.");
	AddWidget(&stiffnessSlider);

	randomnessSlider.Create(0, 1, 0.2f, 1000, "Randomness: ");
	randomnessSlider.SetSize(XMFLOAT2(wid, hei));
	randomnessSlider.SetPos(XMFLOAT2(x, y += step));
	randomnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->randomness = args.fValue;
			hair->SetDirty();
		}
	});
	randomnessSlider.SetEnabled(false);
	randomnessSlider.SetTooltip("Set hair length randomization factor. This will affect randomness of hair lengths.");
	AddWidget(&randomnessSlider);

	//segmentcountSlider.Create(1, 10, 1, 9, "Segment Count: ");
	//segmentcountSlider.SetSize(XMFLOAT2(wid, hei));
	//segmentcountSlider.SetPos(XMFLOAT2(x, y += step));
	//segmentcountSlider.OnSlide([&](wi::gui::EventArgs args) {
	//	wi::scene::Scene& scene = editor->GetCurrentScene();
	//	for (auto& x : editor->translator.selected)
	//	{
	//		wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
	//		if (hair == nullptr)
	//			continue;
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
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->randomSeed = (uint32_t)args.iValue;
			hair->SetDirty();
		}
	});
	randomSeedSlider.SetEnabled(false);
	randomSeedSlider.SetTooltip("Set hair system-wide random seed value. This will affect hair patch placement randomization.");
	AddWidget(&randomSeedSlider);

	viewDistanceSlider.Create(0, 1000, 100, 10000, "View distance: ");
	viewDistanceSlider.SetSize(XMFLOAT2(wid, hei));
	viewDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	viewDistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->viewDistance = args.fValue;
			hair->SetDirty();
		}
		});
	viewDistanceSlider.SetEnabled(false);
	viewDistanceSlider.SetTooltip("Set view distance. After this, particles will be faded out.");
	AddWidget(&viewDistanceSlider);

	uniformitySlider.Create(0.01f, 2.0f, 0.1f, 1000, "Uniformity: ");
	uniformitySlider.SetSize(XMFLOAT2(wid, hei));
	uniformitySlider.SetPos(XMFLOAT2(x, y += step));
	uniformitySlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(x.entity);
			if (hair == nullptr)
				continue;
			hair->uniformity = args.fValue;
			hair->SetDirty();
		}
		});
	uniformitySlider.SetTooltip("How much the sprite selection distribution noise is modulated by particle positions.");
	AddWidget(&uniformitySlider);

	addSpriteButton.Create("Select sprite rect");
	addSpriteButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();
		if (!scene.materials.Contains(entity))
		{
			scene.materials.Create(entity);
		}
		const MaterialComponent* material = scene.materials.GetComponent(entity);
		wi::Sprite sprite;
		sprite.textureResource = material->textures[MaterialComponent::BASECOLORMAP].resource;
		if (!sprite.textureResource.IsValid() || !sprite.textureResource.GetTexture().IsValid())
		{
			sprite.textureResource.SetTexture(*wi::texturehelper::getWhite());
		}

		spriterectwnd.SetVisible(true);

		spriterectwnd.SetSprite(sprite);
		spriterectwnd.ResetSelection();

		spriterectwnd.OnAccepted([=]() {

			wi::HairParticleSystem* hair = GetHair();
			if (hair == nullptr)
				return;

			hair->atlas_rects.emplace_back().texMulAdd = spriterectwnd.muladd;
			hair->SetDirty();

			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				RefreshSprites();
			});
		});
	});
	addSpriteButton.SetTooltip("Add a new sprite to the atlas selection to create particle variety.\nIf no sprites are added, then simply the whole texture will be used from the material.");
	AddWidget(&addSpriteButton);

	spriterectwnd.Create(editor);
	editor->GetGUI().AddWidget(&spriterectwnd);
	spriterectwnd.SetVisible(false);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(entity);
}

void HairParticleWindow::RefreshSprites()
{
	for (auto& x : sprites)
	{
		RemoveWidget(&x);
	}
	sprites.clear();

	for (auto& x : spriteRemoveButtons)
	{
		RemoveWidget(&x);
	}
	spriteRemoveButtons.clear();

	for (auto& x : spriteSizeSliders)
	{
		RemoveWidget(&x);
	}
	spriteSizeSliders.clear();

	auto hair = GetHair();
	if (hair == nullptr)
		return;

	Scene& scene = editor->GetCurrentScene();
	if (!scene.materials.Contains(entity))
		return;
	const MaterialComponent* material = scene.materials.GetComponent(entity);
	wi::Sprite sprite;
	sprite.textureResource = material->textures[MaterialComponent::BASECOLORMAP].resource;
	if (!sprite.textureResource.IsValid() || !sprite.textureResource.GetTexture().IsValid())
		return;

	const wi::graphics::TextureDesc& desc = sprite.textureResource.GetTexture().GetDesc();

	sprites.resize(hair->atlas_rects.size());
	spriteRemoveButtons.resize(hair->atlas_rects.size());
	spriteSizeSliders.resize(hair->atlas_rects.size());
	for (size_t i = 0; i < sprites.size(); ++i)
	{
		XMFLOAT4 rect = hair->atlas_rects[i].texMulAdd;

		auto& x = sprites[i];
		x.Create("");
		x.SetLocalizationEnabled(false);
		for (auto& y : x.sprites)
		{
			y = sprite;
			y.params.enableDrawRect(XMFLOAT4(rect.z * desc.width, rect.w * desc.height, rect.x * desc.width, rect.y * desc.height));
		}
		x.OnClick([=](wi::gui::EventArgs args) {

			spriterectwnd.SetVisible(true);

			spriterectwnd.SetSprite(sprite);
			spriterectwnd.ResetSelection();

			spriterectwnd.OnAccepted([=]() {

				wi::HairParticleSystem* hair = GetHair();
				if (hair == nullptr)
					return;

				hair->atlas_rects[i].texMulAdd = spriterectwnd.muladd;
				hair->SetDirty();

				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					RefreshSprites();
				});
			});
		});
		AddWidget(&x);

		auto& r = spriteRemoveButtons[i];
		r.Create("X");
		r.SetLocalizationEnabled(false);
		r.OnClick([=](wi::gui::EventArgs args) {

			hair->atlas_rects.erase(hair->atlas_rects.begin() + i);
			hair->SetDirty();

			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				RefreshSprites();
				});
		});
		AddWidget(&r);

		auto& s = spriteSizeSliders[i];
		s.Create(0, 2, 1, 100, "");
		s.SetValue(hair->atlas_rects[i].size);
		s.SetLocalizationEnabled(false);
		s.SetTooltip("Adjust sprite's overall size");
		s.OnSlide([=](wi::gui::EventArgs args) {

			hair->atlas_rects[i].size = args.fValue;
			hair->SetDirty();

		});
		AddWidget(&s);
	}

	editor->generalWnd.themeCombo.SetSelected(editor->generalWnd.themeCombo.GetSelected()); // theme callback
}

void HairParticleWindow::SetEntity(Entity entity)
{
	bool changed = this->entity != entity;
	this->entity = entity;

	auto hair = GetHair();

	if (hair != nullptr)
	{
		lengthSlider.SetValue(hair->length);
		widthSlider.SetValue(hair->width);
		stiffnessSlider.SetValue(hair->stiffness);
		randomnessSlider.SetValue(hair->randomness);
		countSlider.SetValue((float)hair->strandCount);
		//segmentcountSlider.SetValue((float)hair->segmentCount);
		randomSeedSlider.SetValue((float)hair->randomSeed);
		viewDistanceSlider.SetValue(hair->viewDistance);
		uniformitySlider.SetValue(hair->uniformity);

		const MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (changed || material->IsDirty())
		{
			RefreshSprites();
		}

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
	ss += "To use hair particle system, first you must select a surface mesh to spawn particles on, and then increase particle count to grow particles. The particles will get their texture from the Material that is created on the current entity.\n\n";
	ss += "Position format: " + std::string(wi::graphics::GetFormatString(hair->position_format)) + "\n";
	ss += "Memory usage: " + wi::helper::GetMemorySizeText(hair->GetMemorySizeInBytes()) + "\n";
	infoLabel.SetText(ss);

	meshComboBox.ClearItems();
	meshComboBox.AddItem("NO MESH");
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		Entity entity = scene.meshes.GetEntity(i);
		const NameComponent* name = scene.names.GetComponent(entity);
		std::string name_string;
		if (name == nullptr)
		{
			name_string = "[no_name] " + std::to_string(entity);
		}
		else if (name->name.empty())
		{
			name_string = "[name_empty] " + std::to_string(entity);
		}
		else
		{
			name_string = name->name;
		}
		meshComboBox.AddItem(name_string);

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
	add(countSlider);
	add(lengthSlider);
	add(widthSlider);
	add(stiffnessSlider);
	add(randomnessSlider);
	add(randomSeedSlider);
	add(viewDistanceSlider);
	add(uniformitySlider);

	y += jump;
	add_fullwidth(addSpriteButton);

	const float preview_size = 100;
	const float border = 40 * preview_size / 100.0f;
	int cells = std::max(1, int(GetWidgetAreaSize().x / (preview_size + border)));
	int i = 0;
	for (auto& x : sprites)
	{
		x.SetSize(XMFLOAT2(preview_size, preview_size));
		x.SetPos(XMFLOAT2((i % cells) * (preview_size + border) + padding, y));
		if ((i % cells) == (cells - 1))
		{
			y += preview_size + 20;
		}
		auto& r = spriteRemoveButtons[i];
		r.SetPos(XMFLOAT2(x.GetPos().x + x.GetSize().x + 1, x.GetPos().y));
		r.SetSize(XMFLOAT2(14, 14));
		auto& s = spriteSizeSliders[i];
		s.SetPos(XMFLOAT2(x.GetPos().x, x.GetPos().y + x.GetSize().y + 2));
		s.SetSize(XMFLOAT2(x.GetSize().x, 14));
		i++;
	}

	if (IsVisible())
	{
		Scene& scene = editor->GetCurrentScene();
		if (scene.materials.Contains(entity))
		{
			MaterialComponent* material = scene.materials.GetComponent(entity);
			if (material->textures[BASECOLORMAP].resource.IsValid())
			{
				material->textures[BASECOLORMAP].resource.StreamingRequestResolution(65536);
			}
		}
	}

}
