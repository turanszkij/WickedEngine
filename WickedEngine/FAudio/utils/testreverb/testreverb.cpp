#include "../uicommon/imgui.h"

#include "audio.h"
#include <math.h>

const char* TOOL_NAME = "Reverb Test Tool";
int TOOL_WIDTH = 640;
int TOOL_HEIGHT = 850;

int next_window_dims(int y_pos, int height)
{
	ImGui::SetNextWindowPos(ImVec2(0, static_cast<float>(y_pos)));
	ImGui::SetNextWindowSize(ImVec2(640, static_cast<float>(height)));

	return y_pos + height;
}

void FAudioTool_Init()
{
	/* Nothing to do... */
}

void FAudioTool_Quit()
{
	/* Nothing to do... */
}

void FAudioTool_Update()
{
	bool update_engine = false;
	bool update_wave = false;
	bool play_wave = false;
	bool stop_wave = false;
	bool update_effect = false;

	// gui
	int window_y = next_window_dims(0, 80);
	ImGui::Begin("Output Audio Engine");

		static int audio_engine = (int)AudioEngine_FAudio;
		update_engine |= ImGui::RadioButton("FAudio", &audio_engine, (int)AudioEngine_FAudio); ImGui::SameLine();
		#ifdef HAVE_XAUDIO2 
		update_engine |= ImGui::RadioButton("XAudio2", &audio_engine, (int)AudioEngine_XAudio2); ImGui::SameLine();
		#endif

		static bool output_5p1 = false;
		update_engine |= ImGui::Checkbox("5.1 channel output", &output_5p1);

		static int32_t voice_index = (int32_t) AudioVoiceType_Submix;
		update_engine |= ImGui::Combo("Apply effect to", &voice_index, audio_voice_type_names, 3);

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Wave file to play");

		static int wave_index = (int)AudioWave_SnareDrum01;
		static bool wave_stereo = false;

		update_wave |= ImGui::RadioButton("Snare Drum (Forte)", &wave_index, (int)AudioWave_SnareDrum01); ImGui::SameLine();
		update_wave |= ImGui::RadioButton("Snare Drum (Fortissimo)", &wave_index, (int)AudioWave_SnareDrum02); ImGui::SameLine();
		update_wave |= ImGui::RadioButton("Snare Drum (Mezzo-Forte)", &wave_index, (int)AudioWave_SnareDrum03); 

		play_wave = ImGui::Button("Play"); ImGui::SameLine();
		stop_wave = ImGui::Button("Stop"); ImGui::SameLine();
		update_wave |= ImGui::Checkbox("Stereo", &wave_stereo);
		
	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Reverb effect");
		
		static bool effect_enabled = false;
		update_effect |= ImGui::Checkbox("Enabled", &effect_enabled);

		static int32_t preset_index = -1;
		static ReverbParameters reverb_params = {
			100.0f,
			40,
			60,
			0, 0, 0, 0, 0,
			3,
			3,
			0,
			0,
			0,
			0,
			5000.0f,
			-5.0f,
			-5.0f,
			-5.0f,
			-5.0f,
			2000.0f,
			50.0f,
			100.0f,
		};

		if (ImGui::Combo("Preset", &preset_index, audio_reverb_preset_names, audio_reverb_preset_count)) {
			memcpy(&reverb_params, &audio_reverb_presets[preset_index], sizeof(ReverbParameters));
			update_effect = true;
		}

	ImGui::End();

	window_y = next_window_dims(window_y, 600);
	ImGui::Begin("FAudio Tune Detail");

		int ReverbDelay = reverb_params.ReverbDelay;
		int RearDelay = reverb_params.RearDelay;
		int PositionLeft = reverb_params.PositionLeft;
		int PositionRight = reverb_params.PositionRight;
		int PositionMatrixLeft = reverb_params.PositionMatrixLeft;
		int PositionMatrixRight = reverb_params.PositionMatrixRight;
		int EarlyDiffusion = reverb_params.EarlyDiffusion;
		int LateDiffusion = reverb_params.LateDiffusion;
		int LowEQGain = reverb_params.LowEQGain;
		int LowEQCutoff = reverb_params.LowEQCutoff;
		int HighEQGain = reverb_params.HighEQGain;
		int HighEQCutoff = reverb_params.HighEQCutoff;

		update_effect |= ImGui::SliderFloat("WetDryMix (%)", &reverb_params.WetDryMix, 0, 100);
		update_effect |= ImGui::SliderInt("ReflectionsDelay (ms)", (int *)&reverb_params.ReflectionsDelay, 0, 300);
		update_effect |= ImGui::SliderInt("ReverbDelay (ms)", &ReverbDelay, 0, 85);
		update_effect |= ImGui::SliderInt("RearDelay (ms)", &RearDelay, 0, 5);

		update_effect |= ImGui::SliderInt("PositionLeft", &PositionLeft, 0, 30);
		update_effect |= ImGui::SliderInt("PositionRight", &PositionRight, 0, 30);
		update_effect |= ImGui::SliderInt("PositionMatrixLeft", &PositionMatrixLeft, 0, 30);
		update_effect |= ImGui::SliderInt("PositionMatrixRight", &PositionMatrixRight, 0, 30);

		update_effect |= ImGui::SliderInt("Early Diffusion", &EarlyDiffusion, 0, 15);
		update_effect |= ImGui::SliderInt("Late Diffusion", &LateDiffusion, 0, 15);

		update_effect |= ImGui::SliderInt("LowEqGain", &LowEQGain, 0, 12);
		update_effect |= ImGui::SliderInt("LowEqCuttoff", &LowEQCutoff, 0, 9);
		update_effect |= ImGui::SliderInt("HighEqGain", &HighEQGain, 0, 8);
		update_effect |= ImGui::SliderInt("HighEqCuttoff", &HighEQCutoff, 0, 14);

		update_effect |= ImGui::SliderFloat("RoomFreq (Hz)", &reverb_params.RoomFilterFreq, 20, 20000);
		update_effect |= ImGui::SliderFloat("RoomGain (dB)", &reverb_params.RoomFilterMain, -100, 0);
		update_effect |= ImGui::SliderFloat("RoomHFGain (dB)", &reverb_params.RoomFilterHF, -100, 0);

		update_effect |= ImGui::SliderFloat("ReflectionsGain (dB)", &reverb_params.ReflectionsGain, -100, 20);
		update_effect |= ImGui::SliderFloat("ReverbGain (dB)", &reverb_params.ReverbGain, -100, 20);

		update_effect |= ImGui::SliderFloat("DecayTime (s)", &reverb_params.DecayTime, 0.1f, 15.0f);

		update_effect |= ImGui::SliderFloat("Density (%)", &reverb_params.Density, 0, 100);
		update_effect |= ImGui::SliderFloat("RoomSize (feet)", &reverb_params.RoomSize, 1, 100);

		reverb_params.ReverbDelay = ReverbDelay;
		reverb_params.RearDelay = RearDelay;
		reverb_params.PositionLeft = PositionLeft;
		reverb_params.PositionRight = PositionRight;
		reverb_params.PositionMatrixLeft = PositionMatrixLeft;
		reverb_params.PositionMatrixRight = PositionMatrixRight;
		reverb_params.EarlyDiffusion = EarlyDiffusion;
		reverb_params.LateDiffusion = LateDiffusion;
		reverb_params.LowEQGain = LowEQGain;
		reverb_params.LowEQCutoff = LowEQCutoff;
		reverb_params.HighEQGain = HighEQGain;
		reverb_params.HighEQCutoff = HighEQCutoff;

	ImGui::End();

	// audio control
	static AudioContext *player = NULL;

	if (player == NULL || update_engine)
	{
		if (player != NULL)
		{
			audio_destroy_context(player);
		}
		player = audio_create_context((AudioEngine) audio_engine, output_5p1, (AudioVoiceType) voice_index);
	}

	if (update_wave | update_engine)
	{
		audio_wave_load(player, (AudioSampleWave) wave_index, wave_stereo);
	}

	if (play_wave) {
		audio_wave_play(player);
	}

	if (stop_wave)
	{
		audio_wave_stop(player);
	}

	if ((update_engine || update_effect))
	{
		audio_effect_change(player, effect_enabled, &reverb_params);
	}
}
