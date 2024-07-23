#include "stdafx.h"
#include "AnimationWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void AnimationWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_ANIMATION " Animation", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(520, 540));

	closeButton.SetTooltip("Delete AnimationComponent");
	OnClose([=](wi::gui::EventArgs args) {
		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().animations.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 80;
	float y = 0;
	float hei = 18;
	float wid = 200;
	float step = hei + 4;
	float padding = 4;
	float lastDirection = 1;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(100, 50));
	infoLabel.SetText("The animation window will stay open even if you select other entities until it is collapsed, so you can record other entities' data.");
	AddWidget(&infoLabel);

	modeComboBox.Create("Sampling: ");
	modeComboBox.SetSize(XMFLOAT2(wid, hei));
	modeComboBox.SetPos(XMFLOAT2(x, y));
	modeComboBox.SetEnabled(false);
	modeComboBox.AddItem("Step", AnimationComponent::AnimationSampler::Mode::STEP);
	modeComboBox.AddItem("Linear", AnimationComponent::AnimationSampler::Mode::LINEAR);
	modeComboBox.AddItem("Cubic spline", AnimationComponent::AnimationSampler::Mode::CUBICSPLINE);
	modeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			for (const AnimationComponent::AnimationChannel& channel : animation->channels)
			{
				assert(channel.samplerIndex < (int)animation->samplers.size());
				AnimationComponent::AnimationSampler& sampler = animation->samplers[channel.samplerIndex];
				sampler.mode = (AnimationComponent::AnimationSampler::Mode)args.userdata;

				if (sampler.mode == AnimationComponent::AnimationSampler::Mode::CUBICSPLINE)
				{
					const AnimationDataComponent* animationdata = editor->GetCurrentScene().animation_datas.GetComponent(sampler.data);
					if (animationdata == nullptr)
					{
						sampler.mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
					}
					else if (animationdata->keyframe_data.size() != animationdata->keyframe_times.size() * 3 * 3)
					{
						sampler.mode = AnimationComponent::AnimationSampler::Mode::LINEAR;
					}
				}

			}
		}
	});
	modeComboBox.SetTooltip("Choose how animation data is interpreted between keyframes.\nNote that Cubic spline sampling requires spline animation data, otherwise, it will fall back to Linear!\nCurrently spline animation data creation is not supported, but it can be imported from GLTF/VRM models.");
	AddWidget(&modeComboBox);

	loopTypeButton.Create("LoopType");
	loopTypeButton.SetText(ICON_LOOP);
	loopTypeButton.SetDescription("Loop type: ");
	loopTypeButton.SetSize(XMFLOAT2(hei, hei));
	loopTypeButton.SetPos(XMFLOAT2(x, y += step));
	loopTypeButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->IsLooped())
			{
				animation->SetPingPong();
			}
			else if (animation->IsPingPong())
			{
				animation->SetPlayOnce();
			}
			else
			{
				animation->SetLooped();
			}
		}
	});
	AddWidget(&loopTypeButton);

	backwardsButton.Create("Backwards");
	backwardsButton.SetText(ICON_PLAY);
	backwardsButton.font.params.enableFlipHorizontally();
	backwardsButton.SetTooltip("Play the animation backwards from the current position.");
	backwardsButton.SetSize(XMFLOAT2(30, hei));
	backwardsButton.SetPos(XMFLOAT2(loopTypeButton.GetPos().x + loopTypeButton.GetSize().x + padding, y));
	backwardsButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->timer > animation->start) // Only play backwards if it is not in the start position.
			{
				animation->speed = -speedSlider.GetValue();
				animation->Play();
			}
		}
	});
	AddWidget(&backwardsButton);

	backwardsFromEndButton.Create("BackwardsFromEnd");
	backwardsFromEndButton.SetText(ICON_BACKWARD);
	backwardsFromEndButton.SetTooltip("Play the animation backwards starting from the end.");
	backwardsFromEndButton.SetSize(backwardsButton.GetSize());
	backwardsFromEndButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x + padding, y));
	backwardsFromEndButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = animation->end;
			animation->speed = -speedSlider.GetValue();
			animation->Play();
		}
	});
	AddWidget(&backwardsFromEndButton);

	stopButton.Create("Stop");
	stopButton.SetText(ICON_STOP);
	stopButton.SetTooltip("Stop.");
	stopButton.SetSize(backwardsButton.GetSize());
	stopButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 2 + padding * 2, y));
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->IsPlaying())
			{
				animation->Pause();
			}
			else
			{
				animation->Stop();
			}
		}
	});
	AddWidget(&stopButton);

	playFromStartButton.Create("PlayFromStart");
	playFromStartButton.SetText(ICON_FORWARD);
	playFromStartButton.SetTooltip("Play the animation from the beginning.");
	playFromStartButton.SetSize(backwardsButton.GetSize());
	playFromStartButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 3 + padding * 3, y));
	playFromStartButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = animation->start;
			animation->speed = speedSlider.GetValue();
			animation->Play();

		}
	});
	AddWidget(&playFromStartButton);

	playButton.Create("Play");
	playButton.SetText(ICON_PLAY);
	playButton.SetTooltip("Play the animation from the current position.");
	playButton.SetSize(backwardsButton.GetSize());
	playButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 4 + padding * 4, y));
	playButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->timer < animation->end) // Only play forward if it is not in the end position.
			{
				animation->speed = speedSlider.GetValue();
				animation->Play();
			}
		}
	});
	AddWidget(&playButton);

	timerSlider.Create(0, 1, 0, 100000, "Timer: ");
	timerSlider.SetSize(XMFLOAT2(wid, hei));
	timerSlider.SetPos(XMFLOAT2(x, y += step));
	timerSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = args.fValue;
		}
	});
	timerSlider.SetEnabled(false);
	timerSlider.SetTooltip("Set the animation timer by hand.");
	AddWidget(&timerSlider);

	amountSlider.Create(0, 1, 1, 100000, "Amount: ");
	amountSlider.SetSize(XMFLOAT2(wid, hei));
	amountSlider.SetPos(XMFLOAT2(x, y += step));
	amountSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->amount = args.fValue;
		}
	});
	amountSlider.SetEnabled(false);
	amountSlider.SetTooltip("Set the animation blending amount by hand.");
	AddWidget(&amountSlider);

	speedSlider.Create(0, 4, 1, 100000, "Speed: ");
	speedSlider.SetSize(XMFLOAT2(wid, hei));
	speedSlider.SetPos(XMFLOAT2(x, y += step));
	speedSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			// Adjusts the new animation speed based on the previous direction of animation->speed.
			lastDirection = std::signbit(animation->speed) ? -1 : 1;
			animation->speed = args.fValue * lastDirection;
		}
	});
	speedSlider.SetEnabled(false);
	speedSlider.SetTooltip("Set the animation speed.");
	AddWidget(&speedSlider);

	startInput.Create("Start");
	startInput.SetDescription("Start time: ");
	startInput.SetSize(XMFLOAT2(wid, hei));
	startInput.SetPos(XMFLOAT2(x, y += step));
	startInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->start = args.fValue;
		}
		});
	startInput.SetTooltip("Set the animation start in seconds. This will be the loop's starting point.");
	AddWidget(&startInput);

	endInput.Create("End");
	endInput.SetDescription("End time: ");
	endInput.SetSize(XMFLOAT2(wid, hei));
	endInput.SetPos(XMFLOAT2(x, y += step));
	endInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->end = args.fValue;
		}
		});
	endInput.SetTooltip("Set the animation end in seconds. This is relative to 0, not the animation start.");
	AddWidget(&endInput);


	recordCombo.Create("Record: ");
	recordCombo.selected_font.anim.typewriter.looped = true;
	recordCombo.selected_font.anim.typewriter.time = 2;
	recordCombo.selected_font.anim.typewriter.character_start = 1;
	recordCombo.SetSize(XMFLOAT2(wid, hei));
	recordCombo.SetPos(XMFLOAT2(x, y += step));
	recordCombo.SetInvalidSelectionText("...");
	recordCombo.AddItem("Transform " ICON_TRANSLATE " " ICON_ROTATE " " ICON_SCALE);
	recordCombo.AddItem("Position " ICON_TRANSLATE);
	recordCombo.AddItem("Rotation " ICON_ROTATE);
	recordCombo.AddItem("Scale " ICON_SCALE);
	recordCombo.AddItem("Morph weights " ICON_MESH);
	recordCombo.AddItem("Light [color] " ICON_POINTLIGHT);
	recordCombo.AddItem("Light [intensity] " ICON_POINTLIGHT);
	recordCombo.AddItem("Light [range] " ICON_POINTLIGHT);
	recordCombo.AddItem("Light [inner cone] " ICON_POINTLIGHT);
	recordCombo.AddItem("Light [outer cone] " ICON_POINTLIGHT);
	recordCombo.AddItem("Sound [play] " ICON_SOUND);
	recordCombo.AddItem("Sound [stop] " ICON_SOUND);
	recordCombo.AddItem("Sound [volume] " ICON_SOUND);
	recordCombo.AddItem("Emitter [emit count] " ICON_EMITTER);
	recordCombo.AddItem("Camera [fov] " ICON_CAMERA);
	recordCombo.AddItem("Camera [focal length] " ICON_CAMERA);
	recordCombo.AddItem("Camera [aperture size] " ICON_CAMERA);
	recordCombo.AddItem("Camera [aperture shape] " ICON_CAMERA);
	recordCombo.AddItem("Script [play] " ICON_SCRIPT);
	recordCombo.AddItem("Script [stop] " ICON_SCRIPT);
	recordCombo.AddItem("Material [color] " ICON_MATERIAL);
	recordCombo.AddItem("Material [emissive] " ICON_MATERIAL);
	recordCombo.AddItem("Material [roughness] " ICON_MATERIAL);
	recordCombo.AddItem("Material [metalness] " ICON_MATERIAL);
	recordCombo.AddItem("Material [reflectance] " ICON_MATERIAL);
	recordCombo.AddItem("Material [texmuladd] " ICON_MATERIAL);
	recordCombo.AddItem("Close loop " ICON_LOOP, ~0ull);
	recordCombo.OnSelect([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();

		AnimationComponent* animation = scene.animations.GetComponent(entity);
		if (animation != nullptr)
		{
			const float current_time = animation->timer;

			if (args.userdata == ~0ull)
			{
				// Close loop:
				for (auto& channel : animation->channels)
				{
					auto& sam = animation->samplers[channel.samplerIndex];
					AnimationDataComponent* animation_data = scene.animation_datas.GetComponent(sam.data);
					if (animation_data != nullptr)
					{
						// Search for leftmost keyframe:
						int keyFirst = 0;
						float timeFirst = std::numeric_limits<float>::max();
						for (int k = 0; k < (int)animation_data->keyframe_times.size(); ++k)
						{
							const float time = animation_data->keyframe_times[k];
							if (time < timeFirst)
							{
								timeFirst = time;
								keyFirst = k;
							}
						}

						// Duplicate first frame to current position:
						animation_data->keyframe_times.push_back(current_time);

						const AnimationComponent::AnimationChannel::PathDataType path_data_type = channel.GetPathDataType();

						switch (path_data_type)
						{
						case AnimationComponent::AnimationChannel::PathDataType::Float:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst]);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float2:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 2 + 0]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 2 + 1]);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float3:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 0]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 1]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 2]);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Float4:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 0]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 1]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 2]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 3]);
						}
						break;
						case AnimationComponent::AnimationChannel::PathDataType::Weights:
						{
							const MeshComponent* mesh = scene.meshes.GetComponent(channel.target);
							if (mesh == nullptr && scene.objects.Contains(channel.target))
							{
								// Also try query mesh of selected object:
								ObjectComponent* object = scene.objects.GetComponent(channel.target);
								mesh = scene.meshes.GetComponent(object->meshID);
							}
							if (mesh != nullptr && !mesh->morph_targets.empty())
							{
								int idx = 0;
								for (const MeshComponent::MorphTarget& morph : mesh->morph_targets)
								{
									animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * mesh->morph_targets.size() + idx]);
									idx++;
								}
							}
						}
						break;
						default:
							break;
						}
					}
				}
			}
			else
			{
				// Add keyframe type:
				wi::vector<AnimationComponent::AnimationChannel::Path> paths; // stack allocation would be better here..

				switch (args.iValue)
				{
				default:
				case 0:
					paths.push_back(AnimationComponent::AnimationChannel::Path::TRANSLATION);
					paths.push_back(AnimationComponent::AnimationChannel::Path::ROTATION);
					paths.push_back(AnimationComponent::AnimationChannel::Path::SCALE);
					break;
				case 1:
					paths.push_back(AnimationComponent::AnimationChannel::Path::TRANSLATION);
					break;
				case 2:
					paths.push_back(AnimationComponent::AnimationChannel::Path::ROTATION);
					break;
				case 3:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SCALE);
					break;
				case 4:
					paths.push_back(AnimationComponent::AnimationChannel::Path::WEIGHTS);
					break;
				case 5:
					paths.push_back(AnimationComponent::AnimationChannel::Path::LIGHT_COLOR);
					break;
				case 6:
					paths.push_back(AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY);
					break;
				case 7:
					paths.push_back(AnimationComponent::AnimationChannel::Path::LIGHT_RANGE);
					break;
				case 8:
					paths.push_back(AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE);
					break;
				case 9:
					paths.push_back(AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE);
					break;
				case 10:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SOUND_PLAY);
					break;
				case 11:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SOUND_STOP);
					break;
				case 12:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SOUND_VOLUME);
					break;
				case 13:
					paths.push_back(AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT);
					break;
				case 14:
					paths.push_back(AnimationComponent::AnimationChannel::Path::CAMERA_FOV);
					break;
				case 15:
					paths.push_back(AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH);
					break;
				case 16:
					paths.push_back(AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE);
					break;
				case 17:
					paths.push_back(AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE);
					break;
				case 18:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY);
					break;
				case 19:
					paths.push_back(AnimationComponent::AnimationChannel::Path::SCRIPT_STOP);
					break;
				case 20:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR);
					break;
				case 21:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE);
					break;
				case 22:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS);
					break;
				case 23:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS);
					break;
				case 24:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE);
					break;
				case 25:
					paths.push_back(AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD);
					break;
				}

				for (auto path : paths)
				{
					for (auto& selected : editor->translator.selected)
					{
						int channelIndex = -1;
						for (int i = 0; i < (int)animation->channels.size(); ++i)
						{
							// Search for channel for this path and target:
							auto& channel = animation->channels[i];
							if (channel.path == path && channel.target == selected.entity)
							{
								channelIndex = i;
								break;
							}
						}
						if (channelIndex < 0)
						{
							// No channel found for this path and target, create it:
							channelIndex = (int)animation->channels.size();
							auto& channel = animation->channels.emplace_back();
							channel.samplerIndex = (int)animation->samplers.size();
							channel.target = selected.entity;
							channel.path = path;
							auto& sam = animation->samplers.emplace_back();
							Entity animation_data_entity = CreateEntity();
							scene.animation_datas.Create(animation_data_entity);
							sam.data = animation_data_entity;
						}
						auto& channel = animation->channels[channelIndex];

						AnimationDataComponent* animation_data = scene.animation_datas.GetComponent(animation->samplers[channel.samplerIndex].data);
						if (animation_data != nullptr)
						{
							animation_data->keyframe_times.push_back(current_time);

							switch (channel.path)
							{
							case wi::scene::AnimationComponent::AnimationChannel::Path::TRANSLATION:
							{
								const TransformComponent* transform = scene.transforms.GetComponent(channel.target);
								if (transform != nullptr)
								{
									animation_data->keyframe_data.push_back(transform->translation_local.x);
									animation_data->keyframe_data.push_back(transform->translation_local.y);
									animation_data->keyframe_data.push_back(transform->translation_local.z);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::ROTATION:
							{
								const TransformComponent* transform = scene.transforms.GetComponent(channel.target);
								if (transform != nullptr)
								{
									animation_data->keyframe_data.push_back(transform->rotation_local.x);
									animation_data->keyframe_data.push_back(transform->rotation_local.y);
									animation_data->keyframe_data.push_back(transform->rotation_local.z);
									animation_data->keyframe_data.push_back(transform->rotation_local.w);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::SCALE:
							{
								const TransformComponent* transform = scene.transforms.GetComponent(channel.target);
								if (transform != nullptr)
								{
									animation_data->keyframe_data.push_back(transform->scale_local.x);
									animation_data->keyframe_data.push_back(transform->scale_local.y);
									animation_data->keyframe_data.push_back(transform->scale_local.z);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::WEIGHTS:
							{
								const MeshComponent* mesh = scene.meshes.GetComponent(channel.target);
								if (mesh == nullptr && scene.objects.Contains(selected.entity))
								{
									// Also try query mesh of selected object:
									ObjectComponent* object = scene.objects.GetComponent(selected.entity);
									mesh = scene.meshes.GetComponent(object->meshID);
									channel.target = selected.entity;
								}
								if (mesh != nullptr && !mesh->morph_targets.empty())
								{
									for (const MeshComponent::MorphTarget& morph : mesh->morph_targets)
									{
										animation_data->keyframe_data.push_back(morph.weight);
									}
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
							{
								const LightComponent* light = scene.lights.GetComponent(channel.target);
								if (light != nullptr)
								{
									animation_data->keyframe_data.push_back(light->color.x);
									animation_data->keyframe_data.push_back(light->color.y);
									animation_data->keyframe_data.push_back(light->color.z);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
							{
								const LightComponent* light = scene.lights.GetComponent(channel.target);
								if (light != nullptr)
								{
									animation_data->keyframe_data.push_back(light->intensity);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
							{
								const LightComponent* light = scene.lights.GetComponent(channel.target);
								if (light != nullptr)
								{
									animation_data->keyframe_data.push_back(light->range);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
							{
								const LightComponent* light = scene.lights.GetComponent(channel.target);
								if (light != nullptr)
								{
									animation_data->keyframe_data.push_back(light->innerConeAngle);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
							{
								const LightComponent* light = scene.lights.GetComponent(channel.target);
								if (light != nullptr)
								{
									animation_data->keyframe_data.push_back(light->outerConeAngle);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_PLAY:
							case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_STOP:
							{
								const SoundComponent* sound = scene.sounds.GetComponent(channel.target);
								if (sound != nullptr)
								{
									// no data
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
							{
								const SoundComponent* sound = scene.sounds.GetComponent(channel.target);
								if (sound != nullptr)
								{
									animation_data->keyframe_data.push_back(sound->volume);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
							{
								const wi::EmittedParticleSystem* emitter = scene.emitters.GetComponent(channel.target);
								if (emitter != nullptr)
								{
									animation_data->keyframe_data.push_back(emitter->count);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
							{
								const CameraComponent* camera = scene.cameras.GetComponent(channel.target);
								if (camera != nullptr)
								{
									animation_data->keyframe_data.push_back(camera->fov);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
							{
								const CameraComponent* camera = scene.cameras.GetComponent(channel.target);
								if (camera != nullptr)
								{
									animation_data->keyframe_data.push_back(camera->focal_length);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
							{
								const CameraComponent* camera = scene.cameras.GetComponent(channel.target);
								if (camera != nullptr)
								{
									animation_data->keyframe_data.push_back(camera->aperture_size);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
							{
								const CameraComponent* camera = scene.cameras.GetComponent(channel.target);
								if (camera != nullptr)
								{
									animation_data->keyframe_data.push_back(camera->aperture_shape.x);
									animation_data->keyframe_data.push_back(camera->aperture_shape.y);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY:
							case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_STOP:
							{
								const ScriptComponent* script = scene.scripts.GetComponent(channel.target);
								if (script != nullptr)
								{
									// no data
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->baseColor.x);
									animation_data->keyframe_data.push_back(material->baseColor.y);
									animation_data->keyframe_data.push_back(material->baseColor.z);
									animation_data->keyframe_data.push_back(material->baseColor.w);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->emissiveColor.x);
									animation_data->keyframe_data.push_back(material->emissiveColor.y);
									animation_data->keyframe_data.push_back(material->emissiveColor.z);
									animation_data->keyframe_data.push_back(material->emissiveColor.w);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->roughness);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->metalness);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->reflectance);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
							{
								const MaterialComponent* material = scene.materials.GetComponent(channel.target);
								if (material != nullptr)
								{
									animation_data->keyframe_data.push_back(material->texMulAdd.x);
									animation_data->keyframe_data.push_back(material->texMulAdd.y);
									animation_data->keyframe_data.push_back(material->texMulAdd.z);
									animation_data->keyframe_data.push_back(material->texMulAdd.w);
								}
								else
								{
									animation_data->keyframe_times.pop_back();
									animation->channels.pop_back();
								}
							}
							break;
							default:
								break;
							}
						}
					}
				}
			}
		}
		recordCombo.SetSelectedWithoutCallback(-1);
		RefreshKeyframesList();
	});
	recordCombo.SetTooltip("Record selected entities' specified channels into the animation at the current time.");
	AddWidget(&recordCombo);


	keyframesList.Create("Keyframes");
	keyframesList.SetSize(XMFLOAT2(wid, 200));
	keyframesList.SetPos(XMFLOAT2(4, y += step));
	keyframesList.OnSelect([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		AnimationComponent* animation = scene.animations.GetComponent(entity);
		if (animation != nullptr)
		{
			uint32_t channelIndex = args.userdata & 0xFFFFFFFF;
			uint32_t timeIndex = uint32_t(args.userdata >> 32ull) & 0xFFFFFFFF;
			if (animation->channels.size() > channelIndex)
			{
				const AnimationComponent::AnimationChannel& channel = animation->channels[channelIndex];
				const AnimationComponent::AnimationSampler& sam = animation->samplers[channel.samplerIndex];
				const AnimationDataComponent* animation_data = scene.animation_datas.GetComponent(sam.data);
				if (animation_data != nullptr && animation_data->keyframe_times.size() > timeIndex)
				{
					float time = animation_data->keyframe_times[timeIndex];
					animation->timer = time;
				}
			}
		}
	});
	keyframesList.OnDelete([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		AnimationComponent* animation = scene.animations.GetComponent(entity);
		if (animation != nullptr)
		{
			uint32_t channelIndex = args.userdata & 0xFFFFFFFF;
			uint32_t timeIndex = uint32_t(args.userdata >> 32ull) & 0xFFFFFFFF;
			if (animation->channels.size() > channelIndex)
			{
				const AnimationComponent::AnimationChannel& channel = animation->channels[channelIndex];
				const AnimationComponent::AnimationSampler& sam = animation->samplers[channel.samplerIndex];
				AnimationDataComponent* animation_data = scene.animation_datas.GetComponent(sam.data);

				if (animation_data != nullptr && animation_data->keyframe_times.size() > timeIndex)
				{
					// specific keyframe deletion:
					const AnimationComponent::AnimationChannel::PathDataType path_data_type = channel.GetPathDataType();

					switch (path_data_type)
					{
					case AnimationComponent::AnimationChannel::PathDataType::Event:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						break;
					case AnimationComponent::AnimationChannel::PathDataType::Float:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex);
						break;
					case AnimationComponent::AnimationChannel::PathDataType::Float2:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * 2, animation_data->keyframe_data.begin() + timeIndex * 2 + 2);
						break;
					case AnimationComponent::AnimationChannel::PathDataType::Float3:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * 3, animation_data->keyframe_data.begin() + timeIndex * 3 + 3);
						break;
					case AnimationComponent::AnimationChannel::PathDataType::Float4:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * 4, animation_data->keyframe_data.begin() + timeIndex * 4 + 4);
						break;
					case AnimationComponent::AnimationChannel::PathDataType::Weights:
						{
							MeshComponent* mesh = scene.meshes.GetComponent(channel.target);
							if (mesh == nullptr && scene.objects.Contains(channel.target))
							{
								// Also try query mesh of selected object:
								ObjectComponent* object = scene.objects.GetComponent(channel.target);
								mesh = scene.meshes.GetComponent(object->meshID);
							}
							if (mesh != nullptr)
							{
								animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
								animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * mesh->morph_targets.size(), animation_data->keyframe_data.begin() + timeIndex * mesh->morph_targets.size() + mesh->morph_targets.size());
							}
						}
						break;
					default:
						break;
					}
				}
				else
				{
					// entire channel deletion:
					animation->channels.erase(animation->channels.begin() + channelIndex);
				}
			}
		}
		RefreshKeyframesList();
	});
	AddWidget(&keyframesList);


	retargetCombo.Create("Retarget: ");
	retargetCombo.SetSize(XMFLOAT2(wid, hei));
	retargetCombo.selected_font.anim.typewriter.looped = true;
	retargetCombo.selected_font.anim.typewriter.time = 2;
	retargetCombo.selected_font.anim.typewriter.character_start = 1;
	retargetCombo.SetTooltip("Make a copy of this animation and retarget it for a humanoid.\nThis will only work for bones in this animation that are part of a humanoid.");
	retargetCombo.SetInvalidSelectionText("...");
	retargetCombo.OnSelect([=](wi::gui::EventArgs args) {
		retargetCombo.SetSelectedWithoutCallback(-1);
		wi::scene::Scene& scene = editor->GetCurrentScene();

		Entity retarget_entity = scene.RetargetAnimation((Entity)args.userdata, entity, true);
		if (retarget_entity != INVALID_ENTITY)
		{
			NameComponent name;
			const NameComponent* name_source = scene.names.GetComponent(entity);
			if (name_source != nullptr)
			{
				name.name += name_source->name;
			}
			scene.names.Create(retarget_entity) = name;

			editor->componentsWnd.RefreshEntityTree();
		}
	});
	AddWidget(&retargetCombo);

	// Root Motion Tick
	rootMotionCheckBox.Create("RootMotion: ");
	rootMotionCheckBox.SetTooltip("Toggle root bone animation.");
	rootMotionCheckBox.SetSize(XMFLOAT2(hei, hei));
	//rootMotionCheckBox.SetPos(XMFLOAT2(x, y += step));
	rootMotionCheckBox.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (args.bValue) {
				animation->RootMotionOn();
			}
			else {
				animation->RootMotionOff();
			}
		}
		});
	rootMotionCheckBox.SetCheckText(ICON_CHECK);
	AddWidget(&rootMotionCheckBox);
	// Root Bone selector
	rootBoneComboBox.Create("Root Bone: ");
	rootBoneComboBox.SetSize(XMFLOAT2(wid, hei));
	rootBoneComboBox.SetPos(XMFLOAT2(x, y));
	rootBoneComboBox.SetEnabled(false);
	rootBoneComboBox.OnSelect([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			Entity ent = (Entity)args.userdata;
			if (ent != wi::ecs::INVALID_ENTITY)
			{
				animation->rootMotionBone = ent;
			}
			else
			{
				animation->rootMotionBone = wi::ecs::INVALID_ENTITY;
			}
		}
		});
	rootBoneComboBox.SetTooltip("Choose the root bone to evaluate root motion from.");
	AddWidget(&rootBoneComboBox);

	SetMinimized(true);
	SetVisible(false);

}
// Example function to check if an entity already exists in the list
static bool EntityExistsInList(Entity entity, const wi::vector<Entity>& entityList)
{
	// Iterate through the list of entities
	for (Entity e : entityList)
	{
		if (e == entity)
		{
			// Entity found in the list
			return true;
		}
	}
	// Entity not found in the list
	return false;
}

void AnimationWindow::SetEntity(Entity entity)
{
	if (this->entity != entity)
	{
		wi::scene::Scene& scene = editor->GetCurrentScene();

		AnimationComponent* animation = scene.animations.GetComponent(entity);
		if (animation != nullptr || IsCollapsed())
		{
			this->entity = entity;
			RefreshKeyframesList();

			rootBoneComboBox.ClearItems();
			rootBoneComboBox.AddItem("NO ROOT BONE " ICON_DISABLED, wi::ecs::INVALID_ENTITY);
			if (animation != nullptr)
			{
				// Define a list of entities
				wi::vector<Entity> bone_list;
				// Add items to root bone name combo box.
				for (const AnimationComponent::AnimationChannel& channel : animation->channels)
				{
					if (channel.path == AnimationComponent::AnimationChannel::Path::TRANSLATION ||
						channel.path == AnimationComponent::AnimationChannel::Path::ROTATION) {

						if (!EntityExistsInList(channel.target, bone_list))
						{
							bone_list.push_back(channel.target);
							const NameComponent* name = scene.names.GetComponent(channel.target);
							if (name != nullptr)
							{
								rootBoneComboBox.AddItem(name->name, channel.target);
							}
						}
					}
				}
				bone_list.clear();
			}
		}
	}
}

void AnimationWindow::Update()
{
	Scene& scene = editor->GetCurrentScene();

	if (!scene.animations.Contains(entity))
	{
		SetEnabled(false);
		return;
	}
	else
	{
		SetEnabled(true);
	}

	AnimationComponent& animation = *scene.animations.GetComponent(entity);

	if (animation.IsPlaying())
	{
		stopButton.SetText(ICON_PAUSE);
		stopButton.SetTooltip("Pause");
	}
	else
	{
		stopButton.SetText(ICON_STOP);
		stopButton.SetTooltip("Stop");
	}

	if (animation.IsLooped())
	{
		loopTypeButton.SetText(ICON_LOOP);
		loopTypeButton.SetTooltip("Looping is enabled. The animation will repeat continuously.");
	}
	else if (animation.IsPingPong())
	{
		loopTypeButton.SetText(ICON_PINGPONG);
		loopTypeButton.SetTooltip("Ping-Pong is enabled. The animation will play forward and then backwards repeatedly.");
	}
	else
	{
		loopTypeButton.SetText("-");
		loopTypeButton.SetTooltip("No loop. The animation will play once.");
	}

	if(!animation.samplers.empty())
	{
		modeComboBox.SetSelectedByUserdataWithoutCallback(animation.samplers[0].mode);
	}
	else
	{
		modeComboBox.SetSelectedByUserdataWithoutCallback(AnimationComponent::AnimationSampler().mode);
	}

	rootMotionCheckBox.SetCheck(animation.IsRootMotion());
	rootBoneComboBox.SetSelectedByUserdataWithoutCallback(animation.rootMotionBone);

	timerSlider.SetRange(animation.start, animation.end);
	timerSlider.SetValue(animation.timer);
	amountSlider.SetValue(animation.amount);
	startInput.SetValue(animation.start);
	endInput.SetValue(animation.end);
}


void AnimationWindow::RefreshKeyframesList()
{
	Scene& scene = editor->GetCurrentScene();

	if (!scene.animations.Contains(entity))
	{
		SetEntity(INVALID_ENTITY);
		return;
	}

	AnimationComponent& animation = *scene.animations.GetComponent(entity);

	keyframesList.ClearItems();
	uint32_t channelIndex = 0;
	for (const AnimationComponent::AnimationChannel& channel : animation.channels)
	{
		wi::gui::TreeList::Item item;
		switch (channel.path)
		{
		case wi::scene::AnimationComponent::AnimationChannel::Path::TRANSLATION:
			item.name += ICON_TRANSLATE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::ROTATION:
			item.name += ICON_ROTATE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SCALE:
			item.name += ICON_SCALE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::WEIGHTS:
			item.name += ICON_MESH " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
			item.name += ICON_POINTLIGHT " [color] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
			item.name += ICON_POINTLIGHT " [intensity] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
			item.name += ICON_POINTLIGHT " [range] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
			item.name += ICON_POINTLIGHT " [inner cone] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
			item.name += ICON_POINTLIGHT " [outer cone] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_PLAY:
			item.name += ICON_SOUND " [play] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_STOP:
			item.name += ICON_SOUND " [stop] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SOUND_VOLUME:
			item.name += ICON_SOUND " [volume] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::EMITTER_EMITCOUNT:
			item.name += ICON_EMITTER " [emit count] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOV:
			item.name += ICON_CAMERA " [fov] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_FOCAL_LENGTH:
			item.name += ICON_CAMERA " [focal length] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SIZE:
			item.name += ICON_CAMERA " [aperture size] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::CAMERA_APERTURE_SHAPE:
			item.name += ICON_CAMERA " [aperture shape] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_PLAY:
			item.name += ICON_SCRIPT " [play] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::SCRIPT_STOP:
			item.name += ICON_SCRIPT " [stop] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_COLOR:
			item.name += ICON_MATERIAL " [color] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_EMISSIVE:
			item.name += ICON_MATERIAL " [emissive] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_ROUGHNESS:
			item.name += ICON_MATERIAL " [roughness] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_METALNESS:
			item.name += ICON_MATERIAL " [metalness] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_REFLECTANCE:
			item.name += ICON_MATERIAL " [reflectance] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::Path::MATERIAL_TEXMULADD:
			item.name += ICON_MATERIAL " [texmuladd] ";
			break;
		default:
			break;
		}
		const NameComponent* name = scene.names.GetComponent(channel.target);
		if (name == nullptr)
		{
			item.name += "[no_name] " + std::to_string(channel.target);
		}
		else if (name->name.empty())
		{
			item.name += "[name_empty] " + std::to_string(channel.target);
		}
		else
		{
			item.name += name->name;
		}

		item.userdata = 0ull;
		item.userdata |= channelIndex & 0xFFFFFFFF;
		item.userdata |= uint64_t(0xFFFFFFFF) << 32ull; // invalid time index, means entire channel
		keyframesList.AddItem(item);

		auto& sam = animation.samplers[channel.samplerIndex];
		AnimationDataComponent* animation_data = scene.animation_datas.GetComponent(sam.data);
		if (animation_data != nullptr)
		{
			uint32_t timeIndex = 0;
			for (float time : animation_data->keyframe_times)
			{
				wi::gui::TreeList::Item item2;
				item2.name = std::to_string(time) + " sec";
				item2.level = 1;
				item2.userdata = 0ull;
				item2.userdata |= channelIndex & 0xFFFFFFFF;
				item2.userdata |= uint64_t(timeIndex & 0xFFFFFFFF) << 32ull;
				keyframesList.AddItem(item2);
				timeIndex++;
			}
		}
		channelIndex++;
	}

	retargetCombo.ClearItems();
	for (size_t i = 0; i < scene.humanoids.GetCount(); ++i)
	{
		std::string item = ICON_HUMANOID " ";
		Entity entity = scene.humanoids.GetEntity(i);
		const NameComponent* name = scene.names.GetComponent(entity);
		if (name == nullptr)
		{
			item += "[no_name] " + std::to_string(entity);
		}
		else if (name->name.empty())
		{
			item += "[name_empty] " + std::to_string(entity);
		}
		else
		{
			item += name->name;
		}
		retargetCombo.AddItem(item, (uint64_t)entity);
	}
}


void AnimationWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 80;
	const float margin_right = 50;

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
		const float margin_right = 40;
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
	add(modeComboBox);

	loopTypeButton.SetPos(XMFLOAT2(margin_left, y));
	const float l = loopTypeButton.GetPos().x + loopTypeButton.GetSize().x + padding;
	const float r = width - margin_right - padding * 4;
	const float diff = r - l;
	backwardsButton.SetSize(XMFLOAT2(diff/5, backwardsButton.GetSize().y));
	backwardsFromEndButton.SetSize(backwardsButton.GetSize());
	stopButton.SetSize(backwardsButton.GetSize());
	playFromStartButton.SetSize(backwardsButton.GetSize());
	playButton.SetSize(backwardsButton.GetSize());
	backwardsButton.SetPos(XMFLOAT2(loopTypeButton.GetPos().x + loopTypeButton.GetSize().x + padding, y));
	backwardsFromEndButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x + padding, y));
	stopButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 2 + padding * 2, y));
	playFromStartButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 3 + padding * 3, y));
	playButton.SetPos(XMFLOAT2(backwardsButton.GetPos().x + backwardsButton.GetSize().x * 4 + padding * 4, y));
	y += stopButton.GetSize().y;
	y += padding;

	add(timerSlider);
	add(amountSlider);
	add(speedSlider);
	add(startInput);
	add(endInput);
	add(recordCombo);
	add(retargetCombo);
	rootMotionCheckBox.SetPos(XMFLOAT2(margin_left, y));
	y += rootMotionCheckBox.GetSize().y;
	y += padding;
	add(rootBoneComboBox);
	add_fullwidth(keyframesList);
}
