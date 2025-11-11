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
	wi::gui::Slider masterVolumeSlider;
	wi::gui::ComboBox themeCombo;
	wi::gui::Button themeEditorButton;
	wi::gui::ComboBox saveModeComboBox;
	wi::gui::CheckBox saveCompressionCheckBox;
	wi::gui::ComboBox entityTreeSortingComboBox;
	wi::gui::ComboBox languageCombo;

	wi::gui::CheckBox physicsDebugCheckBox;
	wi::gui::Slider physicsDebugMaxDistanceSlider;
	wi::gui::CheckBox nameDebugCheckBox;
	wi::gui::CheckBox gridHelperCheckBox;
	wi::gui::CheckBox aabbDebugCheckBox;
	wi::gui::CheckBox boneLinesCheckBox;
	wi::gui::CheckBox debugEmittersCheckBox;
	wi::gui::CheckBox debugForceFieldsCheckBox;
	wi::gui::CheckBox debugRaytraceBVHCheckBox;
	wi::gui::CheckBox placeInFrontOfCameraCheckBox;
	wi::gui::ComboBox wireFrameComboBox;
	wi::gui::CheckBox envProbesCheckBox;
	wi::gui::CheckBox cameraVisCheckBox;
	wi::gui::CheckBox colliderVisCheckBox;
	wi::gui::CheckBox springVisCheckBox;
	wi::gui::CheckBox splineVisCheckBox;
	wi::gui::CheckBox freezeCullingCameraCheckBox;
	wi::gui::CheckBox disableAlbedoMapsCheckBox;
	wi::gui::CheckBox forceDiffuseLightingCheckBox;
	wi::gui::CheckBox focusModeCheckBox;
	wi::gui::CheckBox disableRoundCornersCheckBox;
	wi::gui::CheckBox disableGradientCheckBox;

	wi::gui::Slider outlineOpacitySlider;
	wi::gui::Slider transformToolOpacitySlider;
	wi::gui::Slider transformToolDarkenSlider;
	wi::gui::Slider bonePickerOpacitySlider;
	wi::gui::CheckBox skeletonsVisibleCheckBox;

	wi::gui::Button localizationButton;
	wi::gui::Button eliminateCoarseCascadesButton;
	wi::gui::Button ddsConvButton;
	wi::gui::Button duplicateCollidersButton;

	std::string currentTheme;

	void ResizeLayout() override;

	void RefreshLanguageSelectionAfterWholeGUIWasInitialized();

	void RefreshTheme();
	void ReloadThemes();
};

