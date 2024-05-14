#pragma once
#include "GraphicsWindow.h"
#include "CameraWindow.h"
#include "MaterialPickerWindow.h"
#include "PaintToolWindow.h"
#include "GeneralWindow.h"

class EditorComponent;

class OptionsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);
	void Update(float dt);

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	GeneralWindow generalWnd;
	GraphicsWindow graphicsWnd;
	CameraWindow cameraWnd;
	MaterialPickerWindow materialPickerWnd;
	PaintToolWindow paintToolWnd;

};
