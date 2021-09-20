#include "../uicommon/imgui.h"

#include "audio_player.h"
#include <math.h>

const char* TOOL_NAME = "Filter Test Tool";
int TOOL_WIDTH = 640;
int TOOL_HEIGHT = 580;

static const int NOTE_MIN = 24;
static const int NOTE_MAX = 96;

float note_to_frequency(int p_note)
{
	return (float)(440.0f * pow(2.0, (p_note - 69.0) / 12.0));
}

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
	bool update_sine = false;
	bool update_square = false;
	bool update_saw = false;
	bool update_filter = false;
	bool update_output_filter = false;

	// gui
	int window_y = next_window_dims(0, 80);
	ImGui::Begin("Output Audio Engine");

		static int audio_engine = (int)AudioEngine_FAudio;
		update_engine |= ImGui::RadioButton("FAudio", &audio_engine, (int)AudioEngine_FAudio);
		#ifdef HAVE_XAUDIO2 
		ImGui::SameLine();
		update_engine |= ImGui::RadioButton("XAudio2", &audio_engine, (int)AudioEngine_XAudio2); 
		#endif

		static int num_channels = 1;
		update_engine |= ImGui::RadioButton("1 Channel", &num_channels, 1); ImGui::SameLine();
		update_engine |= ImGui::RadioButton("2 Channels", &num_channels, 2);

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Sine Wave Generator");
		
		static int sine_note = 60;
		update_sine |= ImGui::SliderInt("Frequency", &sine_note, NOTE_MIN, NOTE_MAX); ImGui::SameLine();
		ImGui::Text(" (%.2f Hz)", note_to_frequency(sine_note));

		static float sine_volume = 0.0f;
		update_sine |= ImGui::SliderFloat("Volume", &sine_volume, 0.0f, 1.0f);

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Square Wave Generator");

		static int square_note = 60;
		update_square |= ImGui::SliderInt("Frequency", &square_note, NOTE_MIN, NOTE_MAX); ImGui::SameLine();
		ImGui::Text(" (%.2f Hz)", note_to_frequency(square_note));

		static float square_volume = 0.0f;
		update_square |= ImGui::SliderFloat("Volume", &square_volume, 0.0f, 1.0f);

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Saw Tooth Generator");

		static int saw_note = 60;
		update_saw |= ImGui::SliderInt("Frequency", &saw_note, NOTE_MIN, NOTE_MAX); ImGui::SameLine();
		ImGui::Text(" (%.2f Hz)", note_to_frequency(saw_note));

		static float saw_volume = 0.0f;
		update_saw |= ImGui::SliderFloat("Volume", &saw_volume, 0.0f, 1.0f);

	ImGui::End();

	window_y = next_window_dims(window_y, 100);
	ImGui::Begin("Filter");

		static int filter_type = -1;
		static int filter_cutoff_note = 60;
		static float filter_q = 0.7f;

		update_filter |= ImGui::RadioButton("None", &filter_type, -1); ImGui::SameLine();
		update_filter |= ImGui::RadioButton("Low-Pass", &filter_type, 0); ImGui::SameLine();
		update_filter |= ImGui::RadioButton("Band-Pass", &filter_type, 1); ImGui::SameLine();
		update_filter |= ImGui::RadioButton("High-Pass", &filter_type, 2); ImGui::SameLine();
		update_filter |= ImGui::RadioButton("Notch", &filter_type, 3); 

		update_filter |= ImGui::SliderInt("Cutoff Frequency", &filter_cutoff_note, 12, 108); ImGui::SameLine();
		ImGui::Text(" (%.2f Hz)", note_to_frequency(filter_cutoff_note));
		update_filter |= ImGui::SliderFloat("Q", &filter_q, 0.7f, 100.0f, "%.1f");

	ImGui::End();

	window_y = next_window_dims(window_y, 100);
	ImGui::Begin("Output Filter");

		static int output_filter_type = -1;
		static int output_filter_cutoff_note = 60;
		static float output_filter_q = 0.7f;

		update_output_filter |= ImGui::RadioButton("None", &output_filter_type, -1); ImGui::SameLine();
		update_output_filter |= ImGui::RadioButton("Low-Pass", &output_filter_type, 0); ImGui::SameLine();
		update_output_filter |= ImGui::RadioButton("Band-Pass", &output_filter_type, 1); ImGui::SameLine();
		update_output_filter |= ImGui::RadioButton("High-Pass", &output_filter_type, 2); ImGui::SameLine();
		update_output_filter |= ImGui::RadioButton("Notch", &output_filter_type, 3);

		update_output_filter |= ImGui::SliderInt("Cutoff Frequency", &output_filter_cutoff_note, 12, 108); ImGui::SameLine();
		ImGui::Text(" (%.2f Hz)", note_to_frequency(output_filter_cutoff_note));
		update_output_filter |= ImGui::SliderFloat("Q", &output_filter_q, 0.7f, 100.0f, "%.1f");

	ImGui::End();

	// audio control
	static AudioPlayer	player;

	if (update_engine)
	{
		player.shutdown();
		player.setup((AudioEngine)audio_engine, num_channels);
	}

	if (update_sine | update_engine)
	{
		player.oscillator_change(AudioPlayer::SineWave, note_to_frequency(sine_note), sine_volume);
	}

	if (update_square | update_engine)
	{
		player.oscillator_change(AudioPlayer::SquareWave, note_to_frequency(square_note), square_volume);
	}

	if (update_saw | update_engine)
	{
		player.oscillator_change(AudioPlayer::SawTooth, note_to_frequency(saw_note), saw_volume);
	}

	if (update_filter | update_engine)
	{
		player.filter_change(filter_type, note_to_frequency(filter_cutoff_note), filter_q);
	}
	if (update_output_filter | update_engine)
	{
		player.output_filter_change(output_filter_type, note_to_frequency(output_filter_cutoff_note), output_filter_q);
	}
}
