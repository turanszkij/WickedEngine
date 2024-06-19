#pragma once
#include "Translator.h"
#include "wiScene_BindLua.h"
#include "ComponentsWindow.h"
#include "ProfilerWindow.h"
#include "ContentBrowserWindow.h"
#include "GraphicsWindow.h"
#include "CameraWindow.h"
#include "MaterialPickerWindow.h"
#include "PaintToolWindow.h"
#include "GeneralWindow.h"
#include "IconDefinitions.h"

class Editor;
class EditorComponent : public wi::RenderPath2D
{
public:
	Editor* main = nullptr;

	wi::gui::Button newSceneButton;

	wi::gui::ComboBox newEntityCombo;

	wi::gui::Button translateButton;
	wi::gui::Button rotateButton;
	wi::gui::Button scaleButton;

	wi::gui::Button physicsButton;

	wi::gui::Button dummyButton;
	bool dummy_enabled = false;
	bool dummy_male = false;
	XMFLOAT3 dummy_pos = XMFLOAT3(0, 0, 0);

	wi::gui::Button navtestButton;
	bool navtest_enabled = false;
	wi::scene::PickResult navtest_start_pick;
	wi::scene::PickResult navtest_goal_pick;
	wi::PathQuery navtest_pathquery;

	wi::gui::Button playButton;
	wi::gui::Button stopButton;

	wi::gui::Button saveButton;
	wi::gui::Button openButton;
	wi::gui::Button contentBrowserButton;
	wi::gui::Button logButton;
	wi::gui::Button profilerButton;
	wi::gui::Button cinemaButton;
	wi::gui::Button fullscreenButton;
	wi::gui::Button bugButton;
	wi::gui::Button aboutButton;
	wi::gui::Button exitButton;
	wi::gui::Window aboutWindow;
	wi::gui::Label aboutLabel;

	ComponentsWindow componentsWnd;
	ProfilerWindow profilerWnd;
	ContentBrowserWindow contentBrowserWnd;
	wi::gui::Window topmenuWnd;

	wi::gui::Button generalButton;
	wi::gui::Button graphicsButton;
	wi::gui::Button cameraButton;
	wi::gui::Button materialsButton;
	wi::gui::Button paintToolButton;

	GeneralWindow generalWnd;
	GraphicsWindow graphicsWnd;
	CameraWindow cameraWnd;
	MaterialPickerWindow materialPickerWnd;
	PaintToolWindow paintToolWnd;

	wi::primitive::Ray pickRay;
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

	wi::graphics::Viewport viewport3D;
	wi::primitive::Hitbox2D viewport3D_hitbox;
	void ResizeViewport3D();

	bool camControlStart = true;
	XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
	XMFLOAT4 currentMouse = XMFLOAT4(0, 0, 0, 0);

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

	const wi::Color springDebugColor = wi::Color(255, 70, 165, 255);
	const wi::Color ikDebugColor = wi::Color(49, 190, 103, 255);

	wi::graphics::Texture editor_depthbuffer;
	wi::graphics::Texture editor_rendertarget;

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

	void UpdateDynamicWidgets();

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

	wi::vector<std::string> recentFilenames;
	size_t maxRecentFilenames = 10;
	wi::vector<std::string> recentFolders;
	size_t maxRecentFolders = 8;
	void RegisterRecentlyUsed(const std::string& filename);

	void Open(std::string filename);
	void Save(const std::string& filename);
	void SaveAs();
	bool deleting = false;

	wi::graphics::Texture CreateThumbnailScreenshot() const;

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

	wi::jobsystem::context loadmodel_workload;
	wi::SpriteFont loadmodel_font;
};

class Editor : public wi::Application
{
public:
	EditorComponent renderComponent;
	wi::config::File config;

	void Initialize() override;

	void HotReload();

	~Editor() override
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
	__REMOVED_Cinema,
	FullScreen,
	Windowed,
	BugReport,
	About,
	Exit,
	UntitledScene,
	ContentBrowser,

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
	"Untitled scene",
	"Content",
};
static_assert(arraysize(EditorLocalizationStrings) == size_t(EditorLocalization::Count));
