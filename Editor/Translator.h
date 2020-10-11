#pragma once
#include "CommonInclude.h"

#include <list>

class Translator
{
private:
	XMFLOAT4 prevPointer = XMFLOAT4(0, 0, 0, 0);
	XMFLOAT4X4 dragDeltaMatrix = IDENTITYMATRIX;
	bool dragging = false;
	bool dragStarted = false;
	bool dragEnded = false;
public:
	void Create();

	void Update();
	void Draw(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate();

	wiScene::TransformComponent transform;
	std::list<wiScene::PickResult> selected;

	bool enabled = false;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state = TRANSLATOR_IDLE;

	float dist = 1;

	bool isTranslator = true, isScalator = false, isRotator = false;


	// Check if the drag started in this exact frame
	bool IsDragStarted() const { return dragStarted; };
	// Check if the drag ended in this exact frame
	bool IsDragEnded() const { return dragEnded; };
	// Delta matrix from beginning to end of drag operation
	XMFLOAT4X4 GetDragDeltaMatrix() const { return dragDeltaMatrix; }
};

