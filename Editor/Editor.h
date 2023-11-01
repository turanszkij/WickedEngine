#pragma once
#include "Translator.h"
#include "wiScene_BindLua.h"
#include "OptionsWindow.h"
#include "ComponentsWindow.h"
#include "ProfilerWindow.h"
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

	wi::gui::Button newSceneButton;

	wi::gui::Button translateButton;
	wi::gui::Button rotateButton;
	wi::gui::Button scaleButton;

	wi::gui::Button physicsButton;

	wi::gui::Button dummyButton;
	bool dummy_enabled = false;
	bool dummy_male = false;
	XMFLOAT3 dummy_pos = XMFLOAT3(0, 0, 0);

	wi::gui::Button playButton;
	wi::gui::Button stopButton;

	wi::gui::Button saveButton;
	wi::gui::Button openButton;
	wi::gui::Button logButton;
	wi::gui::Button profilerButton;
	wi::gui::Button cinemaButton;
	wi::gui::Button fullscreenButton;
	wi::gui::Button bugButton;
	wi::gui::Button aboutButton;
	wi::gui::Button exitButton;
	wi::gui::Window aboutWindow;
	wi::gui::Label aboutLabel;

	OptionsWindow optionsWnd;
	ComponentsWindow componentsWnd;
	ProfilerWindow profilerWnd;

	wi::physics::PickDragOperation physicsDragOp;

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
	wi::graphics::Texture rt_dummyOutline;
	float outlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);
	wi::Color inactiveEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
	wi::Color hoveredEntityColor = wi::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));
	wi::Color backgroundEntityColor = wi::Color::Black();
	wi::Color dummyColor = wi::Color::White();

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

	void Open(const std::string& filename);
	void Save(const std::string& filename);
	void SaveAs();
	bool deleting = false;

	std::string save_text_message = "";
	std::string save_text_filename = "";
	float save_text_alpha = 0; // seconds until save text disappears
	wi::Color save_text_color = wi::Color::White();
	void PostSaveText(const std::string& message, const std::string& filename = "", float time_seconds = 4);

	std::string last_script_path;

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
		wi::gui::Button tabSelectButton;
		wi::gui::Button tabCloseButton;
	};
	wi::vector<std::unique_ptr<EditorScene>> scenes;
	int current_scene = 0;
	EditorScene& GetCurrentEditorScene() { return *scenes[current_scene].get(); }
	const EditorScene& GetCurrentEditorScene() const { return *scenes[current_scene].get(); }
	wi::scene::Scene& GetCurrentScene() { return scenes[current_scene].get()->scene; }
	const wi::scene::Scene& GetCurrentScene() const { return scenes[current_scene].get()->scene; }
	void SetCurrentScene(int index);
	void RefreshSceneList();
	void NewScene();

	void FocusCameraOnSelected();

	wi::Localization default_localization;
	wi::Localization current_localization;
	void SetDefaultLocalization();
	void SetLocalization(wi::Localization& loc);
	void ReloadLanguage();

	struct FontData
	{
		std::string name;
		wi::vector<uint8_t> filedata;
	};
	wi::vector<FontData> font_datas;
};

class Editor : public wi::Application
{
public:
	EditorComponent renderComponent;
	EditorLoadingScreen loader;
	wi::config::File config;

	void Initialize() override;

	~Editor()
	{
		config.Commit();
	}
};

// Additional localizations that are outside the GUI can be defined here:
enum class EditorLocalization
{
	// Top menu:
	Save,
	Open,
	Backlog,
	Profiler,
	Cinema,
	FullScreen,
	Windowed,
	BugReport,
	About,
	Exit,

	// Other:
	UntitledScene,

	Count
};
static const char* EditorLocalizationStrings[] = {
	// Top menu:
	"Save",
	"Open",
	"Backlog",
	"Profiler",
	"Cinema",
	"Full screen",
	"Windowed",
	"Bug report",
	"About",
	"Exit",

	// Other:
	"Untitled scene"
};
static_assert(arraysize(EditorLocalizationStrings) == size_t(EditorLocalization::Count));
