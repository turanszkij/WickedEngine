#pragma once
#include "WickedEngine.h"
#include "Translator.h"
#include "wiScene_BindLua.h"
#include "OptionsWindow.h"
#include "ComponentsWindow.h"

#include "IconDefinitions.h"

class EditorLoadingScreen : public wi::LoadingScreen
{
private:
	wi::Sprite sprite;
	wi::SpriteFont font;
public:
	void Load() override;
	void Update(float dt) override;
};

class Editor;
class EditorComponent : public wi::RenderPath2D
{
public:
	Editor* main = nullptr;

	wi::gui::Button saveButton;
	wi::gui::Button openButton;
	wi::gui::Button closeButton;
	wi::gui::Button logButton;
	wi::gui::Button fullscreenButton;
	wi::gui::Button bugButton;
	wi::gui::Button aboutButton;
	wi::gui::Button exitButton;
	wi::gui::Label aboutLabel;

	OptionsWindow optionsWnd;
	ComponentsWindow componentsWnd;

	std::unique_ptr<wi::RenderPath3D> renderPath;
	const wi::graphics::Texture* GetGUIBlurredBackground() const override { return renderPath->GetGUIBlurredBackground(); }

	void ResizeBuffers() override;
	void ResizeLayout() override;
	void Load() override;
	void Start() override;
	void PreUpdate() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void PostUpdate() override;
	void Render() const override;
	void Compose(wi::graphics::CommandList cmd) const override;


	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
		EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
		EDITORSTENCILREF_LAST = 0x0F,
	};
	wi::graphics::Texture rt_selectionOutline_MSAA;
	wi::graphics::Texture rt_selectionOutline[2];
	wi::graphics::RenderPass renderpass_selectionOutline[2];
	float selectionOutlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);
	const wi::Color inactiveEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
	const wi::Color hoveredEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));

	wi::graphics::RenderPass renderpass_editor;
	wi::graphics::Texture editor_depthbuffer;

	Translator translator;
	wi::scene::PickResult hovered;
	bool inspector_mode = false;
	wi::ecs::Entity grass_interaction_entity = wi::ecs::INVALID_ENTITY;

	void ClearSelected();
	void AddSelected(wi::ecs::Entity entity);
	void AddSelected(const wi::scene::PickResult& picked);
	bool IsSelected(wi::ecs::Entity entity) const;
	bool selectAll = false;
	wi::unordered_set<wi::ecs::Entity> selectAllStorage;

	bool bone_picking = false;
	void CheckBonePickingEnabled();

	void UpdateTopMenuAnimation();

	wi::Archive clipboard;

	enum HistoryOperationType
	{
		HISTORYOP_TRANSLATOR,		// translator interaction
		HISTORYOP_SELECTION,		// selection changed
		HISTORYOP_ADD,				// entity added
		HISTORYOP_DELETE,			// entity removed
		HISTORYOP_COMPONENT_DATA,	// generic component data changed
		HISTORYOP_PAINTTOOL,		// paint tool interaction
		HISTORYOP_NONE
	};

	void RecordSelection(wi::Archive& archive) const;
	void RecordEntity(wi::Archive& archive, wi::ecs::Entity entity);
	void RecordEntity(wi::Archive& archive, const wi::vector<wi::ecs::Entity>& entities);

	void ResetHistory();
	wi::Archive& AdvanceHistory();
	void ConsumeHistoryOperation(bool undo);

	void Save(const std::string& filename);
	void SaveAs();
	bool deleting = false;

	struct EditorScene
	{
		std::string path;
		wi::scene::Scene scene;
		XMFLOAT3 cam_move = {};
		wi::scene::CameraComponent camera;
		wi::scene::TransformComponent camera_transform;
		wi::scene::TransformComponent camera_target;
		wi::vector<wi::Archive> history;
		int historyPos = -1;
	};
	wi::vector<std::unique_ptr<EditorScene>> scenes;
	int current_scene = 0;
	EditorScene& GetCurrentEditorScene() { return *scenes[current_scene].get(); }
	const EditorScene& GetCurrentEditorScene() const { return *scenes[current_scene].get(); }
	wi::scene::Scene& GetCurrentScene() { return scenes[current_scene].get()->scene; }
	const wi::scene::Scene& GetCurrentScene() const { return scenes[current_scene].get()->scene; }
	void SetCurrentScene(int index)
	{
		current_scene = index;
		this->renderPath->scene = &scenes[current_scene].get()->scene;
		this->renderPath->camera = &scenes[current_scene].get()->camera;
		wi::lua::scene::SetGlobalScene(this->renderPath->scene);
		wi::lua::scene::SetGlobalCamera(this->renderPath->camera);
		optionsWnd.RefreshEntityTree();
		RefreshSceneList();
	}
	void RefreshSceneList()
	{
		optionsWnd.sceneComboBox.ClearItems();
		for (int i = 0; i < int(scenes.size()); ++i)
		{
			if (scenes[i]->path.empty())
			{
				optionsWnd.sceneComboBox.AddItem("Untitled");
			}
			else
			{
				optionsWnd.sceneComboBox.AddItem(wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(scenes[i]->path)));
			}
		}
		optionsWnd.sceneComboBox.AddItem("[New]");
		optionsWnd.sceneComboBox.SetSelectedWithoutCallback(current_scene);
		std::string tooltip = "Choose a scene";
		if (!GetCurrentEditorScene().path.empty())
		{
			tooltip += "\nCurrent path: " + GetCurrentEditorScene().path;
		}
		optionsWnd.sceneComboBox.SetTooltip(tooltip);
	}
	void NewScene()
	{
		scenes.push_back(std::make_unique<EditorScene>());
		SetCurrentScene(int(scenes.size()) - 1);
		RefreshSceneList();
		optionsWnd.cameraWnd.ResetCam();
	}
};

class Editor : public wi::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;
	wi::config::File config;

	void Initialize() override;
};
