#include "stdafx.h"
#include "ArmatureWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ArmatureWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_ARMATURE " Armature", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 350));

	closeButton.SetTooltip("Delete ArmatureComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().armatures.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(100, 50));
	infoLabel.SetText("This window will stay open even if you select other entities until it is collapsed, so you can select other bone entities.");
	AddWidget(&infoLabel);

	resetPoseButton.Create("Reset Pose");
	resetPoseButton.SetSize(XMFLOAT2(wid, hei));
	resetPoseButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		const ArmatureComponent* armature = scene.armatures.GetComponent(entity);
		if (armature == nullptr)
			return;

		XMMATRIX R = XMMatrixIdentity();
		const TransformComponent* armature_transform = scene.transforms.GetComponent(entity);
		if (armature_transform != nullptr)
		{
			R = XMMatrixInverse(nullptr, XMLoadFloat4x4(&armature_transform->world));
		}

		for (size_t i = 0; i < armature->boneCollection.size(); ++i)
		{
			Entity bone = armature->boneCollection[i];
			TransformComponent* transform = scene.transforms.GetComponent(bone);
			if (transform != nullptr)
			{
				transform->ClearTransform();
				transform->MatrixTransform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&armature->inverseBindMatrices[i])) * R);
				transform->UpdateTransform();
				const HierarchyComponent* hier = scene.hierarchy.GetComponent(bone);
				if (hier != nullptr && hier->parentID != INVALID_ENTITY)
				{
					scene.Component_Attach(bone, hier->parentID, false);
				}
			}
		}
		});
	AddWidget(&resetPoseButton);

	boneList.Create("Bones: ");
	boneList.SetSize(XMFLOAT2(wid, 200));
	boneList.SetPos(XMFLOAT2(4, y += step));
	boneList.OnSelect([=](wi::gui::EventArgs args) {

		if (args.iValue < 0)
			return;

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_SELECTION;
		// record PREVIOUS selection state...
		editor->RecordSelection(archive);

		editor->translator.selected.clear();

		for (int i = 0; i < boneList.GetItemCount(); ++i)
		{
			const wi::gui::TreeList::Item& item = boneList.GetItem(i);
			if (item.selected)
			{
				wi::scene::PickResult pick;
				pick.entity = (Entity)item.userdata;
				editor->AddSelected(pick);
			}
		}

		// record NEW selection state...
		editor->RecordSelection(archive);

		editor->optionsWnd.RefreshEntityTree();

		});
	AddWidget(&boneList);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void ArmatureWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	Scene& scene = editor->GetCurrentScene();

	const ArmatureComponent* armature = scene.armatures.GetComponent(entity);

	if (armature != nullptr || IsCollapsed())
	{
		this->entity = entity;
		RefreshBoneList();
	}
}
void ArmatureWindow::RefreshBoneList()
{
	Scene& scene = editor->GetCurrentScene();

	const ArmatureComponent* armature = scene.armatures.GetComponent(entity);

	if (armature != nullptr)
	{
		boneList.ClearItems();
		for (Entity bone : armature->boneCollection)
		{
			wi::gui::TreeList::Item item;
			item.userdata = bone;
			item.name += ICON_BONE " ";

			const NameComponent* name = scene.names.GetComponent(bone);
			if (name == nullptr)
			{
				item.name += "[no_name] " + std::to_string(bone);
			}
			else if (name->name.empty())
			{
				item.name += "[name_empty] " + std::to_string(bone);
			}
			else
			{
				item.name += name->name;
			}
			boneList.AddItem(item);
		}
	}
}

void ArmatureWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 110;
	const float margin_right = 45;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(infoLabel);
	add_fullwidth(resetPoseButton);

	y += jump;

	add_fullwidth(boneList);

}
