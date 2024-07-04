#include "stdafx.h"
#include "ArmatureWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void ArmatureWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_ARMATURE " Armature", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(670, 380));

	closeButton.SetTooltip("Delete ArmatureComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().armatures.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 4;
	float hei = 20;
	float step = hei + 2;
	float wid = 220;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(100, 80));
	infoLabel.SetText("This window will stay open even if you select other entities until it is collapsed, so you can select other bone entities.");
	AddWidget(&infoLabel);

	resetPoseButton.Create("Reset Pose");
	resetPoseButton.SetTooltip("Reset Pose will be performed on the Armature, based on the bone inverse bind matrices.");
	resetPoseButton.SetSize(XMFLOAT2(wid, hei));
	resetPoseButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		scene.ResetPose(entity);
	});
	AddWidget(&resetPoseButton);

	createHumanoidButton.Create("Try to create humanoid rig");
	createHumanoidButton.SetTooltip("Tries to create a humanoid component based on bone naming convention.\nIt supports the VRM and Mixamo naming convention.");
	createHumanoidButton.SetSize(XMFLOAT2(wid, hei));
	createHumanoidButton.OnClick([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		if (scene.humanoids.Contains(entity))
		{
			wi::helper::messageBox("Humanoid Component already exists!");
			return;
		}

		const ArmatureComponent* armature = scene.armatures.GetComponent(entity);
		if (armature == nullptr)
			return;

		static const wi::unordered_map<HumanoidComponent::HumanoidBone, wi::vector<std::string>> mapping = {
			{HumanoidComponent::HumanoidBone::Hips, {"Hips"}},
			{HumanoidComponent::HumanoidBone::Spine, {"Spine"}},
			{HumanoidComponent::HumanoidBone::Chest, {"Chest", "Spine1"}},
			{HumanoidComponent::HumanoidBone::UpperChest, {"UpperChest", "Spine2"}},
			{HumanoidComponent::HumanoidBone::Neck, {"Neck"}},

			{HumanoidComponent::HumanoidBone::Head, {"Head"}},
			{HumanoidComponent::HumanoidBone::LeftEye, {"LeftEye"}},
			{HumanoidComponent::HumanoidBone::RightEye, {"RightEye"}},
			{HumanoidComponent::HumanoidBone::Jaw, {"Jaw"}},

			{HumanoidComponent::HumanoidBone::LeftUpperLeg, {"LeftUpperLeg", "LeftUpLeg"}},
			{HumanoidComponent::HumanoidBone::LeftLowerLeg, {"LeftLowerLeg", "LeftLeg"}},
			{HumanoidComponent::HumanoidBone::LeftFoot, {"LeftFoot"}},
			{HumanoidComponent::HumanoidBone::LeftToes, {"LeftToe"}},
			{HumanoidComponent::HumanoidBone::RightUpperLeg, {"RightUpperLeg", "RightUpLeg"}},
			{HumanoidComponent::HumanoidBone::RightLowerLeg, {"RightLowerLeg", "RightLeg"}},
			{HumanoidComponent::HumanoidBone::RightFoot, {"RightFoot"}},
			{HumanoidComponent::HumanoidBone::RightToes, {"RightToe"}},

			{HumanoidComponent::HumanoidBone::LeftShoulder, {"LeftShoulder"}},
			{HumanoidComponent::HumanoidBone::LeftUpperArm, {"LeftUpperArm", "LeftArm"}},
			{HumanoidComponent::HumanoidBone::LeftLowerArm, {"LeftLowerArm", "LeftForeArm"}},
			{HumanoidComponent::HumanoidBone::LeftHand, {"LeftHand"}},
			{HumanoidComponent::HumanoidBone::RightShoulder, {"RightShoulder"}},
			{HumanoidComponent::HumanoidBone::RightUpperArm, {"RightUpperArm", "RightArm"}},
			{HumanoidComponent::HumanoidBone::RightLowerArm, {"RightLowerArm", "RightForeArm"}},
			{HumanoidComponent::HumanoidBone::RightHand, {"RightHand"}},

			{HumanoidComponent::HumanoidBone::LeftThumbMetacarpal, {"LeftThumbMetacarpal", "LeftHandThumb1"}},
			{HumanoidComponent::HumanoidBone::LeftThumbProximal, {"LeftThumbProximal", "LeftHandThumb2"}},
			{HumanoidComponent::HumanoidBone::LeftThumbDistal, {"LeftThumbDistal", "LeftHandThumb3"}},
			{HumanoidComponent::HumanoidBone::LeftIndexProximal, {"LeftIndexProximal", "LeftHandIndex1"}},
			{HumanoidComponent::HumanoidBone::LeftIndexIntermediate, {"LeftIndexIntermediate", "LeftHandIndex2"}},
			{HumanoidComponent::HumanoidBone::LeftIndexDistal, {"LeftIndexDistal", "LeftHandIndex3"}},
			{HumanoidComponent::HumanoidBone::LeftMiddleProximal, {"LeftMiddleProximal", "LeftHandMiddle1"}},
			{HumanoidComponent::HumanoidBone::LeftMiddleIntermediate, {"LeftMiddleIntermediate", "LeftHandMiddle2"}},
			{HumanoidComponent::HumanoidBone::LeftMiddleDistal, {"LeftMiddleDistal", "LeftHandMiddle3"}},
			{HumanoidComponent::HumanoidBone::LeftRingProximal, {"LeftRingProximal", "LeftHandRing1"}},
			{HumanoidComponent::HumanoidBone::LeftRingIntermediate, {"LeftRingIntermediate", "LeftHandRing2"}},
			{HumanoidComponent::HumanoidBone::LeftRingDistal, {"LeftRingDistal", "LeftHandRing3"}},
			{HumanoidComponent::HumanoidBone::LeftLittleProximal, {"LeftLittleProximal", "LeftHandPinky1"}},
			{HumanoidComponent::HumanoidBone::LeftLittleIntermediate, {"LeftLittleIntermediate", "LeftHandPinky2"}},
			{HumanoidComponent::HumanoidBone::LeftLittleDistal, {"LeftLittleDistal", "LeftHandPinky3"}},
			{HumanoidComponent::HumanoidBone::RightThumbMetacarpal, {"RightThumbMetacarpal", "RightHandThumb1"}},
			{HumanoidComponent::HumanoidBone::RightThumbProximal, {"RightThumbProximal", "RightHandThumb2"}},
			{HumanoidComponent::HumanoidBone::RightThumbDistal, {"RightThumbDistal", "RightHandThumb3"}},
			{HumanoidComponent::HumanoidBone::RightIndexIntermediate, {"RightIndexIntermediate", "RightHandIndex2"}},
			{HumanoidComponent::HumanoidBone::RightIndexDistal, {"RightIndexDistal", "RightHandIndex3"}},
			{HumanoidComponent::HumanoidBone::RightIndexProximal, {"RightIndexProximal", "RightHandIndex1"}},
			{HumanoidComponent::HumanoidBone::RightMiddleProximal, {"RightMiddleProximal", "RightHandMiddle1"}},
			{HumanoidComponent::HumanoidBone::RightMiddleIntermediate, {"RightMiddleIntermediate", "RightHandMiddle2"}},
			{HumanoidComponent::HumanoidBone::RightMiddleDistal, {"RightMiddleDistal", "RightHandMiddle3"}},
			{HumanoidComponent::HumanoidBone::RightRingProximal, {"RightRingProximal", "RightHandRing1"}},
			{HumanoidComponent::HumanoidBone::RightRingIntermediate, {"RightRingIntermediate", "RightHandRing2"}},
			{HumanoidComponent::HumanoidBone::RightRingDistal, {"RightRingDistal", "RightHandRing3"}},
			{HumanoidComponent::HumanoidBone::RightLittleProximal, {"RightLittleProximal", "RightHandPinky1"}},
			{HumanoidComponent::HumanoidBone::RightLittleIntermediate, {"RightLittleIntermediate", "RightHandPinky2"}},
			{HumanoidComponent::HumanoidBone::RightLittleDistal, {"RightLittleDistal", "RightHandPinky3"}},
		};

		HumanoidComponent humanoid;
		bool found_anything = false;

		for (size_t i = 0; i < armature->boneCollection.size(); ++i)
		{
			Entity bone = armature->boneCollection[i];
			NameComponent* name = scene.names.GetComponent(bone);
			if (name == nullptr)
				continue;

			size_t iType = 0;
			for (auto& humanoidBone : humanoid.bones)
			{
				if (humanoidBone == INVALID_ENTITY)
				{
					HumanoidComponent::HumanoidBone type = (HumanoidComponent::HumanoidBone)iType;
					auto it = mapping.find(type);
					if (it != mapping.end())
					{
						for (auto& candidate : it->second)
						{
							if (wi::helper::toUpper(name->name).find(wi::helper::toUpper(candidate)) != std::string::npos)
							{
								humanoidBone = bone;
								found_anything = true;
								break;
							}
						}
					}
				}
				iType++;
			}
		}

		if (found_anything)
		{
			scene.humanoids.Create(entity) = humanoid;
			editor->componentsWnd.humanoidWnd.SetEntity(INVALID_ENTITY);
			editor->componentsWnd.humanoidWnd.SetEntity(entity);
		}
		else
		{
			wi::helper::messageBox("No matching humanoid bones found!");
		}
	});
	AddWidget(&createHumanoidButton);

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

		editor->componentsWnd.RefreshEntityTree();

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
	add_fullwidth(createHumanoidButton);

	y += jump;

	add_fullwidth(boneList);

}
