#include "stdafx.h"
#include "PostprocessWindow.h"

#include <thread>

using namespace std;
using namespace wiGraphics;


PostprocessWindow::PostprocessWindow(wiGUI* gui, RenderPath3D* comp) : GUI(gui), component(comp)
{
	assert(component && "PostprocessWnd invalid component!");
	assert(GUI && "Invalid GUI!");

	ppWindow = new wiWindow(GUI, "PostProcess Window");
	ppWindow->SetSize(XMFLOAT2(400, 700));
	GUI->AddWidget(ppWindow);

	float x = 150;
	float y = 10;
	float step = 30;

	exposureSlider = new wiSlider(0.0f, 3.0f, 1, 10000, "Exposure: ");
	exposureSlider->SetTooltip("Set the tonemap exposure value");
	exposureSlider->SetScriptTip("RenderPath3D::SetExposure(float value)");
	exposureSlider->SetSize(XMFLOAT2(100, 20));
	exposureSlider->SetPos(XMFLOAT2(x, y += step));
	exposureSlider->SetValue(component->getExposure());
	exposureSlider->OnSlide([&](wiEventArgs args) {
		component->setExposure(args.fValue);
	});
	ppWindow->AddWidget(exposureSlider);

	lensFlareCheckBox = new wiCheckBox("LensFlare: ");
	lensFlareCheckBox->SetTooltip("Toggle visibility of light source flares. Additional setup needed per light for a lensflare to be visible.");
	lensFlareCheckBox->SetScriptTip("RenderPath3D::SetLensFlareEnabled(bool value)");
	lensFlareCheckBox->SetPos(XMFLOAT2(x, y += step));
	lensFlareCheckBox->SetCheck(component->getLensFlareEnabled());
	lensFlareCheckBox->OnClick([&](wiEventArgs args) {
		component->setLensFlareEnabled(args.bValue);
	});
	ppWindow->AddWidget(lensFlareCheckBox);

	lightShaftsCheckBox = new wiCheckBox("LightShafts: ");
	lightShaftsCheckBox->SetTooltip("Enable light shaft for directional light sources.");
	lightShaftsCheckBox->SetScriptTip("RenderPath3D::SetLightShaftsEnabled(bool value)");
	lightShaftsCheckBox->SetPos(XMFLOAT2(x, y += step));
	lightShaftsCheckBox->SetCheck(component->getLightShaftsEnabled());
	lightShaftsCheckBox->OnClick([&](wiEventArgs args) {
		component->setLightShaftsEnabled(args.bValue);
	});
	ppWindow->AddWidget(lightShaftsCheckBox);

	aoComboBox = new wiComboBox("AO: ");
	aoComboBox->SetTooltip("Choose Ambient Occlusion type");
	aoComboBox->SetScriptTip("RenderPath3D::SetSSAOEnabled(bool value)");
	aoComboBox->SetPos(XMFLOAT2(x, y += step));
	aoComboBox->AddItem("Disabled");
	aoComboBox->AddItem("SSAO");
	aoComboBox->AddItem("HBAO");
	aoComboBox->AddItem("MSAO");
	aoComboBox->SetSelected(component->getAO());
	aoComboBox->OnSelect([&](wiEventArgs args) {
		component->setAO((RenderPath3D::AO)args.iValue);
	});
	ppWindow->AddWidget(aoComboBox);

	aoPowerSlider = new wiSlider(0.25f, 8.0f, 2, 1000, "Power: ");
	aoPowerSlider->SetTooltip("Set SSAO Power. Higher values produce darker, more pronounced effect");
	aoPowerSlider->SetSize(XMFLOAT2(100, 20));
	aoPowerSlider->SetPos(XMFLOAT2(x + 100, y += step));
	aoPowerSlider->SetValue((float)component->getAOPower());
	aoPowerSlider->OnSlide([&](wiEventArgs args) {
		component->setAOPower(args.fValue);
		});
	ppWindow->AddWidget(aoPowerSlider);

	ssrCheckBox = new wiCheckBox("SSR: ");
	ssrCheckBox->SetTooltip("Enable Screen Space Reflections.");
	ssrCheckBox->SetScriptTip("RenderPath3D::SetSSREnabled(bool value)");
	ssrCheckBox->SetPos(XMFLOAT2(x, y += step));
	ssrCheckBox->SetCheck(component->getSSREnabled());
	ssrCheckBox->OnClick([&](wiEventArgs args) {
		component->setSSREnabled(args.bValue);
	});
	ppWindow->AddWidget(ssrCheckBox);

	sssCheckBox = new wiCheckBox("SSS: ");
	sssCheckBox->SetTooltip("Enable Subsurface Scattering. (Deferred only for now)");
	sssCheckBox->SetScriptTip("RenderPath3D::SetSSSEnabled(bool value)");
	sssCheckBox->SetPos(XMFLOAT2(x, y += step));
	sssCheckBox->SetCheck(component->getSSSEnabled());
	sssCheckBox->OnClick([&](wiEventArgs args) {
		component->setSSSEnabled(args.bValue);
	});
	ppWindow->AddWidget(sssCheckBox);

	eyeAdaptionCheckBox = new wiCheckBox("EyeAdaption: ");
	eyeAdaptionCheckBox->SetTooltip("Enable eye adaption for the overall screen luminance");
	eyeAdaptionCheckBox->SetPos(XMFLOAT2(x, y += step));
	eyeAdaptionCheckBox->SetCheck(component->getEyeAdaptionEnabled());
	eyeAdaptionCheckBox->OnClick([&](wiEventArgs args) {
		component->setEyeAdaptionEnabled(args.bValue);
	});
	ppWindow->AddWidget(eyeAdaptionCheckBox);

	motionBlurCheckBox = new wiCheckBox("MotionBlur: ");
	motionBlurCheckBox->SetTooltip("Enable motion blur for camera movement and animated meshes.");
	motionBlurCheckBox->SetScriptTip("RenderPath3D::SetMotionBlurEnabled(bool value)");
	motionBlurCheckBox->SetPos(XMFLOAT2(x, y += step));
	motionBlurCheckBox->SetCheck(component->getMotionBlurEnabled());
	motionBlurCheckBox->OnClick([&](wiEventArgs args) {
		component->setMotionBlurEnabled(args.bValue);
	});
	ppWindow->AddWidget(motionBlurCheckBox);

	motionBlurStrengthSlider = new wiSlider(0.1f, 400, 100, 10000, "Strength: ");
	motionBlurStrengthSlider->SetTooltip("Set the camera shutter speed for motion blur (higher value means stronger blur).");
	motionBlurStrengthSlider->SetScriptTip("RenderPath3D::SetMotionBlurStrength(float value)");
	motionBlurStrengthSlider->SetSize(XMFLOAT2(100, 20));
	motionBlurStrengthSlider->SetPos(XMFLOAT2(x + 100, y));
	motionBlurStrengthSlider->SetValue(component->getMotionBlurStrength());
	motionBlurStrengthSlider->OnSlide([&](wiEventArgs args) {
		component->setMotionBlurStrength(args.fValue);
		});
	ppWindow->AddWidget(motionBlurStrengthSlider);

	depthOfFieldCheckBox = new wiCheckBox("DepthOfField: ");
	depthOfFieldCheckBox->SetTooltip("Enable Depth of field effect. Additional focus and strength setup required.");
	depthOfFieldCheckBox->SetScriptTip("RenderPath3D::SetDepthOfFieldEnabled(bool value)");
	depthOfFieldCheckBox->SetPos(XMFLOAT2(x, y += step));
	depthOfFieldCheckBox->SetCheck(component->getDepthOfFieldEnabled());
	depthOfFieldCheckBox->OnClick([&](wiEventArgs args) {
		component->setDepthOfFieldEnabled(args.bValue);
	});
	ppWindow->AddWidget(depthOfFieldCheckBox);

	depthOfFieldFocusSlider = new wiSlider(0.1f, 100, 10, 10000, "Focus: ");
	depthOfFieldFocusSlider->SetTooltip("Set the focus distance from the camera. The picture will be sharper near the focus, and blurrier further from it.");
	depthOfFieldFocusSlider->SetScriptTip("RenderPath3D::SetDepthOfFieldFocus(float value)");
	depthOfFieldFocusSlider->SetSize(XMFLOAT2(100, 20));
	depthOfFieldFocusSlider->SetPos(XMFLOAT2(x + 100, y));
	depthOfFieldFocusSlider->SetValue(component->getDepthOfFieldFocus());
	depthOfFieldFocusSlider->OnSlide([&](wiEventArgs args) {
		component->setDepthOfFieldFocus(args.fValue);
	});
	ppWindow->AddWidget(depthOfFieldFocusSlider);

	depthOfFieldScaleSlider = new wiSlider(0.01f, 4, 100, 1000, "Scale: ");
	depthOfFieldScaleSlider->SetTooltip("Set depth of field scale/falloff.");
	depthOfFieldScaleSlider->SetScriptTip("RenderPath3D::SetDepthOfFieldStrength(float value)");
	depthOfFieldScaleSlider->SetSize(XMFLOAT2(100, 20));
	depthOfFieldScaleSlider->SetPos(XMFLOAT2(x + 100, y += step));
	depthOfFieldScaleSlider->SetValue(component->getDepthOfFieldStrength());
	depthOfFieldScaleSlider->OnSlide([&](wiEventArgs args) {
		component->setDepthOfFieldStrength(args.fValue);
	});
	ppWindow->AddWidget(depthOfFieldScaleSlider);

	depthOfFieldAspectSlider = new wiSlider(0.01f, 2, 1, 1000, "Aspect: ");
	depthOfFieldAspectSlider->SetTooltip("Set depth of field bokeh aspect ratio (width/height).");
	depthOfFieldAspectSlider->SetScriptTip("RenderPath3D::SetDepthOfFieldAspect(float value)");
	depthOfFieldAspectSlider->SetSize(XMFLOAT2(100, 20));
	depthOfFieldAspectSlider->SetPos(XMFLOAT2(x + 100, y += step));
	depthOfFieldAspectSlider->SetValue(component->getDepthOfFieldAspect());
	depthOfFieldAspectSlider->OnSlide([&](wiEventArgs args) {
		component->setDepthOfFieldAspect(args.fValue);
		});
	ppWindow->AddWidget(depthOfFieldAspectSlider);

	bloomCheckBox = new wiCheckBox("Bloom: ");
	bloomCheckBox->SetTooltip("Enable bloom. The effect adds color bleeding to the brightest parts of the scene.");
	bloomCheckBox->SetScriptTip("RenderPath3D::SetBloomEnabled(bool value)");
	bloomCheckBox->SetPos(XMFLOAT2(x, y += step));
	bloomCheckBox->SetCheck(component->getBloomEnabled());
	bloomCheckBox->OnClick([&](wiEventArgs args) {
		component->setBloomEnabled(args.bValue);
	});
	ppWindow->AddWidget(bloomCheckBox);

	bloomStrengthSlider = new wiSlider(0.0f, 10, 1, 1000, "Threshold: ");
	bloomStrengthSlider->SetTooltip("Set bloom threshold. The values below this will not glow on the screen.");
	bloomStrengthSlider->SetSize(XMFLOAT2(100, 20));
	bloomStrengthSlider->SetPos(XMFLOAT2(x + 100, y));
	bloomStrengthSlider->SetValue(component->getBloomThreshold());
	bloomStrengthSlider->OnSlide([&](wiEventArgs args) {
		component->setBloomThreshold(args.fValue);
	});
	ppWindow->AddWidget(bloomStrengthSlider);

	fxaaCheckBox = new wiCheckBox("FXAA: ");
	fxaaCheckBox->SetTooltip("Fast Approximate Anti Aliasing. A fast antialiasing method, but can be a bit too blurry.");
	fxaaCheckBox->SetScriptTip("RenderPath3D::SetFXAAEnabled(bool value)");
	fxaaCheckBox->SetPos(XMFLOAT2(x, y += step));
	fxaaCheckBox->SetCheck(component->getFXAAEnabled());
	fxaaCheckBox->OnClick([&](wiEventArgs args) {
		component->setFXAAEnabled(args.bValue);
	});
	ppWindow->AddWidget(fxaaCheckBox);

	colorGradingCheckBox = new wiCheckBox("Color Grading: ");
	colorGradingCheckBox->SetTooltip("Enable color grading of the final render. An additional lookup texture must be set for it to take effect.");
	colorGradingCheckBox->SetScriptTip("RenderPath3D::SetColorGradingEnabled(bool value)");
	colorGradingCheckBox->SetPos(XMFLOAT2(x, y += step));
	colorGradingCheckBox->SetCheck(component->getColorGradingEnabled());
	colorGradingCheckBox->OnClick([&](wiEventArgs args) {
		component->setColorGradingEnabled(args.bValue);
	});
	ppWindow->AddWidget(colorGradingCheckBox);

	colorGradingButton = new wiButton("Load Color Grading LUT...");
	colorGradingButton->SetTooltip("Load a color grading lookup texture...");
	colorGradingButton->SetPos(XMFLOAT2(x + 35, y));
	colorGradingButton->SetSize(XMFLOAT2(200, 18));
	colorGradingButton->OnClick([=](wiEventArgs args) {
		auto x = component->getColorGradingTexture();

		if (x == nullptr)
		{
			thread([&] {
				wiHelper::FileDialogParams params;
				wiHelper::FileDialogResult result;
				params.type = wiHelper::FileDialogParams::OPEN;
				params.description = "Texture";
				params.extensions.push_back("dds");
				params.extensions.push_back("png");
				params.extensions.push_back("jpg");
				params.extensions.push_back("tga");
				wiHelper::FileDialog(params, result);

				if (result.ok) {
					string fileName = result.filenames.front();
					component->setColorGradingTexture(wiResourceManager::Load(fileName));
					if (component->getColorGradingTexture() != nullptr)
					{
						colorGradingButton->SetText(fileName);
					}
				}
			}).detach();
		}
		else
		{
			component->setColorGradingTexture(nullptr);
			colorGradingButton->SetText("Load Color Grading LUT...");
		}

	});
	ppWindow->AddWidget(colorGradingButton);

	outlineCheckBox = new wiCheckBox("Dithering: ");
	outlineCheckBox->SetTooltip("Toggle the full screen dithering effect. This helps to reduce color banding.");
	outlineCheckBox->SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox->SetCheck(component->getDitherEnabled());
	outlineCheckBox->OnClick([&](wiEventArgs args) {
		component->setDitherEnabled(args.bValue);
		});
	ppWindow->AddWidget(outlineCheckBox);

	sharpenFilterCheckBox = new wiCheckBox("Sharpen Filter: ");
	sharpenFilterCheckBox->SetTooltip("Toggle sharpening post process of the final image.");
	sharpenFilterCheckBox->SetScriptTip("RenderPath3D::SetSharpenFilterEnabled(bool value)");
	sharpenFilterCheckBox->SetPos(XMFLOAT2(x, y += step));
	sharpenFilterCheckBox->SetCheck(component->getSharpenFilterEnabled());
	sharpenFilterCheckBox->OnClick([&](wiEventArgs args) {
		component->setSharpenFilterEnabled(args.bValue);
	});
	ppWindow->AddWidget(sharpenFilterCheckBox);

	sharpenFilterAmountSlider = new wiSlider(0, 4, 1, 1000, "Amount: ");
	sharpenFilterAmountSlider->SetTooltip("Set sharpness filter strength.");
	sharpenFilterAmountSlider->SetScriptTip("RenderPath3D::SetSharpenFilterAmount(float value)");
	sharpenFilterAmountSlider->SetSize(XMFLOAT2(100, 20));
	sharpenFilterAmountSlider->SetPos(XMFLOAT2(x + 100, y));
	sharpenFilterAmountSlider->SetValue(component->getSharpenFilterAmount());
	sharpenFilterAmountSlider->OnSlide([&](wiEventArgs args) {
		component->setSharpenFilterAmount(args.fValue);
	});
	ppWindow->AddWidget(sharpenFilterAmountSlider);

	outlineCheckBox = new wiCheckBox("Cartoon Outline: ");
	outlineCheckBox->SetTooltip("Toggle the full screen cartoon outline effect.");
	outlineCheckBox->SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox->SetCheck(component->getOutlineEnabled());
	outlineCheckBox->OnClick([&](wiEventArgs args) {
		component->setOutlineEnabled(args.bValue);
	});
	ppWindow->AddWidget(outlineCheckBox);

	outlineThresholdSlider = new wiSlider(0, 1, 0.1f, 1000, "Threshold: ");
	outlineThresholdSlider->SetTooltip("Outline edge detection threshold. Increase if not enough otlines are detected, decrease if too many outlines are detected.");
	outlineThresholdSlider->SetSize(XMFLOAT2(100, 20));
	outlineThresholdSlider->SetPos(XMFLOAT2(x + 100, y));
	outlineThresholdSlider->SetValue(component->getOutlineThreshold());
	outlineThresholdSlider->OnSlide([&](wiEventArgs args) {
		component->setOutlineThreshold(args.fValue);
	});
	ppWindow->AddWidget(outlineThresholdSlider);

	outlineThicknessSlider = new wiSlider(0, 4, 1, 1000, "Thickness: ");
	outlineThicknessSlider->SetTooltip("Set outline thickness.");
	outlineThicknessSlider->SetSize(XMFLOAT2(100, 20));
	outlineThicknessSlider->SetPos(XMFLOAT2(x + 100, y += step));
	outlineThicknessSlider->SetValue(component->getOutlineThickness());
	outlineThicknessSlider->OnSlide([&](wiEventArgs args) {
		component->setOutlineThickness(args.fValue);
	});
	ppWindow->AddWidget(outlineThicknessSlider);

	chromaticaberrationCheckBox = new wiCheckBox("Chromatic Aberration: ");
	chromaticaberrationCheckBox->SetTooltip("Toggle the full screen chromatic aberration effect. This simulates lens distortion at screen edges.");
	chromaticaberrationCheckBox->SetPos(XMFLOAT2(x, y += step));
	chromaticaberrationCheckBox->SetCheck(component->getOutlineEnabled());
	chromaticaberrationCheckBox->OnClick([&](wiEventArgs args) {
		component->setChromaticAberrationEnabled(args.bValue);
		});
	ppWindow->AddWidget(chromaticaberrationCheckBox);

	chromaticaberrationSlider = new wiSlider(0, 4, 1.0f, 1000, "Amount: ");
	chromaticaberrationSlider->SetTooltip("The lens distortion amount.");
	chromaticaberrationSlider->SetSize(XMFLOAT2(100, 20));
	chromaticaberrationSlider->SetPos(XMFLOAT2(x + 100, y));
	chromaticaberrationSlider->SetValue(component->getChromaticAberrationAmount());
	chromaticaberrationSlider->OnSlide([&](wiEventArgs args) {
		component->setChromaticAberrationAmount(args.fValue);
		});
	ppWindow->AddWidget(chromaticaberrationSlider);


	ppWindow->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 550, 50, 0));
	ppWindow->SetVisible(false);

}


PostprocessWindow::~PostprocessWindow()
{
	ppWindow->RemoveWidgets(true);
	GUI->RemoveWidget(ppWindow);
	delete ppWindow;
}
