#include "stdafx.h"
#include "TransformWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void TransformWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_TRANSFORM " Transform" , wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(480, 360));

	closeButton.SetTooltip("Delete TransformComponent\nNote that a lot of components won't work correctly without a TransformComponent!");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().transforms.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 80;
	float xx = x;
	float y = 4;
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
	y = 4 + step;

	sxInput.Create("");
	sxInput.SetValue(1);
	sxInput.SetDescription("Scale X: ");
	sxInput.SetPos(XMFLOAT2(x, y));
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
	y = step * 4;


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
	y = step * 4;

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

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}
}
