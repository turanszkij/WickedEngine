#include "stdafx.h"
#include "ScriptWindow.h"
#include "Editor.h"

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

		editor->optionsWnd.RefreshEntityTree();
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
					script->resource = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					script->filename = fileName;
					fileButton.SetText(wi::helper::GetFileNameFromPath(fileName));
					});
				});
		}
	});
	AddWidget(&fileButton);

	playstopButton.Create(ICON_PLAY);
	playstopButton.SetTooltip("Play / Stop script");
	playstopButton.SetSize(XMFLOAT2(wid, hei));
	playstopButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);
		if (script == nullptr)
			return;

		if (script->IsPlaying())
		{
			script->Stop();
			playstopButton.SetText(ICON_PLAY);
		}
		else
		{
			script->Play();
			playstopButton.SetText(ICON_STOP);
		}
	});
	AddWidget(&playstopButton);

	SetMinimized(true);
	SetVisible(false);
}

void ScriptWindow::SetEntity(wi::ecs::Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	wi::scene::Scene& scene = editor->GetCurrentScene();
	wi::scene::ScriptComponent* script = scene.scripts.GetComponent(entity);

	if (script != nullptr)
	{
		if (script->resource.IsValid())
		{
			fileButton.SetText(wi::helper::GetFileNameFromPath(script->filename));

			if (script->IsPlaying())
			{
				playstopButton.SetText(ICON_STOP);
			}
			else
			{
				playstopButton.SetText(ICON_PLAY);
			}
		}
		else
		{
			fileButton.SetText("Open File...");
		}
	}
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
		playstopButton.SetPos(XMFLOAT2(60, fileButton.GetPos().y + fileButton.GetSize().y + 4));
		playstopButton.SetSize(XMFLOAT2(GetSize().x - 65, playstopButton.GetSize().y));
	}
	else
	{
		playstopButton.SetVisible(false);
	}
}
