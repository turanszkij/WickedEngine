#include "stdafx.h"
#include "HumanoidWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void HumanoidWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_HUMANOID " Humanoid", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 350));

	closeButton.SetTooltip("Delete HumanoidComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().humanoids.Remove(entity);

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

	lookatCheckBox.Create("Look At Enabled: ");
	lookatCheckBox.SetSize(XMFLOAT2(hei, hei));
	lookatCheckBox.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		HumanoidComponent* humanoid = scene.humanoids.GetComponent(entity);
		if (humanoid != nullptr)
		{
			humanoid->SetLookAtEnabled(args.bValue);
		}
		});
	AddWidget(&lookatCheckBox);

	lookatMouseCheckBox.Create("Look At Mouse: ");
	lookatMouseCheckBox.SetSize(XMFLOAT2(hei, hei));
	AddWidget(&lookatMouseCheckBox);
	lookatMouseCheckBox.SetCheck(true);

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
				if (pick.entity != INVALID_ENTITY)
				{
					editor->AddSelected(pick);
				}
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

void HumanoidWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	Scene& scene = editor->GetCurrentScene();

	const HumanoidComponent* humanoid = scene.humanoids.GetComponent(entity);

	if (humanoid != nullptr || IsCollapsed())
	{
		this->entity = entity;
		RefreshBoneList();

		if (humanoid != nullptr)
		{
			lookatCheckBox.SetCheck(humanoid->IsLookAtEnabled());
		}
	}
}
void HumanoidWindow::RefreshBoneList()
{
	Scene& scene = editor->GetCurrentScene();

	const HumanoidComponent* humanoid = scene.humanoids.GetComponent(entity);

	if (humanoid != nullptr)
	{
		boneList.ClearItems();
		for (int i = 0; i < arraysize(humanoid->bones); ++i)
		{
			HumanoidComponent::HumanoidBone type = (HumanoidComponent::HumanoidBone)i;
			Entity bone = humanoid->bones[i];

			wi::gui::TreeList::Item item;
			item.userdata = bone;
			item.level = 1;

			item.name += ICON_BONE " [";

			switch (type)
			{
			case wi::scene::HumanoidComponent::HumanoidBone::Hips:
				boneList.AddItem("Torso"); // grouping item
				item.name += "Hips";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::Spine:
				item.name += "Spine";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::Chest:
				item.name += "Chest";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::UpperChest:
				item.name += "UpperChest";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::Neck:
				item.name += "Neck";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::Head:
				boneList.AddItem("Head"); // grouping item
				item.name += "Head";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftEye:
				item.name += "LeftEye";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightEye:
				item.name += "RightEye";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::Jaw:
				item.name += "Jaw";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftUpperLeg:
				boneList.AddItem("Legs"); // grouping item
				item.name += "LeftUpperLeg";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftLowerLeg:
				item.name += "LeftLowerLeg";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftFoot:
				item.name += "LeftFoot";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftToes:
				item.name += "LeftToes";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightUpperLeg:
				item.name += "RightUpperLeg";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightLowerLeg:
				item.name += "RightLowerLeg";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightFoot:
				item.name += "RightFoot";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightToes:
				item.name += "RightToes";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftShoulder:
				boneList.AddItem("Arms"); // grouping item
				item.name += "LeftShoulder";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftUpperArm:
				item.name += "LeftUpperArm";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftLowerArm:
				item.name += "LeftLowerArm";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftHand:
				item.name += "LeftHand";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightShoulder:
				item.name += "RightShoulder";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightUpperArm:
				item.name += "RightUpperArm";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightLowerArm:
				item.name += "RightLowerArm";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightHand:
				item.name += "RightHand";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftThumbMetacarpal:
				boneList.AddItem("Fingers"); // grouping item
				item.name += "LeftThumbMetacarpal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftThumbProximal:
				item.name += "LeftThumbProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftThumbDistal:
				item.name += "LeftThumbDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftIndexProximal:
				item.name += "LeftIndexProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftIndexIntermediate:
				item.name += "LeftIndexIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftIndexDistal:
				item.name += "LeftIndexDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftMiddleProximal:
				item.name += "LeftMiddleProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftMiddleIntermediate:
				item.name += "LeftMiddleIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftMiddleDistal:
				item.name += "LeftMiddleDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftRingProximal:
				item.name += "LeftRingProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftRingIntermediate:
				item.name += "LeftRingIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftRingDistal:
				item.name += "LeftRingDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftLittleProximal:
				item.name += "LeftLittleProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftLittleIntermediate:
				item.name += "LeftLittleIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::LeftLittleDistal:
				item.name += "LeftLittleDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightThumbMetacarpal:
				item.name += "RightThumbMetacarpal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightThumbProximal:
				item.name += "RightThumbProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightThumbDistal:
				item.name += "RightThumbDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightIndexIntermediate:
				item.name += "RightIndexIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightIndexDistal:
				item.name += "RightIndexDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightIndexProximal:
				item.name += "RightIndexProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightMiddleProximal:
				item.name += "RightMiddleProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightMiddleIntermediate:
				item.name += "RightMiddleIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightMiddleDistal:
				item.name += "RightMiddleDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightRingProximal:
				item.name += "RightRingProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightRingIntermediate:
				item.name += "RightRingIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightRingDistal:
				item.name += "RightRingDistal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightLittleProximal:
				item.name += "RightLittleProximal";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightLittleIntermediate:
				item.name += "RightLittleIntermediate";
				break;
			case wi::scene::HumanoidComponent::HumanoidBone::RightLittleDistal:
				item.name += "RightLittleDistal";
				break;
			default:
				assert(0); // unhandled type
				break;
			}
			item.name += "] ";

			if (bone == INVALID_ENTITY)
			{
				item.name += ICON_DISABLED;
			}
			else
			{
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
			}

			boneList.AddItem(item);
		}
	}
}

void HumanoidWindow::Update(const wi::Canvas& canvas, float dt)
{
	wi::gui::Window::Update(canvas, dt);

	if (lookatMouseCheckBox.GetCheck())
	{
		Scene& scene = editor->GetCurrentScene();
		const CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		const wi::input::MouseState& mouse = wi::input::GetMouseState();
		wi::primitive::Ray ray = wi::renderer::GetPickRay((long)mouse.position.x, (long)mouse.position.y, canvas, camera);
		XMFLOAT3 lookAt;
		XMStoreFloat3(&lookAt, XMLoadFloat3(&ray.origin) + XMLoadFloat3(&ray.direction)); // look at near plane position

		for (size_t i = 0; i < scene.humanoids.GetCount(); ++i)
		{
			HumanoidComponent& humanoid = scene.humanoids[i];
			humanoid.lookAt = lookAt;
		}
	}
}
void HumanoidWindow::ResizeLayout()
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
	add_right(lookatCheckBox);
	lookatMouseCheckBox.SetPos(XMFLOAT2(lookatCheckBox.GetPos().x - 150, lookatCheckBox.GetPos().y));

	y += jump;

	add_fullwidth(boneList);

}
