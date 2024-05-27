#pragma once
class EditorComponent;

class GeneralWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::gui::CheckBox versionCheckBox;
	wi::gui::CheckBox fpsCheckBox;
	wi::gui::CheckBox otherinfoCheckBox;
	wi::gui::ComboBox themeCombo;
	wi::gui::ComboBox saveModeComboBox;
	wi::gui::ComboBox languageCombo;

	wi::gui::CheckBox physicsDebugCheckBox;
	wi::gui::CheckBox nameDebugCheckBox;
	wi::gui::CheckBox gridHelperCheckBox;
	wi::gui::CheckBox aabbDebugCheckBox;
	wi::gui::CheckBox boneLinesCheckBox;
	wi::gui::CheckBox debugEmittersCheckBox;
	wi::gui::CheckBox debugForceFieldsCheckBox;
	wi::gui::CheckBox debugRaytraceBVHCheckBox;
	wi::gui::CheckBox wireFrameCheckBox;
	wi::gui::CheckBox envProbesCheckBox;
	wi::gui::CheckBox cameraVisCheckBox;
	wi::gui::CheckBox colliderVisCheckBox;
	wi::gui::CheckBox springVisCheckBox;
	wi::gui::CheckBox freezeCullingCameraCheckBox;
	wi::gui::CheckBox disableAlbedoMapsCheckBox;
	wi::gui::CheckBox forceDiffuseLightingCheckBox;

	wi::gui::Slider transformToolOpacitySlider;
	wi::gui::Slider bonePickerOpacitySlider;
	wi::gui::CheckBox skeletonsVisibleCheckBox;

	wi::gui::Button localizationButton;
	wi::gui::Button eliminateCoarseCascadesButton;
	wi::gui::Button ddsConvButton;
	wi::gui::Button ktxConvButton;

	void ResizeLayout() override;

	void RefreshLanguageSelectionAfterWholeGUIWasInitialized();
};

