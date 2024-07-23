#include "stdafx.h"
#include "ScriptWindow.h"

using namespace wi::scene;

void ScriptWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SCRIPT " Script", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(520, 80));

	closeButton.SetTooltip("Delete Script");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().scripts.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float hei = 20;
	float wid = 100;

	fileButton.Create("Open File...");
	fileButton.SetDescription("File: ");
	fileButton.SetSize(XMFLOAT2(wid, hei));
	fileButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);
		if (script == nullptr)
			return;

		if (script->resource.IsValid())
		{
			script->resource = {};
			script->filename = {};
			script->script = {};
			fileButton.SetText("Open File...");
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Lua Script (*.lua)";
			params.extensions = wi::resourcemanager::GetSupportedScriptExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					script->CreateFromFile(fileName);
					fileButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
	});
	AddWidget(&fileButton);

	playonceCheckBox.Create("Once: ");
	playonceCheckBox.SetTooltip("Play the script only one time, and stop immediately.\nUseful for having custom update frequency logic in the script.");
	playonceCheckBox.SetSize(XMFLOAT2(hei, hei));
	playonceCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ScriptComponent* script = scene.scripts.GetComponent(x.entity);
			if (script == nullptr)
				continue;
			script->SetPlayOnce(args.bValue);
		}
	});
	AddWidget(&playonceCheckBox);

	playstopButton.Create("");
	playstopButton.SetTooltip("Play / Stop script");
	playstopButton.SetSize(XMFLOAT2(wid, hei));
	playstopButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ScriptComponent* script = scene.scripts.GetComponent(x.entity);
			if (script == nullptr)
				continue;
			if (script->IsPlaying())
			{
				script->Stop();
			}
			else
			{
				script->Play();
			}
		}
	});
	AddWidget(&playstopButton);

	SetMinimized(true);
	SetVisible(false);
}

void ScriptWindow::SetEntity(wi::ecs::Entity entity)
{
	this->entity = entity;

	wi::scene::Scene& scene = editor->GetCurrentScene();
	wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);

	if (script != nullptr)
	{
		if (script->resource.IsValid())
		{
			fileButton.SetText(wi::helper::GetFileNameFromPath(script->filename));
		}
		else
		{
			fileButton.SetText("Open File...");
		}
		playonceCheckBox.SetCheck(script->IsPlayingOnlyOnce());
	}
	else
	{
		fileButton.SetText("Open File...");
	}
}

void ScriptWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::scene::Scene& scene = editor->GetCurrentScene();
	wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);
	if (script != nullptr)
	{
		if (script->IsPlaying())
		{
			playstopButton.SetText(ICON_STOP);
		}
		else
		{
			playstopButton.SetText(ICON_PLAY);
		}
	}

	wi::gui::Window::Update(canvas, dt);
}
void ScriptWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	fileButton.SetPos(XMFLOAT2(60, 4));
	fileButton.SetSize(XMFLOAT2(GetSize().x - 65, fileButton.GetSize().y));

	wi::scene::Scene& scene = editor->GetCurrentScene();
	wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);
	if (script != nullptr && script->resource.IsValid())
	{
		playstopButton.SetVisible(true);
		playstopButton.SetPos(XMFLOAT2(84, fileButton.GetPos().y + fileButton.GetSize().y + 4));
		playstopButton.SetSize(XMFLOAT2(GetSize().x - 90, playstopButton.GetSize().y));

		playonceCheckBox.SetVisible(true);
		playonceCheckBox.SetPos(XMFLOAT2(playstopButton.GetPos().x - playonceCheckBox.GetSize().x - 4, playstopButton.GetPos().y));
	}
	else
	{
		playstopButton.SetVisible(false);
		playonceCheckBox.SetVisible(false);
	}
}
