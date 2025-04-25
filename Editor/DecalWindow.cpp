#include "stdafx.h"
#include "DecalWindow.h"

using namespace wi::ecs;
using namespace wi::scene;


void DecalWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_DECAL " Decal", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(300, 200));

	closeButton.SetTooltip("Delete DecalComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().decals.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	placementCheckBox.Create("Decal Placement Enabled: ");
	placementCheckBox.SetCheck(false);
	placementCheckBox.SetTooltip("Enable decal placement. Use the left mouse button to place decals to the scene.");
	AddWidget(&placementCheckBox);

	onlyalphaCheckBox.Create("Alpha only basecolor: ");
	onlyalphaCheckBox.SetTooltip("You can enable this to only use alpha channel from basecolor map. Useful for blending normalmap-only decals.");
	onlyalphaCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			DecalComponent* decal = scene.decals.GetComponent(x.entity);
			if (decal == nullptr)
				continue;
			decal->SetBaseColorOnlyAlpha(args.bValue);
		}
	});
	AddWidget(&onlyalphaCheckBox);

	slopeBlendPowerSlider.Create(0, 8, 0, 1000, "Slope Blend: ");
	slopeBlendPowerSlider.SetTooltip("Set a power factor for blending on surface slopes. 0 = no slope blend, increasing = more slope blend");
	slopeBlendPowerSlider.OnSlide([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			DecalComponent* decal = scene.decals.GetComponent(x.entity);
			if (decal == nullptr)
				continue;
			decal->slopeBlendPower = args.fValue;
		}
	});
	AddWidget(&slopeBlendPowerSlider);

	infoLabel.Create("");
	infoLabel.SetText("Set decal properties in the Material component. Decals support the following material properties:\n - Base color\n - Base color texture\n - Emissive strength\n - Normalmap texture\n - Normalmap strength\n - Surfacemap texture\n - Texture tiling (TexMulAdd)");
	infoLabel.SetColor(wi::Color::Transparent());
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void DecalWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const DecalComponent* decal = scene.decals.GetComponent(entity);

	if (decal != nullptr)
	{
		SetEnabled(true);
		onlyalphaCheckBox.SetCheck(decal->IsBaseColorOnlyAlpha());
		slopeBlendPowerSlider.SetValue(decal->slopeBlendPower);
	}
	else
	{
		SetEnabled(false);
	}
}

void DecalWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 100;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = padding;
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
	add_right(placementCheckBox);
	add_right(onlyalphaCheckBox);
	add(slopeBlendPowerSlider);

}
