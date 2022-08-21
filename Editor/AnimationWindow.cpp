#include "stdafx.h"
#include "AnimationWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void AnimationWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_ANIMATION " Animation", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(520, 400));

	closeButton.SetTooltip("Delete Animation");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().animations.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 80;
	float y = 0;
	float hei = 18;
	float wid = 200;
	float step = hei + 4;

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
	modeComboBox.SetTooltip("Choose how animation data is interpreted between keyframes.\nNote that Cubic spline sampling requires spline animation data, otherwise, it will fall back to Linear!");
	AddWidget(&modeComboBox);

	loopedCheckBox.Create("Looped: ");
	loopedCheckBox.SetTooltip("Toggle animation looping behaviour.");
	loopedCheckBox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckBox.SetPos(XMFLOAT2(x, y += step));
	loopedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->SetLooped(args.bValue);
		}
	});
	loopedCheckBox.SetCheckText(ICON_LOOP);
	AddWidget(&loopedCheckBox);

	playButton.Create(ICON_PLAY);
	playButton.SetSize(XMFLOAT2(100, hei));
	playButton.SetPos(XMFLOAT2(loopedCheckBox.GetPos().x + loopedCheckBox.GetSize().x + 5, y));
	playButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->IsPlaying())
			{
				animation->Pause();
			}
			else
			{
				animation->Play();
			}
		}
	});
	AddWidget(&playButton);

	stopButton.Create(ICON_STOP);
	stopButton.SetTooltip("Stop");
	stopButton.SetSize(XMFLOAT2(70, hei));
	stopButton.SetPos(XMFLOAT2(playButton.GetPos().x + playButton.GetSize().x + 5, y));
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->Stop();
		}
	});
	AddWidget(&stopButton);

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
			animation->speed = args.fValue;
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

						switch (channel.path)
						{
						case wi::scene::AnimationComponent::AnimationChannel::TRANSLATION:
						case wi::scene::AnimationComponent::AnimationChannel::SCALE:
						case wi::scene::AnimationComponent::AnimationChannel::LIGHT_COLOR:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 0]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 1]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 3 + 2]);
						}
						break;
						case wi::scene::AnimationComponent::AnimationChannel::ROTATION:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 0]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 1]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 2]);
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst * 4 + 3]);
						}
						break;
						case wi::scene::AnimationComponent::AnimationChannel::WEIGHTS:
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
						case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INTENSITY:
						case wi::scene::AnimationComponent::AnimationChannel::LIGHT_RANGE:
						case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INNERCONE:
						case wi::scene::AnimationComponent::AnimationChannel::LIGHT_OUTERCONE:
						{
							animation_data->keyframe_data.push_back(animation_data->keyframe_data[keyFirst]);
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
							case wi::scene::AnimationComponent::AnimationChannel::TRANSLATION:
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
							case wi::scene::AnimationComponent::AnimationChannel::ROTATION:
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
							case wi::scene::AnimationComponent::AnimationChannel::SCALE:
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
							case wi::scene::AnimationComponent::AnimationChannel::WEIGHTS:
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
							case wi::scene::AnimationComponent::AnimationChannel::LIGHT_COLOR:
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
							case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INTENSITY:
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
							case wi::scene::AnimationComponent::AnimationChannel::LIGHT_RANGE:
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
							case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INNERCONE:
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
							case wi::scene::AnimationComponent::AnimationChannel::LIGHT_OUTERCONE:
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
					switch (channel.path)
					{
					case AnimationComponent::AnimationChannel::Path::TRANSLATION:
					case AnimationComponent::AnimationChannel::Path::SCALE:
					case AnimationComponent::AnimationChannel::Path::LIGHT_COLOR:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * 3, animation_data->keyframe_data.begin() + timeIndex * 3 + 3);
						break;
					case AnimationComponent::AnimationChannel::Path::ROTATION:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
						animation_data->keyframe_data.erase(animation_data->keyframe_data.begin() + timeIndex * 4, animation_data->keyframe_data.begin() + timeIndex * 4 + 4);
						break;
					case AnimationComponent::AnimationChannel::Path::WEIGHTS:
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
					case AnimationComponent::AnimationChannel::Path::LIGHT_INTENSITY:
					case AnimationComponent::AnimationChannel::Path::LIGHT_RANGE:
					case AnimationComponent::AnimationChannel::Path::LIGHT_INNERCONE:
					case AnimationComponent::AnimationChannel::Path::LIGHT_OUTERCONE:
						animation_data->keyframe_times.erase(animation_data->keyframe_times.begin() + timeIndex);
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



	SetMinimized(true);
	SetVisible(false);

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
		playButton.SetText(ICON_PAUSE);
		playButton.SetTooltip("Pause");
	}
	else
	{
		playButton.SetText(ICON_PLAY);
		playButton.SetTooltip("Play");
	}

	if(!animation.samplers.empty())
	{
		modeComboBox.SetSelectedByUserdataWithoutCallback(animation.samplers[0].mode);
	}
	else
	{
		modeComboBox.SetSelectedByUserdataWithoutCallback(AnimationComponent::AnimationSampler().mode);
	}

	loopedCheckBox.SetCheck(animation.IsLooped());

	timerSlider.SetRange(animation.start, animation.end);
	timerSlider.SetValue(animation.timer);
	amountSlider.SetValue(animation.amount);
	speedSlider.SetValue(animation.speed);
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
		case wi::scene::AnimationComponent::AnimationChannel::TRANSLATION:
			item.name += ICON_TRANSLATE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::ROTATION:
			item.name += ICON_ROTATE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::SCALE:
			item.name += ICON_SCALE " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::WEIGHTS:
			item.name += ICON_MESH " ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::LIGHT_COLOR:
			item.name += ICON_POINTLIGHT " [color] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INTENSITY:
			item.name += ICON_POINTLIGHT " [intensity] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::LIGHT_RANGE:
			item.name += ICON_POINTLIGHT " [range] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::LIGHT_INNERCONE:
			item.name += ICON_POINTLIGHT " [inner cone] ";
			break;
		case wi::scene::AnimationComponent::AnimationChannel::LIGHT_OUTERCONE:
			item.name += ICON_POINTLIGHT " [outer cone] ";
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
}


void AnimationWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x - padding * 2;
	keyframesList.SetSize(XMFLOAT2(width, keyframesList.GetSize().y));
}
