#pragma once
class EditorComponent;

class ArmatureWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);
	void RefreshBoneList();

	wi::gui::Label infoLabel;
	wi::gui::Button resetPoseButton;
	wi::gui::Button createHumanoidButton;
	wi::gui::TreeList boneList;

	void ResizeLayout() override;
};

