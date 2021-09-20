#include "../uicommon/imgui.h"

#include "audio.h"
#include <math.h>

const char* TOOL_NAME = "Volume Meter Test Tool";
int TOOL_WIDTH = 640;
int TOOL_HEIGHT = 510;

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

	#define MAX_TIMELINE 120
	static float peaksL[MAX_TIMELINE] = { 0 };
	static float peaksR[MAX_TIMELINE] = { 0 };
	static float rmsL[MAX_TIMELINE] = { 0 };
	static float rmsR[MAX_TIMELINE] = { 0 };
	static int vmOffset = 0;

	// GUI
	int window_y = next_window_dims(0, 50);
	ImGui::Begin("Output Audio Engine");

		static int audio_engine = (int) AudioEngine_FAudio;
		update_engine |= ImGui::RadioButton(
			"FAudio",
			&audio_engine,
			(int) AudioEngine_FAudio);
		ImGui::SameLine();
		#ifdef HAVE_XAUDIO2
		update_engine |= ImGui::RadioButton(
			"XAudio2",
			&audio_engine,
			(int) AudioEngine_XAudio2
		);
		#endif

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Wave file to play");

		static int wave_index = (int) AudioWave_SnareDrum01;
		static bool wave_stereo = false;

		update_wave |= ImGui::RadioButton(
			"Snare Drum (Forte)",
			&wave_index,
			(int) AudioWave_SnareDrum01
		);
		ImGui::SameLine();
		update_wave |= ImGui::RadioButton(
			"Snare Drum (Fortissimo)",
			&wave_index,
			(int) AudioWave_SnareDrum02
		);
		ImGui::SameLine();
		update_wave |= ImGui::RadioButton(
			"Snare Drum (Mezzo-Forte)",
			&wave_index,
			(int) AudioWave_SnareDrum03
		);

		play_wave = ImGui::Button("Play"); ImGui::SameLine();
		update_wave |= ImGui::Checkbox("Stereo", &wave_stereo);

	ImGui::End();

	window_y = next_window_dims(window_y, 370);
	ImGui::Begin("Volume Meter");

		ImGui::PlotLines(
			"Peaks Channel 1",
			peaksL,
			MAX_TIMELINE,
			vmOffset,
			NULL,
			0.0f,
			1.0f,
			ImVec2(0, 80)
		);
		ImGui::PlotLines(
			"Peaks Channel 2",
			peaksR,
			MAX_TIMELINE,
			vmOffset,
			NULL,
			0.0f,
			1.0f,
			ImVec2(0, 80)
		);
		ImGui::PlotLines(
			"RMS Channel 1",
			rmsL,
			MAX_TIMELINE,
			vmOffset,
			NULL,
			0.0f,
			1.0f,
			ImVec2(0, 80)
		);
		ImGui::PlotLines(
			"RMS Channel 2",
			rmsR,
			MAX_TIMELINE,
			vmOffset,
			NULL,
			0.0f,
			1.0f,
			ImVec2(0, 80)
		);

	ImGui::End();

	// Audio context
	static AudioContext *player = NULL;
	if (player == NULL || update_engine)
	{
		if (player != NULL)
		{
			audio_destroy_context(player);
		}
		player = audio_create_context((AudioEngine) audio_engine);
	}
	if (update_wave | update_engine)
	{
		audio_wave_load(player, (AudioSampleWave) wave_index, wave_stereo);
	}
	if (play_wave)
	{
		audio_wave_play(player);
	}
	float peaks[2] = { 0.0f, 0.0f };
	float rms[2] = { 0.0f, 0.0f };
	audio_update_volumemeter(player, peaks, rms);
	peaksL[vmOffset] = peaks[0];
	peaksR[vmOffset] = peaks[1];
	rmsL[vmOffset] = rms[0];
	rmsR[vmOffset] = rms[1];
	vmOffset = (vmOffset + 1) % MAX_TIMELINE;
}
