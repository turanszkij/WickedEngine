#include "stdafx.h"
#include "TransformWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void TransformWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Transform", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(480, 200));

	float x = 80;
	float xx = x;
	float y = 0;
	float step = 25;
	float siz = 50;
	float hei = 20;
	float wid = 200;

	clearButton.Create("Clear Transform");
	clearButton.SetTooltip("Reset transform to identity");
	clearButton.SetPos(XMFLOAT2(x, y));
	clearButton.SetSize(XMFLOAT2(wid + hei + 1, hei));
	clearButton.OnClick([=](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			transform->ClearTransform();
			transform->UpdateTransform();

			editor->RecordEntity(archive, entity);
		}
		});
	AddWidget(&clearButton);

	parentCombo.Create("Parent: ");
	parentCombo.SetSize(XMFLOAT2(wid, hei));
	parentCombo.SetPos(XMFLOAT2(x, y += step));
	parentCombo.SetEnabled(false);
	parentCombo.OnSelect([&](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		Scene& scene = editor->GetCurrentScene();
		if (args.iValue == 0)
		{
			scene.Component_Detach(entity);
		}
		else
		{
		    scene.Component_Attach(entity, (Entity)args.userdata);
		}

		editor->RecordEntity(archive, entity);

	});
	parentCombo.SetTooltip("Choose a parent entity (also works if selected entity has no transform)");
	AddWidget(&parentCombo);

	txInput.Create("");
	txInput.SetValue(0);
	txInput.SetDescription("Position X: ");
	txInput.SetPos(XMFLOAT2(x, y += step));
	txInput.SetSize(XMFLOAT2(siz, hei));
	txInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.x = args.fValue;
			transform->SetDirty();
		}
	});
	AddWidget(&txInput);

	tyInput.Create("");
	tyInput.SetValue(0);
	tyInput.SetDescription("Position Y: ");
	tyInput.SetPos(XMFLOAT2(x, y += step));
	tyInput.SetSize(XMFLOAT2(siz, hei));
	tyInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&tyInput);

	tzInput.Create("");
	tzInput.SetValue(0);
	tzInput.SetDescription("Position Z: ");
	tzInput.SetPos(XMFLOAT2(x, y += step));
	tzInput.SetSize(XMFLOAT2(siz, hei));
	tzInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&tzInput);

	x = 250;
	y = step;

	sxInput.Create("");
	sxInput.SetValue(1);
	sxInput.SetDescription("Scale X: ");
	sxInput.SetPos(XMFLOAT2(x, y += step));
	sxInput.SetSize(XMFLOAT2(siz, hei));
	sxInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.x = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&sxInput);

	syInput.Create("");
	syInput.SetValue(1);
	syInput.SetDescription("Scale Y: ");
	syInput.SetPos(XMFLOAT2(x, y += step));
	syInput.SetSize(XMFLOAT2(siz, hei));
	syInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&syInput);

	szInput.Create("");
	szInput.SetValue(1);
	szInput.SetDescription("Scale Z: ");
	szInput.SetPos(XMFLOAT2(x, y += step));
	szInput.SetSize(XMFLOAT2(siz, hei));
	szInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&szInput);

	x = xx;
	y = step * 4.5f;


	rollInput.Create("");
	rollInput.SetValue(0);
	rollInput.SetDescription("Rotation X: ");
	rollInput.SetTooltip("Roll (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	rollInput.SetPos(XMFLOAT2(x, y += step));
	rollInput.SetSize(XMFLOAT2(siz, hei));
	rollInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			float roll = float(std::atof(rollInput.GetValue().c_str())) / 180.0f * XM_PI;
			float pitch = float(std::atof(pitchInput.GetValue().c_str())) / 180.0f * XM_PI;
			float yaw = float(std::atof(yawInput.GetValue().c_str())) / 180.0f * XM_PI;
			XMVECTOR Q = XMQuaternionRotationRollPitchYaw(roll, pitch, yaw);
			Q = XMQuaternionNormalize(Q);
			XMStoreFloat4(&transform->rotation_local, Q);
			transform->SetDirty();
		}
		});
	AddWidget(&rollInput);

	pitchInput.Create("");
	pitchInput.SetValue(0);
	pitchInput.SetDescription("Rotation Y: ");
	pitchInput.SetTooltip("Pitch (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	pitchInput.SetPos(XMFLOAT2(x, y += step));
	pitchInput.SetSize(XMFLOAT2(siz, hei));
	pitchInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			float roll = float(std::atof(rollInput.GetValue().c_str())) / 180.0f * XM_PI;
			float pitch = float(std::atof(pitchInput.GetValue().c_str())) / 180.0f * XM_PI;
			float yaw = float(std::atof(yawInput.GetValue().c_str())) / 180.0f * XM_PI;
			XMVECTOR Q = XMQuaternionRotationRollPitchYaw(roll, pitch, yaw);
			Q = XMQuaternionNormalize(Q);
			XMStoreFloat4(&transform->rotation_local, Q);
			transform->SetDirty();
		}
		});
	AddWidget(&pitchInput);

	yawInput.Create("");
	yawInput.SetValue(0);
	yawInput.SetDescription("Rotation Z: ");
	yawInput.SetTooltip("Yaw (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	yawInput.SetPos(XMFLOAT2(x, y += step));
	yawInput.SetSize(XMFLOAT2(siz, hei));
	yawInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			float roll = float(std::atof(rollInput.GetValue().c_str())) / 180.0f * XM_PI;
			float pitch = float(std::atof(pitchInput.GetValue().c_str())) / 180.0f * XM_PI;
			float yaw = float(std::atof(yawInput.GetValue().c_str())) / 180.0f * XM_PI;
			XMVECTOR Q = XMQuaternionRotationRollPitchYaw(roll, pitch, yaw);
			Q = XMQuaternionNormalize(Q);
			XMStoreFloat4(&transform->rotation_local, Q);
			transform->SetDirty();
		}
		});
	AddWidget(&yawInput);

	x = 250;
	y = step * 4.5f;

	rxInput.Create("");
	rxInput.SetValue(0);
	rxInput.SetDescription("Quaternion X: ");
	rxInput.SetTooltip("Rotation Quaternion.X");
	rxInput.SetPos(XMFLOAT2(x, y += step));
	rxInput.SetSize(XMFLOAT2(siz, hei));
	rxInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.x = args.fValue;
			XMStoreFloat4(&transform->rotation_local, XMLoadFloat4(&transform->rotation_local));
			transform->SetDirty();
		}
		});
	AddWidget(&rxInput);

	ryInput.Create("");
	ryInput.SetValue(0);
	ryInput.SetDescription("Quaternion Y: ");
	ryInput.SetTooltip("Rotation Quaternion.Y");
	ryInput.SetPos(XMFLOAT2(x, y += step));
	ryInput.SetSize(XMFLOAT2(siz, hei));
	ryInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.y = args.fValue;
			XMStoreFloat4(&transform->rotation_local, XMLoadFloat4(&transform->rotation_local));
			transform->SetDirty();
		}
		});
	AddWidget(&ryInput);

	rzInput.Create("");
	rzInput.SetValue(0);
	rzInput.SetDescription("Quaternion Z: ");
	rzInput.SetTooltip("Rotation Quaternion.Z");
	rzInput.SetPos(XMFLOAT2(x, y += step));
	rzInput.SetSize(XMFLOAT2(siz, hei));
	rzInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.z = args.fValue;
			XMStoreFloat4(&transform->rotation_local, XMLoadFloat4(&transform->rotation_local));
			transform->SetDirty();
		}
		});
	AddWidget(&rzInput);

	rwInput.Create("");
	rwInput.SetValue(1);
	rwInput.SetDescription("Quaternion W: ");
	rwInput.SetTooltip("Rotation Quaternion.W");
	rwInput.SetPos(XMFLOAT2(x, y += step));
	rwInput.SetSize(XMFLOAT2(siz, hei));
	rwInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.w = args.fValue;
			XMStoreFloat4(&transform->rotation_local, XMLoadFloat4(&transform->rotation_local));
			transform->SetDirty();
		}
		});
	AddWidget(&rwInput);


	x = xx;
	y += step * 0.5f;

	ikButton.Create("Create IK");
	ikButton.SetTooltip("Create/Remove an Inverse Kinematics Component for this entity");
	ikButton.SetPos(XMFLOAT2(x, y += step));
	ikButton.SetSize(XMFLOAT2(wid + hei + 1, hei));
	ikButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		if (scene.inverse_kinematics.Contains(entity))
		{
			scene.inverse_kinematics.Remove(entity);
			ikButton.SetText("Create IK");
		}
		else
		{
			scene.inverse_kinematics.Create(entity);
			ikButton.SetText("Remove IK");
		}
		});
	AddWidget(&ikButton);

	springButton.Create("Create Spring");
	springButton.SetTooltip("Create/Remove a Spring Component for this entity");
	springButton.SetPos(XMFLOAT2(x, y += step));
	springButton.SetSize(XMFLOAT2(wid + hei + 1, hei));
	springButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		if (scene.springs.Contains(entity))
		{
			scene.springs.Remove(entity);
			springButton.SetText("Create Spring");
		}
		else
		{
			scene.springs.Create(entity);
			springButton.SetText("Remove Spring");
		}
		});
	AddWidget(&springButton);

	x = 250;

	snapScaleInput.Create("");
	snapScaleInput.SetValue(1);
	snapScaleInput.SetDescription("Snap mode Scale: ");
	snapScaleInput.SetPos(XMFLOAT2(x, y += step));
	snapScaleInput.SetSize(XMFLOAT2(siz, hei));
	snapScaleInput.SetValue(editor->translator.scale_snap);
	snapScaleInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		editor->translator.scale_snap = args.fValue;
		});
	AddWidget(&snapScaleInput);

	snapRotateInput.Create("");
	snapRotateInput.SetValue(1);
	snapRotateInput.SetDescription("Snap mode Rotate (in degrees): ");
	snapRotateInput.SetPos(XMFLOAT2(x, y += step));
	snapRotateInput.SetSize(XMFLOAT2(siz, hei));
	snapRotateInput.SetValue(editor->translator.rotate_snap / XM_PI * 180);
	snapRotateInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		editor->translator.rotate_snap = args.fValue / 180.0f * XM_PI;
		});
	AddWidget(&snapRotateInput);

	snapTranslateInput.Create("");
	snapTranslateInput.SetValue(1);
	snapTranslateInput.SetDescription("Snap mode Translate: ");
	snapTranslateInput.SetPos(XMFLOAT2(x, y += step));
	snapTranslateInput.SetSize(XMFLOAT2(siz, hei));
	snapTranslateInput.SetValue(editor->translator.translate_snap);
	snapTranslateInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		editor->translator.translate_snap = args.fValue;
		});
	AddWidget(&snapTranslateInput);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void TransformWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const TransformComponent* transform = scene.transforms.GetComponent(entity);

	if (transform != nullptr)
	{

		txInput.SetValue(transform->translation_local.x);
		tyInput.SetValue(transform->translation_local.y);
		tzInput.SetValue(transform->translation_local.z);

		XMFLOAT3 roll_pitch_yaw = wi::math::QuaternionToRollPitchYaw(transform->rotation_local);
		rollInput.SetValue(roll_pitch_yaw.x / XM_PI * 180.0f);
		pitchInput.SetValue(roll_pitch_yaw.y / XM_PI * 180.0f);
		yawInput.SetValue(roll_pitch_yaw.z / XM_PI * 180.0f);

		rxInput.SetValue(transform->rotation_local.x);
		ryInput.SetValue(transform->rotation_local.y);
		rzInput.SetValue(transform->rotation_local.z);
		rwInput.SetValue(transform->rotation_local.w);

		sxInput.SetValue(transform->scale_local.x);
		syInput.SetValue(transform->scale_local.y);
		szInput.SetValue(transform->scale_local.z);

		if (scene.inverse_kinematics.Contains(entity))
		{
			ikButton.SetText("Remove IK");
		}
		else
		{
			ikButton.SetText("Create IK");
		}

		if (scene.springs.Contains(entity))
		{
			springButton.SetText("Remove Spring");
		}
		else
		{
			springButton.SetText("Create Spring");
		}

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}

	parentCombo.SetEnabled(true);
	parentCombo.ClearItems();
	parentCombo.AddItem("NO PARENT");
	HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
	for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
	{
		Entity candidate_parent_entity = scene.transforms.GetEntity(i);
		if (candidate_parent_entity == entity)
		{
			continue; // Don't list selected (don't allow attach to self)
		}

		bool loop = false;
		for (size_t j = 0; j < scene.hierarchy.GetCount(); ++j)
		{
			if (scene.hierarchy[j].parentID == entity)
			{
				loop = true;
				break;
			}
		}
		if (loop)
		{
			continue;
		}

		const NameComponent* name = scene.names.GetComponent(candidate_parent_entity);
		parentCombo.AddItem(name == nullptr ? std::to_string(candidate_parent_entity) : name->name, candidate_parent_entity);

		if (hier != nullptr && hier->parentID == candidate_parent_entity)
		{
			parentCombo.SetSelectedWithoutCallback((int)parentCombo.GetItemCount() - 1);
		}
	}
}
