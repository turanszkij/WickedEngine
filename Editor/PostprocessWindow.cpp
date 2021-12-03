#include "stdafx.h"
#include "PostprocessWindow.h"
#include "Editor.h"

#include <thread>

using namespace wi::graphics;

void PostprocessWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("PostProcess Window");
	SetSize(XMFLOAT2(420, 500));

	float x = 150;
	float y = 10;
	float hei = 18;
	float step = hei + 2;

	exposureSlider.Create(0.0f, 3.0f, 1, 10000, "Exposure: ");
	exposureSlider.SetTooltip("Set the tonemap exposure value");
	exposureSlider.SetScriptTip("RenderPath3D::SetExposure(float value)");
	exposureSlider.SetSize(XMFLOAT2(100, hei));
	exposureSlider.SetPos(XMFLOAT2(x, y += step));
	exposureSlider.SetValue(editor->renderPath->getExposure());
	exposureSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setExposure(args.fValue);
	});
	AddWidget(&exposureSlider);

	lensFlareCheckBox.Create("LensFlare: ");
	lensFlareCheckBox.SetTooltip("Toggle visibility of light source flares. Additional setup needed per light for a lensflare to be visible.");
	lensFlareCheckBox.SetScriptTip("RenderPath3D::SetLensFlareEnabled(bool value)");
	lensFlareCheckBox.SetSize(XMFLOAT2(hei, hei));
	lensFlareCheckBox.SetPos(XMFLOAT2(x, y += step));
	lensFlareCheckBox.SetCheck(editor->renderPath->getLensFlareEnabled());
	lensFlareCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setLensFlareEnabled(args.bValue);
	});
	AddWidget(&lensFlareCheckBox);

	lightShaftsCheckBox.Create("LightShafts: ");
	lightShaftsCheckBox.SetTooltip("Enable light shaft for directional light sources.");
	lightShaftsCheckBox.SetScriptTip("RenderPath3D::SetLightShaftsEnabled(bool value)");
	lightShaftsCheckBox.SetSize(XMFLOAT2(hei, hei));
	lightShaftsCheckBox.SetPos(XMFLOAT2(x, y += step));
	lightShaftsCheckBox.SetCheck(editor->renderPath->getLightShaftsEnabled());
	lightShaftsCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setLightShaftsEnabled(args.bValue);
	});
	AddWidget(&lightShaftsCheckBox);

	aoComboBox.Create("AO: ");
	aoComboBox.SetTooltip("Choose Ambient Occlusion type. RTAO is only available if hardware supports ray tracing");
	aoComboBox.SetScriptTip("RenderPath3D::SetAO(int value)");
	aoComboBox.SetSize(XMFLOAT2(150, hei));
	aoComboBox.SetPos(XMFLOAT2(x, y += step));
	aoComboBox.AddItem("Disabled");
	aoComboBox.AddItem("SSAO");
	aoComboBox.AddItem("HBAO");
	aoComboBox.AddItem("MSAO");
	if (wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
	{
		aoComboBox.AddItem("RTAO");
	}
	aoComboBox.SetSelected(editor->renderPath->getAO());
	aoComboBox.OnSelect([=](wi::gui::EventArgs args) {
		editor->renderPath->setAO((wi::RenderPath3D::AO)args.iValue);

		switch (editor->renderPath->getAO())
		{
		case wi::RenderPath3D::AO_SSAO:
			aoRangeSlider.SetEnabled(true); 
			aoRangeSlider.SetValue(2.0f);
			aoSampleCountSlider.SetEnabled(true); 
			aoSampleCountSlider.SetValue(9.0f);
			break;
		case wi::RenderPath3D::AO_RTAO:
			aoRangeSlider.SetEnabled(true); 
			aoRangeSlider.SetValue(10.0f);
			aoSampleCountSlider.SetEnabled(false);
			break;
		default:
			aoRangeSlider.SetEnabled(false);
			aoSampleCountSlider.SetEnabled(false);
			break;
		}

		editor->renderPath->setAORange(aoRangeSlider.GetValue());
		editor->renderPath->setAOSampleCount((uint32_t)aoSampleCountSlider.GetValue());
	});
	AddWidget(&aoComboBox);

	aoPowerSlider.Create(0.25f, 8.0f, 2, 1000, "Power: ");
	aoPowerSlider.SetTooltip("Set SSAO Power. Higher values produce darker, more pronounced effect");
	aoPowerSlider.SetSize(XMFLOAT2(100, hei));
	aoPowerSlider.SetPos(XMFLOAT2(x + 100, y += step));
	aoPowerSlider.SetValue((float)editor->renderPath->getAOPower());
	aoPowerSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setAOPower(args.fValue);
		});
	AddWidget(&aoPowerSlider);

	aoRangeSlider.Create(1.0f, 100.0f, 1, 1000, "Range: ");
	aoRangeSlider.SetTooltip("Set AO ray length. Only for SSAO and RTAO");
	aoRangeSlider.SetSize(XMFLOAT2(100, hei));
	aoRangeSlider.SetPos(XMFLOAT2(x + 100, y += step));
	aoRangeSlider.SetValue((float)editor->renderPath->getAOPower());
	aoRangeSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setAORange(args.fValue);
		});
	AddWidget(&aoRangeSlider);

	aoSampleCountSlider.Create(1, 16, 9, 15, "Sample Count: ");
	aoSampleCountSlider.SetTooltip("Set AO ray count. Only for SSAO");
	aoSampleCountSlider.SetSize(XMFLOAT2(100, hei));
	aoSampleCountSlider.SetPos(XMFLOAT2(x + 100, y += step));
	aoSampleCountSlider.SetValue((float)editor->renderPath->getAOPower());
	aoSampleCountSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setAOSampleCount(args.iValue);
		});
	AddWidget(&aoSampleCountSlider);

	ssrCheckBox.Create("SSR: ");
	ssrCheckBox.SetTooltip("Enable Screen Space Reflections.");
	ssrCheckBox.SetScriptTip("RenderPath3D::SetSSREnabled(bool value)");
	ssrCheckBox.SetSize(XMFLOAT2(hei, hei));
	ssrCheckBox.SetPos(XMFLOAT2(x, y += step));
	ssrCheckBox.SetCheck(editor->renderPath->getSSREnabled());
	ssrCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setSSREnabled(args.bValue);
	});
	AddWidget(&ssrCheckBox);

	raytracedReflectionsCheckBox.Create("Ray Traced Reflections: ");
	raytracedReflectionsCheckBox.SetTooltip("Enable Ray Traced Reflections. Only if GPU supports raytracing.");
	raytracedReflectionsCheckBox.SetScriptTip("RenderPath3D::SetRaytracedReflectionsEnabled(bool value)");
	raytracedReflectionsCheckBox.SetSize(XMFLOAT2(hei, hei));
	raytracedReflectionsCheckBox.SetPos(XMFLOAT2(x + 200, y));
	raytracedReflectionsCheckBox.SetCheck(editor->renderPath->getRaytracedReflectionEnabled());
	raytracedReflectionsCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setRaytracedReflectionsEnabled(args.bValue);
		});
	AddWidget(&raytracedReflectionsCheckBox);
	raytracedReflectionsCheckBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::RAYTRACING));

	screenSpaceShadowsCheckBox.Create("SS Shadows: ");
	screenSpaceShadowsCheckBox.SetTooltip("Enable screen space contact shadows. This can add small shadows details to shadow maps in screen space.");
	screenSpaceShadowsCheckBox.SetSize(XMFLOAT2(hei, hei));
	screenSpaceShadowsCheckBox.SetPos(XMFLOAT2(x, y += step));
	screenSpaceShadowsCheckBox.SetCheck(wi::renderer::GetScreenSpaceShadowsEnabled());
	screenSpaceShadowsCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::renderer::SetScreenSpaceShadowsEnabled(args.bValue);
		});
	AddWidget(&screenSpaceShadowsCheckBox);

	screenSpaceShadowsRangeSlider.Create(0.1f, 10.0f, 1, 1000, "Range: ");
	screenSpaceShadowsRangeSlider.SetTooltip("Range of contact shadows");
	screenSpaceShadowsRangeSlider.SetSize(XMFLOAT2(100, hei));
	screenSpaceShadowsRangeSlider.SetPos(XMFLOAT2(x + 100, y));
	screenSpaceShadowsRangeSlider.SetValue((float)editor->renderPath->getScreenSpaceShadowRange());
	screenSpaceShadowsRangeSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setScreenSpaceShadowRange(args.fValue);
		});
	AddWidget(&screenSpaceShadowsRangeSlider);

	screenSpaceShadowsStepCountSlider.Create(4, 128, 16, 128 - 4, "Sample Count: ");
	screenSpaceShadowsStepCountSlider.SetTooltip("Sample count of contact shadows. Higher values are better quality but slower.");
	screenSpaceShadowsStepCountSlider.SetSize(XMFLOAT2(100, hei));
	screenSpaceShadowsStepCountSlider.SetPos(XMFLOAT2(x + 100, y += step));
	screenSpaceShadowsStepCountSlider.SetValue((float)editor->renderPath->getScreenSpaceShadowSampleCount());
	screenSpaceShadowsStepCountSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setScreenSpaceShadowSampleCount(args.iValue);
		});
	AddWidget(&screenSpaceShadowsStepCountSlider);

	eyeAdaptionCheckBox.Create("EyeAdaption: ");
	eyeAdaptionCheckBox.SetTooltip("Enable eye adaption for the overall screen luminance");
	eyeAdaptionCheckBox.SetSize(XMFLOAT2(hei, hei));
	eyeAdaptionCheckBox.SetPos(XMFLOAT2(x, y += step));
	eyeAdaptionCheckBox.SetCheck(editor->renderPath->getEyeAdaptionEnabled());
	eyeAdaptionCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setEyeAdaptionEnabled(args.bValue);
	});
	AddWidget(&eyeAdaptionCheckBox);

	eyeAdaptionKeySlider.Create(0.01f, 0.5f, 0.1f, 10000, "Key: ");
	eyeAdaptionKeySlider.SetTooltip("Set the key value for eye adaption.");
	eyeAdaptionKeySlider.SetSize(XMFLOAT2(100, hei));
	eyeAdaptionKeySlider.SetPos(XMFLOAT2(x + 100, y));
	eyeAdaptionKeySlider.SetValue(editor->renderPath->getEyeAdaptionKey());
	eyeAdaptionKeySlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setEyeAdaptionKey(args.fValue);
		});
	AddWidget(&eyeAdaptionKeySlider);

	eyeAdaptionRateSlider.Create(0.01f, 4, 0.5f, 10000, "Rate: ");
	eyeAdaptionRateSlider.SetTooltip("Set the eye adaption rate (speed of adjustment)");
	eyeAdaptionRateSlider.SetSize(XMFLOAT2(100, hei));
	eyeAdaptionRateSlider.SetPos(XMFLOAT2(x + 100, y += step));
	eyeAdaptionRateSlider.SetValue(editor->renderPath->getEyeAdaptionRate());
	eyeAdaptionRateSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setEyeAdaptionRate(args.fValue);
		});
	AddWidget(&eyeAdaptionRateSlider);

	motionBlurCheckBox.Create("MotionBlur: ");
	motionBlurCheckBox.SetTooltip("Enable motion blur for camera movement and animated meshes.");
	motionBlurCheckBox.SetScriptTip("RenderPath3D::SetMotionBlurEnabled(bool value)");
	motionBlurCheckBox.SetSize(XMFLOAT2(hei, hei));
	motionBlurCheckBox.SetPos(XMFLOAT2(x, y += step));
	motionBlurCheckBox.SetCheck(editor->renderPath->getMotionBlurEnabled());
	motionBlurCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setMotionBlurEnabled(args.bValue);
	});
	AddWidget(&motionBlurCheckBox);

	motionBlurStrengthSlider.Create(0.1f, 400, 100, 10000, "Strength: ");
	motionBlurStrengthSlider.SetTooltip("Set the camera shutter speed for motion blur (higher value means stronger blur).");
	motionBlurStrengthSlider.SetScriptTip("RenderPath3D::SetMotionBlurStrength(float value)");
	motionBlurStrengthSlider.SetSize(XMFLOAT2(100, hei));
	motionBlurStrengthSlider.SetPos(XMFLOAT2(x + 100, y));
	motionBlurStrengthSlider.SetValue(editor->renderPath->getMotionBlurStrength());
	motionBlurStrengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setMotionBlurStrength(args.fValue);
		});
	AddWidget(&motionBlurStrengthSlider);

	depthOfFieldCheckBox.Create("DepthOfField: ");
	depthOfFieldCheckBox.SetTooltip("Enable Depth of field effect. Requires additional camera setup: focal length and aperture size.");
	depthOfFieldCheckBox.SetScriptTip("RenderPath3D::SetDepthOfFieldEnabled(bool value)");
	depthOfFieldCheckBox.SetSize(XMFLOAT2(hei, hei));
	depthOfFieldCheckBox.SetPos(XMFLOAT2(x, y += step));
	depthOfFieldCheckBox.SetCheck(editor->renderPath->getDepthOfFieldEnabled());
	depthOfFieldCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setDepthOfFieldEnabled(args.bValue);
	});
	AddWidget(&depthOfFieldCheckBox);

	depthOfFieldScaleSlider.Create(1.0f, 20, 100, 1000, "Strength: ");
	depthOfFieldScaleSlider.SetTooltip("Set depth of field strength. This is used to scale the Camera's ApertureSize setting");
	depthOfFieldScaleSlider.SetScriptTip("RenderPath3D::SetDepthOfFieldStrength(float value)");
	depthOfFieldScaleSlider.SetSize(XMFLOAT2(100, hei));
	depthOfFieldScaleSlider.SetPos(XMFLOAT2(x + 100, y));
	depthOfFieldScaleSlider.SetValue(editor->renderPath->getDepthOfFieldStrength());
	depthOfFieldScaleSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setDepthOfFieldStrength(args.fValue);
	});
	AddWidget(&depthOfFieldScaleSlider);

	bloomCheckBox.Create("Bloom: ");
	bloomCheckBox.SetTooltip("Enable bloom. The effect adds color bleeding to the brightest parts of the scene.");
	bloomCheckBox.SetScriptTip("RenderPath3D::SetBloomEnabled(bool value)");
	bloomCheckBox.SetSize(XMFLOAT2(hei, hei));
	bloomCheckBox.SetPos(XMFLOAT2(x, y += step));
	bloomCheckBox.SetCheck(editor->renderPath->getBloomEnabled());
	bloomCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setBloomEnabled(args.bValue);
	});
	AddWidget(&bloomCheckBox);

	bloomStrengthSlider.Create(0.0f, 10, 1, 1000, "Threshold: ");
	bloomStrengthSlider.SetTooltip("Set bloom threshold. The values below this will not glow on the screen.");
	bloomStrengthSlider.SetSize(XMFLOAT2(100, hei));
	bloomStrengthSlider.SetPos(XMFLOAT2(x + 100, y));
	bloomStrengthSlider.SetValue(editor->renderPath->getBloomThreshold());
	bloomStrengthSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setBloomThreshold(args.fValue);
	});
	AddWidget(&bloomStrengthSlider);

	fxaaCheckBox.Create("FXAA: ");
	fxaaCheckBox.SetTooltip("Fast Approximate Anti Aliasing. A fast antialiasing method, but can be a bit too blurry.");
	fxaaCheckBox.SetScriptTip("RenderPath3D::SetFXAAEnabled(bool value)");
	fxaaCheckBox.SetSize(XMFLOAT2(hei, hei));
	fxaaCheckBox.SetPos(XMFLOAT2(x, y += step));
	fxaaCheckBox.SetCheck(editor->renderPath->getFXAAEnabled());
	fxaaCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setFXAAEnabled(args.bValue);
	});
	AddWidget(&fxaaCheckBox);

	colorGradingCheckBox.Create("Color Grading: ");
	colorGradingCheckBox.SetTooltip("Enable color grading of the final render. An additional lookup texture must be set in the Weather!");
	colorGradingCheckBox.SetSize(XMFLOAT2(hei, hei));
	colorGradingCheckBox.SetPos(XMFLOAT2(x, y += step));
	colorGradingCheckBox.SetCheck(editor->renderPath->getColorGradingEnabled());
	colorGradingCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setColorGradingEnabled(args.bValue);
	});
	AddWidget(&colorGradingCheckBox);

	ditherCheckBox.Create("Dithering: ");
	ditherCheckBox.SetTooltip("Toggle the full screen dithering effect. This helps to reduce color banding.");
	ditherCheckBox.SetSize(XMFLOAT2(hei, hei));
	ditherCheckBox.SetPos(XMFLOAT2(x, y += step));
	ditherCheckBox.SetCheck(editor->renderPath->getDitherEnabled());
	ditherCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setDitherEnabled(args.bValue);
		});
	AddWidget(&ditherCheckBox);

	sharpenFilterCheckBox.Create("Sharpen Filter: ");
	sharpenFilterCheckBox.SetTooltip("Toggle sharpening post process of the final image.");
	sharpenFilterCheckBox.SetScriptTip("RenderPath3D::SetSharpenFilterEnabled(bool value)");
	sharpenFilterCheckBox.SetSize(XMFLOAT2(hei, hei));
	sharpenFilterCheckBox.SetPos(XMFLOAT2(x, y += step));
	sharpenFilterCheckBox.SetCheck(editor->renderPath->getSharpenFilterEnabled());
	sharpenFilterCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setSharpenFilterEnabled(args.bValue);
	});
	AddWidget(&sharpenFilterCheckBox);

	sharpenFilterAmountSlider.Create(0, 4, 1, 1000, "Amount: ");
	sharpenFilterAmountSlider.SetTooltip("Set sharpness filter strength.");
	sharpenFilterAmountSlider.SetScriptTip("RenderPath3D::SetSharpenFilterAmount(float value)");
	sharpenFilterAmountSlider.SetSize(XMFLOAT2(100, hei));
	sharpenFilterAmountSlider.SetPos(XMFLOAT2(x + 100, y));
	sharpenFilterAmountSlider.SetValue(editor->renderPath->getSharpenFilterAmount());
	sharpenFilterAmountSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setSharpenFilterAmount(args.fValue);
	});
	AddWidget(&sharpenFilterAmountSlider);

	outlineCheckBox.Create("Cartoon Outline: ");
	outlineCheckBox.SetTooltip("Toggle the full screen cartoon outline effect.");
	outlineCheckBox.SetSize(XMFLOAT2(hei, hei));
	outlineCheckBox.SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox.SetCheck(editor->renderPath->getOutlineEnabled());
	outlineCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setOutlineEnabled(args.bValue);
	});
	AddWidget(&outlineCheckBox);

	outlineThresholdSlider.Create(0, 1, 0.1f, 1000, "Threshold: ");
	outlineThresholdSlider.SetTooltip("Outline edge detection threshold. Increase if not enough otlines are detected, decrease if too many outlines are detected.");
	outlineThresholdSlider.SetSize(XMFLOAT2(100, hei));
	outlineThresholdSlider.SetPos(XMFLOAT2(x + 100, y));
	outlineThresholdSlider.SetValue(editor->renderPath->getOutlineThreshold());
	outlineThresholdSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setOutlineThreshold(args.fValue);
	});
	AddWidget(&outlineThresholdSlider);

	outlineThicknessSlider.Create(0, 4, 1, 1000, "Thickness: ");
	outlineThicknessSlider.SetTooltip("Set outline thickness.");
	outlineThicknessSlider.SetSize(XMFLOAT2(100, hei));
	outlineThicknessSlider.SetPos(XMFLOAT2(x + 100, y += step));
	outlineThicknessSlider.SetValue(editor->renderPath->getOutlineThickness());
	outlineThicknessSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setOutlineThickness(args.fValue);
	});
	AddWidget(&outlineThicknessSlider);

	chromaticaberrationCheckBox.Create("Chromatic Aberration: ");
	chromaticaberrationCheckBox.SetTooltip("Toggle the full screen chromatic aberration effect. This simulates lens distortion at screen edges.");
	chromaticaberrationCheckBox.SetSize(XMFLOAT2(hei, hei));
	chromaticaberrationCheckBox.SetPos(XMFLOAT2(x, y += step));
	chromaticaberrationCheckBox.SetCheck(editor->renderPath->getOutlineEnabled());
	chromaticaberrationCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setChromaticAberrationEnabled(args.bValue);
		});
	AddWidget(&chromaticaberrationCheckBox);

	chromaticaberrationSlider.Create(0, 4, 1.0f, 1000, "Amount: ");
	chromaticaberrationSlider.SetTooltip("The lens distortion amount.");
	chromaticaberrationSlider.SetSize(XMFLOAT2(100, hei));
	chromaticaberrationSlider.SetPos(XMFLOAT2(x + 100, y));
	chromaticaberrationSlider.SetValue(editor->renderPath->getChromaticAberrationAmount());
	chromaticaberrationSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setChromaticAberrationAmount(args.fValue);
		});
	AddWidget(&chromaticaberrationSlider);

	fsrCheckBox.Create("FSR: ");
	fsrCheckBox.SetTooltip("FidelityFX FSR Upscaling. Use this only with Temporal AA or MSAA when the resolution scaling is lowered.");
	fsrCheckBox.SetSize(XMFLOAT2(hei, hei));
	fsrCheckBox.SetPos(XMFLOAT2(x, y += step));
	fsrCheckBox.SetCheck(editor->renderPath->getFSREnabled());
	fsrCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->renderPath->setFSREnabled(args.bValue);
		});
	AddWidget(&fsrCheckBox);

	fsrSlider.Create(0, 2, 1.0f, 1000, "Sharpness: ");
	fsrSlider.SetTooltip("The sharpening amount to apply for FSR upscaling.");
	fsrSlider.SetSize(XMFLOAT2(100, hei));
	fsrSlider.SetPos(XMFLOAT2(x + 100, y));
	fsrSlider.SetValue(editor->renderPath->getFSRSharpness());
	fsrSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->renderPath->setFSRSharpness(args.fValue);
		});
	AddWidget(&fsrSlider);


	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 500, 80, 0));
	SetVisible(false);

}
