#pragma once
#include "WickedEngine.h"
#include "Translator.h"
#include "TerrainGenerator.h"
#include "wiScene_BindLua.h"

#include "MaterialWindow.h"
#include "MaterialPickerWindow.h"
#include "PostprocessWindow.h"
#include "WeatherWindow.h"
#include "ObjectWindow.h"
#include "MeshWindow.h"
#include "CameraWindow.h"
#include "RendererWindow.h"
#include "EnvProbeWindow.h"
#include "DecalWindow.h"
#include "LightWindow.h"
#include "AnimationWindow.h"
#include "EmitterWindow.h"
#include "HairParticleWindow.h"
#include "ForceFieldWindow.h"
#include "SoundWindow.h"
#include "PaintToolWindow.h"
#include "SpringWindow.h"
#include "IKWindow.h"
#include "TransformWindow.h"
#include "LayerWindow.h"
#include "NameWindow.h"

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
	MaterialWindow materialWnd;
	MaterialPickerWindow materialPickerWnd;
	PostprocessWindow postprocessWnd;
	WeatherWindow weatherWnd;
	ObjectWindow objectWnd;
	MeshWindow meshWnd;
	CameraWindow cameraWnd;
	RendererWindow rendererWnd;
	EnvProbeWindow envProbeWnd;
	DecalWindow decalWnd;
	SoundWindow soundWnd;
	LightWindow lightWnd;
	AnimationWindow animWnd;
	EmitterWindow emitterWnd;
	HairParticleWindow hairWnd;
	ForceFieldWindow forceFieldWnd;
	PaintToolWindow paintToolWnd;
	SpringWindow springWnd;
	IKWindow ikWnd;
	TransformWindow transformWnd;
	LayerWindow layerWnd;
	NameWindow nameWnd;
	TerrainGenerator terragen;

	Editor* main = nullptr;

	wi::gui::Button saveButton;
	wi::gui::Button openButton;
	wi::gui::Button closeButton;
	wi::gui::Button aboutButton;
	wi::gui::Button exitButton;
	wi::gui::Label aboutLabel;

	wi::gui::Window optionsWnd;
	wi::gui::CheckBox translatorCheckBox;
	wi::gui::CheckBox isScalatorCheckBox;
	wi::gui::CheckBox isRotatorCheckBox;
	wi::gui::CheckBox isTranslatorCheckBox;
	wi::gui::CheckBox profilerEnabledCheckBox;
	wi::gui::CheckBox physicsEnabledCheckBox;
	wi::gui::CheckBox cinemaModeCheckBox;
	wi::gui::CheckBox versionCheckBox;
	wi::gui::CheckBox fpsCheckBox;
	wi::gui::CheckBox otherinfoCheckBox;
	wi::gui::ComboBox themeCombo;
	wi::gui::ComboBox renderPathComboBox;
	wi::gui::ComboBox saveModeComboBox;
	wi::gui::ComboBox sceneComboBox;
	void RefreshOptionsWindow();

	enum class Filter : uint64_t
	{
		Transform = 1 << 0,
		Material = 1 << 1,
		Mesh = 1 << 2,
		Object = 1 << 3,
		EnvironmentProbe = 1 << 4,
		Decal = 1 << 5,
		Sound = 1 << 6,
		Weather = 1 << 7,
		Light = 1 << 8,

		All = ~0ull,
	} filter = Filter::All;
	wi::gui::ComboBox newCombo;
	wi::gui::ComboBox filterCombo;
	wi::gui::TreeList entityTree;
	wi::unordered_set<wi::ecs::Entity> entitytree_added_items;
	wi::unordered_set<wi::ecs::Entity> entitytree_opened_items;
	void PushToEntityTree(wi::ecs::Entity entity, int level);
	void RefreshEntityTree();

	wi::gui::ComboBox newComponentCombo;
	wi::gui::Window componentWindow;
	void RefreshComponentWindow();

	wi::gui::Slider pathTraceTargetSlider;
	wi::gui::Label pathTraceStatisticsLabel;

	std::unique_ptr<wi::RenderPath3D> renderPath;
	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);
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
		RefreshEntityTree();
		RefreshSceneList();
	}
	void RefreshSceneList()
	{
		sceneComboBox.ClearItems();
		for (int i = 0; i < int(scenes.size()); ++i)
		{
			if (scenes[i]->path.empty())
			{
				sceneComboBox.AddItem("Untitled");
			}
			else
			{
				sceneComboBox.AddItem(wi::helper::RemoveExtension(wi::helper::GetFileNameFromPath(scenes[i]->path)));
			}
		}
		sceneComboBox.AddItem("[New]");
		sceneComboBox.SetSelectedWithoutCallback(current_scene);
		std::string tooltip = "Choose a scene";
		if (!GetCurrentEditorScene().path.empty())
		{
			tooltip += "\nCurrent path: " + GetCurrentEditorScene().path;
		}
		sceneComboBox.SetTooltip(tooltip);
	}
	void NewScene()
	{
		scenes.push_back(std::make_unique<EditorScene>());
		SetCurrentScene(int(scenes.size()) - 1);
		RefreshSceneList();
		cameraWnd.ResetCam();
	}
};

class Editor : public wi::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;

	void Initialize() override;
};


template<>
struct enable_bitmask_operators<EditorComponent::Filter> {
	static const bool enable = true;
};
