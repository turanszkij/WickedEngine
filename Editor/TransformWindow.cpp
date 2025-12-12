#include "stdafx.h"
#include "TransformWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void TransformWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_TRANSFORM " Transform" , wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(480, 280));

	closeButton.SetTooltip("Delete TransformComponent\nNote that a lot of components won't work correctly without a TransformComponent!");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().transforms.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
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

	moveToEditorCameraButton.Create("Move to Editor Camera");
	moveToEditorCameraButton.SetTooltip("Move the selected transform(s) to match the current editor camera view");
	moveToEditorCameraButton.SetPos(XMFLOAT2(x, y += step));
	moveToEditorCameraButton.SetSize(XMFLOAT2(wid + hei + 1, hei));
	moveToEditorCameraButton.OnClick([=](wi::gui::EventArgs args) {
		const Scene& scene = editor->GetCurrentScene();
		const auto& editorscene = editor->GetCurrentEditorScene();

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;

		for (auto& x : editor->translator.selected)
		{
			TransformComponent* transform = scene.transforms.GetComponent(x.entity);
			if (transform == nullptr)
				continue;

			editor->RecordEntity(archive, x.entity);

			*transform = editorscene.camera_transform;
			transform->SetDirty();
		}

		for (auto& x : editor->translator.selected)
		{
			editor->RecordEntity(archive, x.entity);
		}
	});
	AddWidget(&moveToEditorCameraButton);

	txInput.Create("");
	txInput.SetValue(0);
	txInput.SetDescription("Position: ");
	txInput.SetPos(XMFLOAT2(x, y += step));
	txInput.SetSize(XMFLOAT2(siz, hei));
	txInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	tyInput.SetPos(XMFLOAT2(x, y += step));
	tyInput.SetSize(XMFLOAT2(siz, hei));
	tyInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	tzInput.SetPos(XMFLOAT2(x, y += step));
	tzInput.SetSize(XMFLOAT2(siz, hei));
	tzInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	sxInput.SetDescription("Scale: ");
	sxInput.SetPos(XMFLOAT2(x, y));
	sxInput.SetSize(XMFLOAT2(siz, hei));
	sxInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	syInput.SetPos(XMFLOAT2(x, y += step));
	syInput.SetSize(XMFLOAT2(siz, hei));
	syInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	szInput.SetPos(XMFLOAT2(x, y += step));
	szInput.SetSize(XMFLOAT2(siz, hei));
	szInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&szInput);

	suInput.Create("");
	suInput.SetDescription("Uniform Scale: ");
	suInput.SetValue(1);
	suInput.SetPos(XMFLOAT2(x, y += step));
	suInput.SetSize(XMFLOAT2(siz, hei));
	suInput.OnInputAccepted([this](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.x = args.fValue;
			transform->scale_local.y = args.fValue;
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&suInput);

	x = xx;
	y = step * 4;


	rollInput.Create("");
	rollInput.SetValue(0);
	rollInput.SetDescription("Rotation: ");
	rollInput.SetTooltip("Roll (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	rollInput.SetPos(XMFLOAT2(x, y += step));
	rollInput.SetSize(XMFLOAT2(siz, hei));
	rollInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	pitchInput.SetTooltip("Pitch (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	pitchInput.SetPos(XMFLOAT2(x, y += step));
	pitchInput.SetSize(XMFLOAT2(siz, hei));
	pitchInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	yawInput.SetTooltip("Yaw (in degrees)\n Note: Euler angle rotations can result in precision loss from quaternion conversion!");
	yawInput.SetPos(XMFLOAT2(x, y += step));
	yawInput.SetSize(XMFLOAT2(siz, hei));
	yawInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	rxInput.SetDescription("Quaternion: ");
	rxInput.SetTooltip("Rotation Quaternion.X");
	rxInput.SetPos(XMFLOAT2(x, y += step));
	rxInput.SetSize(XMFLOAT2(siz, hei));
	rxInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	ryInput.SetTooltip("Rotation Quaternion.Y");
	ryInput.SetPos(XMFLOAT2(x, y += step));
	ryInput.SetSize(XMFLOAT2(siz, hei));
	ryInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	rzInput.SetTooltip("Rotation Quaternion.Z");
	rzInput.SetPos(XMFLOAT2(x, y += step));
	rzInput.SetSize(XMFLOAT2(siz, hei));
	rzInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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
	rwInput.SetTooltip("Rotation Quaternion.W");
	rwInput.SetPos(XMFLOAT2(x, y += step));
	rwInput.SetSize(XMFLOAT2(siz, hei));
	rwInput.OnInputAccepted([this](wi::gui::EventArgs args) {
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

	resetTranslationButton.Create("ResetTranslation");
	resetTranslationButton.SetText("X");
	resetTranslationButton.SetTooltip("Reset translation");
	resetTranslationButton.SetSize(XMFLOAT2(hei, hei));
	resetTranslationButton.OnClick([=](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local = XMFLOAT3(0, 0, 0);
			transform->SetDirty();
		}
		});
	AddWidget(&resetTranslationButton);

	resetScaleButton.Create("ResetScale");
	resetScaleButton.SetText("X");
	resetScaleButton.SetTooltip("Reset scale");
	resetScaleButton.SetSize(XMFLOAT2(hei, hei));
	resetScaleButton.OnClick([=](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local = XMFLOAT3(1, 1, 1);
			transform->SetDirty();
		}
		});
	AddWidget(&resetScaleButton);

	resetScaleUniformButton.Create("ResetScaleUniform");
	resetScaleUniformButton.SetText("X");
	resetScaleUniformButton.SetTooltip("Reset scale");
	resetScaleUniformButton.SetSize(XMFLOAT2(hei, hei));
	resetScaleUniformButton.OnClick([=](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local = XMFLOAT3(1, 1, 1);
			transform->SetDirty();
		}
		});
	AddWidget(&resetScaleUniformButton);

	resetRotationButton.Create("ResetRotation");
	resetRotationButton.SetText("X");
	resetRotationButton.SetTooltip("Reset rotation");
	resetRotationButton.SetSize(XMFLOAT2(hei, hei));
	resetRotationButton.OnClick([=](wi::gui::EventArgs args) {
		TransformComponent* transform = editor->GetCurrentScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local = XMFLOAT4(0, 0, 0, 1);
			transform->SetDirty();
		}
		});
	AddWidget(&resetRotationButton);

	applyScaleToObjectButton.Create("Apply Scale to Object");
	applyScaleToObjectButton.SetTooltip("Recomputes the vertex positions of child object meshes, morph target positions, and armature bones to apply the current transform scale");
	applyScaleToObjectButton.SetSize(XMFLOAT2(wid + hei + 1, hei));
	applyScaleToObjectButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (transform == nullptr)
			return;

		const XMFLOAT3& scale = transform->scale_local;
		if (scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f)
			return;

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		wi::vector<Entity> entities_to_process;
		entities_to_process.push_back(entity);
		scene.GetChildren(entity, entities_to_process);

		wi::unordered_set<Entity> processed_meshes;
		wi::unordered_set<Entity> processed_armatures;

		for (Entity e : entities_to_process)
		{
			// Process ObjectComponent meshes
			ObjectComponent* object = scene.objects.GetComponent(e);
			if (object != nullptr && object->meshID != INVALID_ENTITY)
			{
				if (processed_meshes.count(object->meshID) == 0)
				{
					processed_meshes.insert(object->meshID);

					MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
					if (mesh != nullptr)
					{
						editor->RecordEntity(archive, object->meshID);

						// Apply scale to all vertex positions
						for (XMFLOAT3& pos : mesh->vertex_positions)
						{
							pos.x *= scale.x;
							pos.y *= scale.y;
							pos.z *= scale.z;
						}

						// Apply scale to morph target positions
						for (MeshComponent::MorphTarget& morph : mesh->morph_targets)
						{
							for (XMFLOAT3& pos : morph.vertex_positions)
							{
								pos.x *= scale.x;
								pos.y *= scale.y;
								pos.z *= scale.z;
							}
						}

						// Process the armature associated with this mesh
						if (mesh->armatureID != INVALID_ENTITY && processed_armatures.count(mesh->armatureID) == 0)
						{
							processed_armatures.insert(mesh->armatureID);

							ArmatureComponent* armature = scene.armatures.GetComponent(mesh->armatureID);
							if (armature != nullptr)
							{
								editor->RecordEntity(archive, mesh->armatureID);

								// Apply scale to inverse bind matrices
								for (XMFLOAT4X4& bind : armature->inverseBindMatrices)
								{
									XMVECTOR V_S, V_R, V_T;
									XMMatrixDecompose(&V_S, &V_R, &V_T, XMLoadFloat4x4(&bind));
									XMFLOAT3 pos, bindScale;
									XMFLOAT4 rot;
									XMStoreFloat3(&bindScale, V_S);
									XMStoreFloat3(&pos, V_T);
									XMStoreFloat4(&rot, V_R);

									// Scale the translation component
									pos.x *= scale.x;
									pos.y *= scale.y;
									pos.z *= scale.z;

									XMMATRIX newBind =
										XMMatrixScalingFromVector(XMLoadFloat3(&bindScale)) *
										XMMatrixRotationQuaternion(XMLoadFloat4(&rot)) *
										XMMatrixTranslationFromVector(XMLoadFloat3(&pos));
									XMStoreFloat4x4(&bind, newBind);
								}

								// Apply scale to bone transforms
								for (Entity boneEntity : armature->boneCollection)
								{
									TransformComponent* boneTransform = scene.transforms.GetComponent(boneEntity);
									if (boneTransform != nullptr)
									{
										editor->RecordEntity(archive, boneEntity);

										boneTransform->translation_local.x *= scale.x;
										boneTransform->translation_local.y *= scale.y;
										boneTransform->translation_local.z *= scale.z;
										boneTransform->SetDirty();

										editor->RecordEntity(archive, boneEntity);
									}
								}

								editor->RecordEntity(archive, mesh->armatureID);
							}
						}

						// Recreate render data for the modified mesh
						mesh->CreateRenderData();
						if (!mesh->BLASes.empty())
						{
							mesh->CreateRaytracingRenderData();
						}

						editor->RecordEntity(archive, object->meshID);
					}
				}
			}
		}

		transform->scale_local = XMFLOAT3(1, 1, 1);
		transform->SetDirty();

		editor->RecordEntity(archive, entity);
	});
	AddWidget(&applyScaleToObjectButton);


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
		suInput.SetValue(transform->scale_local.x);

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}
}

void TransformWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 80;

	layout.add_fullwidth(clearButton);
	layout.add_fullwidth(moveToEditorCameraButton);
	layout.add_fullwidth(applyScaleToObjectButton);

	float safe_width = layout.width - 100 - 20 - layout.padding;
	txInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	tyInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	tzInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));

	sxInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	syInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	szInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	suInput.SetSize(XMFLOAT2(safe_width - layout.padding, txInput.GetSize().y));

	rollInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	pitchInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));
	yawInput.SetSize(XMFLOAT2(safe_width / 3.0f - layout.padding, txInput.GetSize().y));

	safe_width = layout.width - 100;
	rxInput.SetSize(XMFLOAT2(safe_width / 4.0f - layout.padding, txInput.GetSize().y));
	ryInput.SetSize(XMFLOAT2(safe_width / 4.0f - layout.padding, txInput.GetSize().y));
	rzInput.SetSize(XMFLOAT2(safe_width / 4.0f - layout.padding, txInput.GetSize().y));
	rwInput.SetSize(XMFLOAT2(safe_width / 4.0f - layout.padding, txInput.GetSize().y));

	layout.add_right(resetTranslationButton);
	tzInput.SetPos(XMFLOAT2(resetTranslationButton.GetPos().x - tzInput.GetSize().x - layout.padding, resetTranslationButton.GetPos().y));
	tyInput.SetPos(XMFLOAT2(tzInput.GetPos().x - tyInput.GetSize().x - layout.padding, tzInput.GetPos().y));
	txInput.SetPos(XMFLOAT2(tyInput.GetPos().x - txInput.GetSize().x - layout.padding, tyInput.GetPos().y));
	layout.add_right(resetScaleButton);
	szInput.SetPos(XMFLOAT2(resetScaleButton.GetPos().x - szInput.GetSize().x - layout.padding, resetScaleButton.GetPos().y));
	syInput.SetPos(XMFLOAT2(szInput.GetPos().x - syInput.GetSize().x - layout.padding, szInput.GetPos().y));
	sxInput.SetPos(XMFLOAT2(syInput.GetPos().x - sxInput.GetSize().x - layout.padding, syInput.GetPos().y));
	layout.add_right(resetScaleUniformButton);
	suInput.SetPos(XMFLOAT2(resetScaleUniformButton.GetPos().x - suInput.GetSize().x - layout.padding, resetScaleUniformButton.GetPos().y));
	layout.add_right(resetRotationButton);
	yawInput.SetPos(XMFLOAT2(resetRotationButton.GetPos().x - yawInput.GetSize().x - layout.padding, resetRotationButton.GetPos().y));
	pitchInput.SetPos(XMFLOAT2(yawInput.GetPos().x - pitchInput.GetSize().x - layout.padding, yawInput.GetPos().y));
	rollInput.SetPos(XMFLOAT2(pitchInput.GetPos().x - rollInput.GetSize().x - layout.padding, pitchInput.GetPos().y));
	layout.add_right(rwInput);
	rzInput.SetPos(XMFLOAT2(rwInput.GetPos().x - rzInput.GetSize().x - layout.padding, rwInput.GetPos().y));
	ryInput.SetPos(XMFLOAT2(rzInput.GetPos().x - ryInput.GetSize().x - layout.padding, rzInput.GetPos().y));
	rxInput.SetPos(XMFLOAT2(ryInput.GetPos().x - rxInput.GetSize().x - layout.padding, ryInput.GetPos().y));

	layout.jump();

	layout.add_right(snapScaleInput);
	layout.add_right(snapRotateInput);
	layout.add_right(snapTranslateInput);
}
