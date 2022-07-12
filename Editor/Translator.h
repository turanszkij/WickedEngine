#pragma once
#include "CommonInclude.h"
#include "wiCanvas.h"
#include "wiVector.h"
#include "wiUnorderedSet.h"

class Translator
{
private:
	bool dragging = false;
	bool dragStarted = false;
	bool dragEnded = false;
	XMFLOAT3 intersection_start = XMFLOAT3(0, 0, 0);
	XMFLOAT3 axis = XMFLOAT3(1, 0, 0);
	float angle = 0;
	float angle_start = 0;
public:

	void Update(const wi::Canvas& canvas);
	void Draw(const wi::scene::CameraComponent& camera, wi::graphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate();

	wi::scene::TransformComponent transform;
	wi::vector<wi::scene::PickResult> selected; // all the selected picks
	wi::unordered_set<wi::ecs::Entity> selectedEntitiesLookup; // fast lookup for selected entities
	wi::vector<wi::ecs::Entity> selectedEntitiesNonRecursive; // selected entities that don't contain entities that would be included in recursive iterations

	bool enabled = false;
	float angle_snap = XM_PIDIV4;

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

	wi::scene::TransformComponent transform_start;
	wi::vector<XMFLOAT4X4> matrices_start;
	wi::vector<XMFLOAT4X4> matrices_current;
};

