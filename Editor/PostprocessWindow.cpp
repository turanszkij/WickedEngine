#include "stdafx.h"
#include "PostprocessWindow.h"
#include "Editor.h"

#include <thread>

using namespace std;
using namespace wiGraphics;


void PostprocessWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("PostProcess Window");
	SetSize(XMFLOAT2(420, 520));

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
	exposureSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setExposure(args.fValue);
	});
	AddWidget(&exposureSlider);

	lensFlareCheckBox.Create("LensFlare: ");
	lensFlareCheckBox.SetTooltip("Toggle visibility of light source flares. Additional setup needed per light for a lensflare to be visible.");
	lensFlareCheckBox.SetScriptTip("RenderPath3D::SetLensFlareEnabled(bool value)");
	lensFlareCheckBox.SetSize(XMFLOAT2(hei, hei));
	lensFlareCheckBox.SetPos(XMFLOAT2(x, y += step));
	lensFlareCheckBox.SetCheck(editor->renderPath->getLensFlareEnabled());
	lensFlareCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setLensFlareEnabled(args.bValue);
	});
	AddWidget(&lensFlareCheckBox);

	lightShaftsCheckBox.Create("LightShafts: ");
	lightShaftsCheckBox.SetTooltip("Enable light shaft for directional light sources.");
	lightShaftsCheckBox.SetScriptTip("RenderPath3D::SetLightShaftsEnabled(bool value)");
	lightShaftsCheckBox.SetSize(XMFLOAT2(hei, hei));
	lightShaftsCheckBox.SetPos(XMFLOAT2(x, y += step));
	lightShaftsCheckBox.SetCheck(editor->renderPath->getLightShaftsEnabled());
	lightShaftsCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setLightShaftsEnabled(args.bValue);
	});
	AddWidget(&lightShaftsCheckBox);

	volumetricCloudsCheckBox.Create("Volumetric clouds: ");
	volumetricCloudsCheckBox.SetTooltip("Enable volumetric cloud rendering.");
	volumetricCloudsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricCloudsCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricCloudsCheckBox.SetCheck(editor->renderPath->getVolumetricCloudsEnabled());
	volumetricCloudsCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setVolumetricCloudsEnabled(args.bValue);
		});
	AddWidget(&volumetricCloudsCheckBox);

	aoComboBox.Create("AO: ");
	aoComboBox.SetTooltip("Choose Ambient Occlusion type. RTAO is only available if hardware supports ray tracing");
	aoComboBox.SetScriptTip("RenderPath3D::SetAO(int value)");
	aoComboBox.SetSize(XMFLOAT2(150, hei));
	aoComboBox.SetPos(XMFLOAT2(x, y += step));
	aoComboBox.AddItem("Disabled");
	aoComboBox.AddItem("SSAO");
	aoComboBox.AddItem("HBAO");
	aoComboBox.AddItem("MSAO");
	if (wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING))
	{
		aoComboBox.AddItem("RTAO");
	}
	aoComboBox.SetSelected(editor->renderPath->getAO());
	aoComboBox.OnSelect([=](wiEventArgs args) {
		editor->renderPath->setAO((RenderPath3D::AO)args.iValue);

		switch (editor->renderPath->getAO())
		{
		case RenderPath3D::AO_SSAO:
			aoRangeSlider.SetEnabled(true); 
			aoRangeSlider.SetValue(2.0f);
			aoSampleCountSlider.SetEnabled(true); 
			aoSampleCountSlider.SetValue(9.0f);
			break;
		case RenderPath3D::AO_RTAO:
			aoRangeSlider.SetEnabled(true); 
			aoRangeSlider.SetValue(10.0f);
			aoSampleCountSlider.SetEnabled(true); 
			aoSampleCountSlider.SetValue(2.0f);
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
	aoPowerSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setAOPower(args.fValue);
		});
	AddWidget(&aoPowerSlider);

	aoRangeSlider.Create(1.0f, 100.0f, 1, 1000, "Range: ");
	aoRangeSlider.SetTooltip("Set AO ray length. Only for SSAO and RTAO");
	aoRangeSlider.SetSize(XMFLOAT2(100, hei));
	aoRangeSlider.SetPos(XMFLOAT2(x + 100, y += step));
	aoRangeSlider.SetValue((float)editor->renderPath->getAOPower());
	aoRangeSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setAORange(args.fValue);
		});
	AddWidget(&aoRangeSlider);

	aoSampleCountSlider.Create(1, 16, 9, 15, "Sample Count: ");
	aoSampleCountSlider.SetTooltip("Set AO ray count. Only for SSAO and RTAO");
	aoSampleCountSlider.SetSize(XMFLOAT2(100, hei));
	aoSampleCountSlider.SetPos(XMFLOAT2(x + 100, y += step));
	aoSampleCountSlider.SetValue((float)editor->renderPath->getAOPower());
	aoSampleCountSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setAOSampleCount(args.iValue);
		});
	AddWidget(&aoSampleCountSlider);

	ssrCheckBox.Create("SSR: ");
	ssrCheckBox.SetTooltip("Enable Screen Space Reflections.");
	ssrCheckBox.SetScriptTip("RenderPath3D::SetSSREnabled(bool value)");
	ssrCheckBox.SetSize(XMFLOAT2(hei, hei));
	ssrCheckBox.SetPos(XMFLOAT2(x, y += step));
	ssrCheckBox.SetCheck(editor->renderPath->getSSREnabled());
	ssrCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setSSREnabled(args.bValue);
	});
	AddWidget(&ssrCheckBox);

	raytracedReflectionsCheckBox.Create("Ray Traced Reflections: ");
	raytracedReflectionsCheckBox.SetTooltip("Enable Ray Traced Reflections. Only if GPU supports raytracing.");
	raytracedReflectionsCheckBox.SetScriptTip("RenderPath3D::SetRaytracedReflectionsEnabled(bool value)");
	raytracedReflectionsCheckBox.SetSize(XMFLOAT2(hei, hei));
	raytracedReflectionsCheckBox.SetPos(XMFLOAT2(x + 200, y));
	raytracedReflectionsCheckBox.SetCheck(editor->renderPath->getRaytracedReflectionEnabled());
	raytracedReflectionsCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setRaytracedReflectionsEnabled(args.bValue);
		});
	AddWidget(&raytracedReflectionsCheckBox);
	raytracedReflectionsCheckBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_RAYTRACING));

	sssCheckBox.Create("SSS: ");
	sssCheckBox.SetTooltip("Enable Subsurface Scattering. Only for PBR shaders.");
	sssCheckBox.SetScriptTip("RenderPath3D::SetSSSEnabled(bool value)");
	sssCheckBox.SetSize(XMFLOAT2(hei, hei));
	sssCheckBox.SetPos(XMFLOAT2(x, y += step));
	sssCheckBox.SetCheck(editor->renderPath->getSSSEnabled());
	sssCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setSSSEnabled(args.bValue);
	});
	AddWidget(&sssCheckBox);

	sssSlider.Create(0.0f, 2.0f, 1, 1000, "Amount: ");
	sssSlider.SetTooltip("Set SSS amount for subsurface materials.");
	sssSlider.SetSize(XMFLOAT2(100, hei));
	sssSlider.SetPos(XMFLOAT2(x + 100, y));
	sssSlider.SetValue((float)editor->renderPath->getSSSBlurAmount());
	sssSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setSSSBlurAmount(args.fValue);
		});
	AddWidget(&sssSlider);

	eyeAdaptionCheckBox.Create("EyeAdaption: ");
	eyeAdaptionCheckBox.SetTooltip("Enable eye adaption for the overall screen luminance");
	eyeAdaptionCheckBox.SetSize(XMFLOAT2(hei, hei));
	eyeAdaptionCheckBox.SetPos(XMFLOAT2(x, y += step));
	eyeAdaptionCheckBox.SetCheck(editor->renderPath->getEyeAdaptionEnabled());
	eyeAdaptionCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setEyeAdaptionEnabled(args.bValue);
	});
	AddWidget(&eyeAdaptionCheckBox);

	motionBlurCheckBox.Create("MotionBlur: ");
	motionBlurCheckBox.SetTooltip("Enable motion blur for camera movement and animated meshes.");
	motionBlurCheckBox.SetScriptTip("RenderPath3D::SetMotionBlurEnabled(bool value)");
	motionBlurCheckBox.SetSize(XMFLOAT2(hei, hei));
	motionBlurCheckBox.SetPos(XMFLOAT2(x, y += step));
	motionBlurCheckBox.SetCheck(editor->renderPath->getMotionBlurEnabled());
	motionBlurCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setMotionBlurEnabled(args.bValue);
	});
	AddWidget(&motionBlurCheckBox);

	motionBlurStrengthSlider.Create(0.1f, 400, 100, 10000, "Strength: ");
	motionBlurStrengthSlider.SetTooltip("Set the camera shutter speed for motion blur (higher value means stronger blur).");
	motionBlurStrengthSlider.SetScriptTip("RenderPath3D::SetMotionBlurStrength(float value)");
	motionBlurStrengthSlider.SetSize(XMFLOAT2(100, hei));
	motionBlurStrengthSlider.SetPos(XMFLOAT2(x + 100, y));
	motionBlurStrengthSlider.SetValue(editor->renderPath->getMotionBlurStrength());
	motionBlurStrengthSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setMotionBlurStrength(args.fValue);
		});
	AddWidget(&motionBlurStrengthSlider);

	depthOfFieldCheckBox.Create("DepthOfField: ");
	depthOfFieldCheckBox.SetTooltip("Enable Depth of field effect. Additional focus and strength setup required.");
	depthOfFieldCheckBox.SetScriptTip("RenderPath3D::SetDepthOfFieldEnabled(bool value)");
	depthOfFieldCheckBox.SetSize(XMFLOAT2(hei, hei));
	depthOfFieldCheckBox.SetPos(XMFLOAT2(x, y += step));
	depthOfFieldCheckBox.SetCheck(editor->renderPath->getDepthOfFieldEnabled());
	depthOfFieldCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setDepthOfFieldEnabled(args.bValue);
	});
	AddWidget(&depthOfFieldCheckBox);

	depthOfFieldFocusSlider.Create(1.0f, 100, 10, 10000, "Focus: ");
	depthOfFieldFocusSlider.SetTooltip("Set the focus distance from the camera. The picture will be sharper near the focus, and blurrier further from it.");
	depthOfFieldFocusSlider.SetScriptTip("RenderPath3D::SetDepthOfFieldFocus(float value)");
	depthOfFieldFocusSlider.SetSize(XMFLOAT2(100, hei));
	depthOfFieldFocusSlider.SetPos(XMFLOAT2(x + 100, y));
	depthOfFieldFocusSlider.SetValue(editor->renderPath->getDepthOfFieldFocus());
	depthOfFieldFocusSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setDepthOfFieldFocus(args.fValue);
	});
	AddWidget(&depthOfFieldFocusSlider);

	depthOfFieldScaleSlider.Create(1.0f, 20, 100, 1000, "Scale: ");
	depthOfFieldScaleSlider.SetTooltip("Set depth of field scale/falloff.");
	depthOfFieldScaleSlider.SetScriptTip("RenderPath3D::SetDepthOfFieldStrength(float value)");
	depthOfFieldScaleSlider.SetSize(XMFLOAT2(100, hei));
	depthOfFieldScaleSlider.SetPos(XMFLOAT2(x + 100, y += step));
	depthOfFieldScaleSlider.SetValue(editor->renderPath->getDepthOfFieldStrength());
	depthOfFieldScaleSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setDepthOfFieldStrength(args.fValue);
	});
	AddWidget(&depthOfFieldScaleSlider);

	depthOfFieldAspectSlider.Create(0.01f, 2, 1, 1000, "Aspect: ");
	depthOfFieldAspectSlider.SetTooltip("Set depth of field bokeh aspect ratio (width/height).");
	depthOfFieldAspectSlider.SetScriptTip("RenderPath3D::SetDepthOfFieldAspect(float value)");
	depthOfFieldAspectSlider.SetSize(XMFLOAT2(100, hei));
	depthOfFieldAspectSlider.SetPos(XMFLOAT2(x + 100, y += step));
	depthOfFieldAspectSlider.SetValue(editor->renderPath->getDepthOfFieldAspect());
	depthOfFieldAspectSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setDepthOfFieldAspect(args.fValue);
		});
	AddWidget(&depthOfFieldAspectSlider);

	bloomCheckBox.Create("Bloom: ");
	bloomCheckBox.SetTooltip("Enable bloom. The effect adds color bleeding to the brightest parts of the scene.");
	bloomCheckBox.SetScriptTip("RenderPath3D::SetBloomEnabled(bool value)");
	bloomCheckBox.SetSize(XMFLOAT2(hei, hei));
	bloomCheckBox.SetPos(XMFLOAT2(x, y += step));
	bloomCheckBox.SetCheck(editor->renderPath->getBloomEnabled());
	bloomCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setBloomEnabled(args.bValue);
	});
	AddWidget(&bloomCheckBox);

	bloomStrengthSlider.Create(0.0f, 10, 1, 1000, "Threshold: ");
	bloomStrengthSlider.SetTooltip("Set bloom threshold. The values below this will not glow on the screen.");
	bloomStrengthSlider.SetSize(XMFLOAT2(100, hei));
	bloomStrengthSlider.SetPos(XMFLOAT2(x + 100, y));
	bloomStrengthSlider.SetValue(editor->renderPath->getBloomThreshold());
	bloomStrengthSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setBloomThreshold(args.fValue);
	});
	AddWidget(&bloomStrengthSlider);

	fxaaCheckBox.Create("FXAA: ");
	fxaaCheckBox.SetTooltip("Fast Approximate Anti Aliasing. A fast antialiasing method, but can be a bit too blurry.");
	fxaaCheckBox.SetScriptTip("RenderPath3D::SetFXAAEnabled(bool value)");
	fxaaCheckBox.SetSize(XMFLOAT2(hei, hei));
	fxaaCheckBox.SetPos(XMFLOAT2(x, y += step));
	fxaaCheckBox.SetCheck(editor->renderPath->getFXAAEnabled());
	fxaaCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setFXAAEnabled(args.bValue);
	});
	AddWidget(&fxaaCheckBox);

	colorGradingCheckBox.Create("Color Grading: ");
	colorGradingCheckBox.SetTooltip("Enable color grading of the final render. An additional lookup texture must be set for it to take effect.");
	colorGradingCheckBox.SetScriptTip("RenderPath3D::SetColorGradingEnabled(bool value)");
	colorGradingCheckBox.SetSize(XMFLOAT2(hei, hei));
	colorGradingCheckBox.SetPos(XMFLOAT2(x, y += step));
	colorGradingCheckBox.SetCheck(editor->renderPath->getColorGradingEnabled());
	colorGradingCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setColorGradingEnabled(args.bValue);
	});
	AddWidget(&colorGradingCheckBox);

	colorGradingButton.Create("Load Color Grading LUT...");
	colorGradingButton.SetTooltip("Load a color grading lookup texture...");
	colorGradingButton.SetPos(XMFLOAT2(x + 35, y));
	colorGradingButton.SetSize(XMFLOAT2(200, hei));
	colorGradingButton.OnClick([=](wiEventArgs args) {
		auto x = editor->renderPath->getColorGradingTexture();

		if (x == nullptr)
		{
			wiHelper::FileDialogParams params;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions.push_back("dds");
			params.extensions.push_back("png");
			params.extensions.push_back("jpg");
			params.extensions.push_back("tga");
			wiHelper::FileDialog(params, [=](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					editor->renderPath->setColorGradingTexture(wiResourceManager::Load(fileName));
					if (editor->renderPath->getColorGradingTexture() != nullptr)
					{
						colorGradingButton.SetText(fileName);
					}
				});
			});
		}
		else
		{
			editor->renderPath->setColorGradingTexture(nullptr);
			colorGradingButton.SetText("Load Color Grading LUT...");
		}

	});
	AddWidget(&colorGradingButton);

	ditherCheckBox.Create("Dithering: ");
	ditherCheckBox.SetTooltip("Toggle the full screen dithering effect. This helps to reduce color banding.");
	ditherCheckBox.SetSize(XMFLOAT2(hei, hei));
	ditherCheckBox.SetPos(XMFLOAT2(x, y += step));
	ditherCheckBox.SetCheck(editor->renderPath->getDitherEnabled());
	ditherCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setDitherEnabled(args.bValue);
		});
	AddWidget(&ditherCheckBox);

	sharpenFilterCheckBox.Create("Sharpen Filter: ");
	sharpenFilterCheckBox.SetTooltip("Toggle sharpening post process of the final image.");
	sharpenFilterCheckBox.SetScriptTip("RenderPath3D::SetSharpenFilterEnabled(bool value)");
	sharpenFilterCheckBox.SetSize(XMFLOAT2(hei, hei));
	sharpenFilterCheckBox.SetPos(XMFLOAT2(x, y += step));
	sharpenFilterCheckBox.SetCheck(editor->renderPath->getSharpenFilterEnabled());
	sharpenFilterCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setSharpenFilterEnabled(args.bValue);
	});
	AddWidget(&sharpenFilterCheckBox);

	sharpenFilterAmountSlider.Create(0, 4, 1, 1000, "Amount: ");
	sharpenFilterAmountSlider.SetTooltip("Set sharpness filter strength.");
	sharpenFilterAmountSlider.SetScriptTip("RenderPath3D::SetSharpenFilterAmount(float value)");
	sharpenFilterAmountSlider.SetSize(XMFLOAT2(100, hei));
	sharpenFilterAmountSlider.SetPos(XMFLOAT2(x + 100, y));
	sharpenFilterAmountSlider.SetValue(editor->renderPath->getSharpenFilterAmount());
	sharpenFilterAmountSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setSharpenFilterAmount(args.fValue);
	});
	AddWidget(&sharpenFilterAmountSlider);

	outlineCheckBox.Create("Cartoon Outline: ");
	outlineCheckBox.SetTooltip("Toggle the full screen cartoon outline effect.");
	outlineCheckBox.SetSize(XMFLOAT2(hei, hei));
	outlineCheckBox.SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox.SetCheck(editor->renderPath->getOutlineEnabled());
	outlineCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setOutlineEnabled(args.bValue);
	});
	AddWidget(&outlineCheckBox);

	outlineThresholdSlider.Create(0, 1, 0.1f, 1000, "Threshold: ");
	outlineThresholdSlider.SetTooltip("Outline edge detection threshold. Increase if not enough otlines are detected, decrease if too many outlines are detected.");
	outlineThresholdSlider.SetSize(XMFLOAT2(100, hei));
	outlineThresholdSlider.SetPos(XMFLOAT2(x + 100, y));
	outlineThresholdSlider.SetValue(editor->renderPath->getOutlineThreshold());
	outlineThresholdSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setOutlineThreshold(args.fValue);
	});
	AddWidget(&outlineThresholdSlider);

	outlineThicknessSlider.Create(0, 4, 1, 1000, "Thickness: ");
	outlineThicknessSlider.SetTooltip("Set outline thickness.");
	outlineThicknessSlider.SetSize(XMFLOAT2(100, hei));
	outlineThicknessSlider.SetPos(XMFLOAT2(x + 100, y += step));
	outlineThicknessSlider.SetValue(editor->renderPath->getOutlineThickness());
	outlineThicknessSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setOutlineThickness(args.fValue);
	});
	AddWidget(&outlineThicknessSlider);

	chromaticaberrationCheckBox.Create("Chromatic Aberration: ");
	chromaticaberrationCheckBox.SetTooltip("Toggle the full screen chromatic aberration effect. This simulates lens distortion at screen edges.");
	chromaticaberrationCheckBox.SetSize(XMFLOAT2(hei, hei));
	chromaticaberrationCheckBox.SetPos(XMFLOAT2(x, y += step));
	chromaticaberrationCheckBox.SetCheck(editor->renderPath->getOutlineEnabled());
	chromaticaberrationCheckBox.OnClick([=](wiEventArgs args) {
		editor->renderPath->setChromaticAberrationEnabled(args.bValue);
		});
	AddWidget(&chromaticaberrationCheckBox);

	chromaticaberrationSlider.Create(0, 4, 1.0f, 1000, "Amount: ");
	chromaticaberrationSlider.SetTooltip("The lens distortion amount.");
	chromaticaberrationSlider.SetSize(XMFLOAT2(100, hei));
	chromaticaberrationSlider.SetPos(XMFLOAT2(x + 100, y));
	chromaticaberrationSlider.SetValue(editor->renderPath->getChromaticAberrationAmount());
	chromaticaberrationSlider.OnSlide([=](wiEventArgs args) {
		editor->renderPath->setChromaticAberrationAmount(args.fValue);
		});
	AddWidget(&chromaticaberrationSlider);


	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 500, 80, 0));
	SetVisible(false);

}
