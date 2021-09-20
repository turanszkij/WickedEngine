/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2021 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

/* This is where the actual tool lives! Try to do your work in here! */

#include <FACT_internal.h> /* DO NOT INCLUDE THIS IN REAL CODE! */

#include "../uicommon/imgui.h"

#include <SDL.h>
#include <vector>
#include <string>

const char* TOOL_NAME = "FACT Auditioning Tool";
int TOOL_WIDTH = 1280;
int TOOL_HEIGHT = 720;

bool openEngineShow = true;

std::vector<FACTAudioEngine*> engines;
std::vector<std::string> engineNames;
std::vector<bool> engineShows;

std::vector<FACTSoundBank*> soundBanks;
std::vector<std::string> soundbankNames;
std::vector<bool> soundbankShows;

std::vector<FACTWaveBank*> waveBanks;
std::vector<uint8_t*> wavebankMems;
std::vector<std::string> wavebankNames;
std::vector<bool> wavebankShows;

std::vector<FACTWave*> waves;

void FAudioTool_Init()
{
	/* Nothing to do... */
}

void FAudioTool_Quit()
{
	for (size_t i = 0; i < soundBanks.size(); i += 1)
	{
		FACTSoundBank_Destroy(soundBanks[i]);
	}
	for (size_t i = 0; i < waves.size(); i += 1)
	{
		FACTWave_Destroy(waves[i]);
	}
	for (size_t i = 0; i < waveBanks.size(); i += 1)
	{
		FACTWaveBank_Destroy(waveBanks[i]);
		SDL_free(wavebankMems[i]);
	}
	for (size_t i = 0; i < engines.size(); i += 1)
	{
		FACTAudioEngine_ShutDown(engines[i]);
	}
}

bool show_test_window = false;
void FAudioTool_Update()
{
	ImGui::ShowDemoWindow(&show_test_window);

	uint8_t *buf;
	size_t len;

	/* Menu bar */
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Open AudioEngine", NULL, openEngineShow, true))
			{
				openEngineShow = !openEngineShow;
			}
			for (size_t i = 0; i < engines.size(); i += 1)
			{
				if (ImGui::MenuItem(
					engineNames[i].c_str(),
					NULL,
					engineShows[i],
					true
				)) {
					engineShows[i] = !engineShows[i];
				}
			}
			for (size_t i = 0; i < soundBanks.size(); i += 1)
			{
				if (ImGui::MenuItem(
					soundbankNames[i].c_str(),
					NULL,
					soundbankShows[i],
					true
				)) {
					soundbankShows[i] = !soundbankShows[i];
				}
			}
			for (size_t i = 0; i < waveBanks.size(); i += 1)
			{
				if (ImGui::MenuItem(
					wavebankNames[i].c_str(),
					NULL,
					wavebankShows[i],
					true
				)) {
					wavebankShows[i] = !wavebankShows[i];
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	/* Open AudioEngine */
	if (openEngineShow)
	{
		if (ImGui::Begin("Open AudioEngine"))
		{
			static char enginename[64] = "AudioEngine.xgs";
			if (ImGui::InputText(
				"Open AudioEngine",
				enginename, 64,
				ImGuiInputTextFlags_EnterReturnsTrue
			)) {
				/* Load up file... */
				buf = (uint8_t*) SDL_LoadFile(enginename, &len);
				FACTRuntimeParameters params;
				SDL_memset(
					&params,
					'\0',
					sizeof(FACTRuntimeParameters)
				);
				params.pGlobalSettingsBuffer = buf;
				params.globalSettingsBufferSize = len;

				/* Create engine... */
				FACTAudioEngine *engine;
				FACTCreateEngine(0, &engine);
				FACTAudioEngine_Initialize(engine, &params);
				SDL_free(buf);

				/* Add to UI... */
				engines.push_back(engine);
				engineNames.push_back(enginename);
				engineShows.push_back(true);
			}
		}
		ImGui::End();
	}

	/* AudioEngine windows */
	for (size_t i = 0; i < engines.size(); i += 1)
	{
		FACTAudioEngine_DoWork(engines[i]);
		if (engineShows[i])
		{
			/* Early out */
			if (!ImGui::Begin(engineNames[i].c_str()))
			{
				ImGui::End();
				continue;
			}

			/* Categories */
			if (ImGui::CollapsingHeader("Categories"))
			for (uint16_t j = 0; j < engines[i]->categoryCount; j += 1)
			if (ImGui::TreeNode(engines[i]->categoryNames[j]))
			{
				ImGui::Text(
					"Max Instances: %d",
					engines[i]->categories[j].instanceLimit
				);
				ImGui::Text(
					"Fade-in (ms): %d",
					engines[i]->categories[j].fadeInMS
				);
				ImGui::Text(
					"Fade-out (ms): %d",
					engines[i]->categories[j].fadeOutMS
				);
				ImGui::Text(
					"Instance Behavior: %X",
					engines[i]->categories[j].maxInstanceBehavior
				);
				ImGui::Text(
					"Parent Category Index: %d",
					engines[i]->categories[j].parentCategory
				);
				ImGui::Text(
					"Base Volume: %f",
					engines[i]->categories[j].volume
				);
				ImGui::Text(
					"Visibility: %X",
					engines[i]->categories[j].visibility
				);
				ImGui::TreePop();
			}

			/* Variables */
			if (ImGui::CollapsingHeader("Variables"))
			for (uint16_t j = 0; j < engines[i]->variableCount; j += 1)
			if (ImGui::TreeNode(engines[i]->variableNames[j]))
			{
				ImGui::Text(
					"Accessibility: %X",
					engines[i]->variables[j].accessibility
				);
				ImGui::Text(
					"Initial Value: %f",
					engines[i]->variables[j].initialValue
				);
				ImGui::Text(
					"Min Value: %f",
					engines[i]->variables[j].minValue
				);
				ImGui::Text(
					"Max Value: %f",
					engines[i]->variables[j].maxValue
				);
				ImGui::TreePop();
			}

			/* RPCs */
			if (ImGui::CollapsingHeader("RPCs"))
			for (uint16_t j = 0; j < engines[i]->rpcCount; j += 1)
			if (ImGui::TreeNode(
				(void*) (intptr_t) j,
				"Code %d",
				engines[i]->rpcCodes[j]
			)) {
				ImGui::Text(
					"Variable Index: %d",
					engines[i]->rpcs[j].variable
				);
				ImGui::Text(
					"Parameter: %d",
					engines[i]->rpcs[j].parameter
				);
				ImGui::Text(
					"Point Count: %d",
					engines[i]->rpcs[j].pointCount
				);
				if (ImGui::TreeNode("Points"))
				{
					for (uint8_t k = 0; k < engines[i]->rpcs[j].pointCount; k += 1)
					if (ImGui::TreeNode(
						(void*) (intptr_t) k,
						"Point #%d",
						k
					)) {
						ImGui::Text(
							"Coordinate: (%f, %f)",
							engines[i]->rpcs[j].points[k].x,
							engines[i]->rpcs[j].points[k].y
						);
						ImGui::Text(
							"Type: %d\n",
							engines[i]->rpcs[j].points[k].type
						);
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}

			/* DSP Presets */
			if (ImGui::CollapsingHeader("DSP Presets"))
			for (uint16_t j = 0; j < engines[i]->dspPresetCount; j += 1)
			if (ImGui::TreeNode(
				(void*) (intptr_t) j,
				"Code %d",
				engines[i]->dspPresetCodes[j]
			)) {
				ImGui::Text(
					"Accessibility: %X",
					engines[i]->dspPresets[j].accessibility
				);
				ImGui::Text(
					"Parameter Count: %d",
					engines[i]->dspPresets[j].parameterCount
				);
				if (ImGui::TreeNode("Parameters"))
				{
					for (uint32_t k = 0; k < engines[i]->dspPresets[j].parameterCount; k += 1)
					if (ImGui::TreeNode(
						(void*) (intptr_t) k,
						"Parameter #%d",
						k
					)) {
						ImGui::Text(
							"Initial Value: %f",
							engines[i]->dspPresets[j].parameters[k].value
						);
						ImGui::Text(
							"Min Value: %f",
							engines[i]->dspPresets[j].parameters[k].minVal
						);
						ImGui::Text(
							"Max Value: %f",
							engines[i]->dspPresets[j].parameters[k].maxVal
						);
						ImGui::Text(
							"Unknown u16: %d",
							engines[i]->dspPresets[j].parameters[k].unknown
						);
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}

			ImGui::Separator();

			/* Open SoundBank */
			static char soundbankname[64] = "Sound Bank.xsb";
			if (ImGui::InputText(
				"Open SoundBank",
				soundbankname, 64,
				ImGuiInputTextFlags_EnterReturnsTrue
			)) {
				/* Load up file... */
				buf = (uint8_t*) SDL_LoadFile(soundbankname, &len);

				/* Create SoundBank... */
				FACTSoundBank *sb;
				FACTAudioEngine_CreateSoundBank(
					engines[i],
					buf,
					len,
					0,
					0,
					&sb
				);
				SDL_free(buf);

				/* Add to UI... */
				soundBanks.push_back(sb);
				soundbankNames.push_back(
					"SoundBank: " + std::string(sb->name)
				);
				soundbankShows.push_back(true);
			}

			/* Open WaveBank */
			static char wavebankname[64] = "Wave Bank.xwb";
			if (ImGui::InputText(
				"Open WaveBank",
				wavebankname, 64,
				ImGuiInputTextFlags_EnterReturnsTrue
			)) {
				/* Load up file... */
				buf = (uint8_t*) SDL_LoadFile(wavebankname, &len);

				/* Create WaveBank... */
				FACTWaveBank *wb;
				FACTAudioEngine_CreateInMemoryWaveBank(
					engines[i],
					buf,
					len,
					0,
					0,
					&wb
				);

				/* Add to UI... */
				waveBanks.push_back(wb);
				wavebankMems.push_back(buf);
				wavebankNames.push_back(
					"WaveBank: " + std::string(wb->name)
				);
				wavebankShows.push_back(true);
			}

			ImGui::Separator();

			/* Close file */
			if (ImGui::Button("Close AudioEngine"))
			{
				/* Destroy SoundBank windows... */
				LinkedList *list = engines[i]->sbList;
				while (list != NULL)
				{
					for (size_t j = 0; j < soundBanks.size(); j += 1)
					{
						if (list->entry == soundBanks[j])
						{
							list = list->next;
							soundBanks.erase(soundBanks.begin() + j);
							soundbankNames.erase(soundbankNames.begin() + j);
							soundbankShows.erase(soundbankShows.begin() + j);
							break;
						}
					}
				}

				/* Destroy WaveBank windows... */
				list = engines[i]->wbList;
				while (list != NULL)
				{
					for (size_t j = 0; j < waveBanks.size(); j += 1)
					{
						if (list->entry == waveBanks[j])
						{
							list = list->next;
							// FIXME: SDL_free(wavebankMems[j]);
							waveBanks.erase(waveBanks.begin() + j);
							wavebankMems.erase(wavebankMems.begin() + j);
							wavebankNames.erase(wavebankNames.begin() + j);
							wavebankShows.erase(wavebankShows.begin() + j);
							break;
						}
					}
				}

				/* Close file, finally */
				FACTAudioEngine_ShutDown(engines[i]);
				engines.erase(engines.begin() + i);
				engineNames.erase(engineNames.begin() + i);
				engineShows.erase(engineShows.begin() + i);
				i -= 1;
			}

			/* We out. */
			ImGui::End();
		}
	}

	/* SoundBank windows */
	for (size_t i = 0; i < soundBanks.size(); i += 1)
	if (soundbankShows[i])
	{
		/* Early out */
		if (!ImGui::Begin(soundbankNames[i].c_str()))
		{
			ImGui::End();
			continue;
		}

		/* Close file */
		if (ImGui::Button("Close SoundBank"))
		{
			FACTSoundBank_Destroy(soundBanks[i]);
			soundBanks.erase(soundBanks.begin() + i);
			soundbankNames.erase(soundbankNames.begin() + i);
			soundbankShows.erase(soundbankShows.begin() + i);
			i -= 1;
			ImGui::End();
			continue;
		}

		ImGui::Separator();

		/* WaveBank Dependencies */
		if (ImGui::CollapsingHeader("WaveBank Dependencies"))
		for (uint8_t j = 0; j < soundBanks[i]->wavebankCount; j += 1)
		{
			ImGui::Text("%s", soundBanks[i]->wavebankNames[j]);
		}

		/* Cues */
		if (ImGui::CollapsingHeader("Cues"))
		for (uint16_t j = 0; j < soundBanks[i]->cueCount; j += 1)
		{
			char nameText[255];
			if (soundBanks[i]->cueNames != NULL)
			{
				SDL_snprintf(nameText, sizeof(nameText), "%s", soundBanks[i]->cueNames[j]);
			}
			else
			{
				SDL_snprintf(nameText, sizeof(nameText), "%s-%d\n", soundBanks[i]->name, j);
			}
			if (ImGui::TreeNode(nameText))
			{
				char playText[64];
				SDL_snprintf(playText, 64, "Play %s", nameText);
				if (ImGui::Button(playText))
				{
					FACTSoundBank_Play(
						soundBanks[i],
						j,
						0,
						0,
						NULL
					);
				}
				ImGui::Text(
					"Flags: %X",
					soundBanks[i]->cues[j].flags
				);
				ImGui::Text(
					"Sound/Variation Code: %d",
					soundBanks[i]->cues[j].sbCode
				);
				ImGui::Text(
					"Transition Offset: %d",
					soundBanks[i]->cues[j].transitionOffset
				);
				ImGui::Text(
					"Instance Limit: %d",
					soundBanks[i]->cues[j].instanceLimit
				);
				ImGui::Text(
					"Fade-in (ms): %d",
					soundBanks[i]->cues[j].fadeInMS
				);
				ImGui::Text(
					"Fade-out (ms): %d",
					soundBanks[i]->cues[j].fadeOutMS
				);
				ImGui::Text(
					"Max Instance Behavior: %d",
					soundBanks[i]->cues[j].maxInstanceBehavior
				);
				ImGui::TreePop();
			}
		}

		/* Sounds */
		if (ImGui::CollapsingHeader("Sounds"))
		for (uint16_t j = 0; j < soundBanks[i]->soundCount; j += 1)
		{
			if (ImGui::TreeNode(
				(void*) (intptr_t) j,
				"Code %d",
				soundBanks[i]->soundCodes[j]
			)) {
				ImGui::Text(
					"Flags: %X",
					soundBanks[i]->sounds[j].flags
				);
				ImGui::Text(
					"Category Index: %d",
					soundBanks[i]->sounds[j].category
				);
				ImGui::Text(
					"Volume: %f",
					soundBanks[i]->sounds[j].volume
				);
				ImGui::Text(
					"Pitch: %d",
					soundBanks[i]->sounds[j].pitch
				);
				ImGui::Text(
					"Priority: %d",
					soundBanks[i]->sounds[j].priority
				);
				ImGui::Text(
					"RPC Code Count: %d",
					soundBanks[i]->sounds[j].rpcCodeCount
				);
				ImGui::Text(
					"DSP Preset Code Count: %d",
					soundBanks[i]->sounds[j].dspCodeCount
				);
				ImGui::Text(
					"Track Count: %d",
					soundBanks[i]->sounds[j].trackCount
				);
				if (ImGui::TreeNode("RPC Codes"))
				{
					for (uint8_t k = 0; k < soundBanks[i]->sounds[j].rpcCodeCount; k += 1)
					{
						ImGui::Text(
							"%d",
							soundBanks[i]->sounds[j].rpcCodes[k]
						);
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("DSP Preset Codes"))
				{
					for (uint8_t k = 0; k < soundBanks[i]->sounds[j].dspCodeCount; k += 1)
					{
						ImGui::Text(
							"%d",
							soundBanks[i]->sounds[j].dspCodes[k]
						);
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Tracks"))
				{
					for (uint8_t k = 0; k < soundBanks[i]->sounds[j].trackCount; k += 1)
					if (ImGui::TreeNode(
						(void*) (intptr_t) k,
						"Track Code %d",
						soundBanks[i]->sounds[j].tracks[k].code
					)) {
						ImGui::Text(
							"Volume: %f",
							soundBanks[i]->sounds[j].tracks[k].volume
						);
						ImGui::Text(
							"Filter Type: %d",
							soundBanks[i]->sounds[j].tracks[k].filter
						);
						ImGui::Text(
							"Filter Q-Factor: %d",
							soundBanks[i]->sounds[j].tracks[k].qfactor
						);
						ImGui::Text(
							"Filter Frequency: %d",
							soundBanks[i]->sounds[j].tracks[k].frequency
						);
						ImGui::Text(
							"RPC Code Count: %d",
							soundBanks[i]->sounds[j].tracks[k].rpcCodeCount
						);
						ImGui::Text(
							"Event Count: %d",
							soundBanks[i]->sounds[j].tracks[k].eventCount
						);
						if (ImGui::TreeNode("RPC Codes"))
						{
							for (uint8_t l = 0; l < soundBanks[i]->sounds[j].tracks[k].rpcCodeCount; l += 1)
							{
								ImGui::Text(
									"%d",
									soundBanks[i]->sounds[j].tracks[k].rpcCodes[l]
								);
							}
							ImGui::TreePop();
						}
						if (ImGui::TreeNode("Events"))
						{
							for (uint8_t l = 0; l < soundBanks[i]->sounds[j].tracks[k].eventCount; l += 1)
							if (ImGui::TreeNode(
								(void*) (intptr_t) l,
								"Event #%d",
								l
							)) {
								const FACTEvent *evt = &soundBanks[i]->sounds[j].tracks[k].events[l];
								ImGui::Text(
									"Type: %d",
									evt->type
								);
								ImGui::Text(
									"Timestamp: %d",
									evt->timestamp
								);
								ImGui::Text(
									"Random Offset: %d",
									evt->randomOffset
								);
								if (evt->type == FACTEVENT_STOP)
								{
									ImGui::Text(
										"Flags: %X",
										evt->stop.flags
									);
								}
								else if (	evt->type == FACTEVENT_PLAYWAVE ||
										evt->type == FACTEVENT_PLAYWAVETRACKVARIATION ||
										evt->type == FACTEVENT_PLAYWAVEEFFECTVARIATION ||
										evt->type == FACTEVENT_PLAYWAVETRACKEFFECTVARIATION	)
								{
									ImGui::Text(
										"Play Flags: %X",
										evt->wave.flags
									);
									ImGui::Text(
										"Position: %d",
										evt->wave.position
									);
									ImGui::Text(
										"Angle: %d",
										evt->wave.angle
									);
									ImGui::Text(
										"Loop Count: %d",
										evt->wave.loopCount
									);
									if (evt->wave.isComplex)
									{
										ImGui::Text(
											"Track Variation Type: %d",
											evt->wave.complex.variation
										);
										ImGui::Text(
											"Track Count: %d",
											evt->wave.complex.trackCount
										);
										if (ImGui::TreeNode("Tracks"))
										{
											for (uint16_t m = 0; m < evt->wave.complex.trackCount; m += 1)
											if (ImGui::TreeNode(
												(void*) (intptr_t) m,
												"Track #%d",
												m
											)) {
												ImGui::Text(
													"Track Index: %d",
													evt->wave.complex.tracks[m]
												);
												ImGui::Text(
													"WaveBank Index: %d",
													evt->wave.complex.wavebanks[m]
												);
												ImGui::Text(
													"Weight: %d",
													evt->wave.complex.weights[m]
												);
												ImGui::TreePop();
											}
											ImGui::TreePop();
										}
									}
									else
									{
										ImGui::Text(
											"Track Index: %d",
											evt->wave.simple.track
										);
										ImGui::Text(
											"WaveBank Index: %d",
											evt->wave.simple.wavebank
										);
									}

									if (	evt->wave.variationFlags != 0 &&
										evt->wave.variationFlags != 0xFFFF	)
									{
										ImGui::Text(
											"Effect Variation Flags: %X",
											evt->wave.variationFlags
										);
										if (evt->wave.variationFlags & 0x1000)
										{
											ImGui::Text(
												"Min Pitch: %d",
												evt->wave.minPitch
											);
											ImGui::Text(
												"Max Pitch: %d",
												evt->wave.maxPitch
											);
										}
										if (evt->wave.variationFlags & 0x2000)
										{
											ImGui::Text(
												"Min Volume: %f",
												evt->wave.minVolume
											);
											ImGui::Text(
												"Max Volume: %f",
												evt->wave.maxVolume
											);
										}
										if (evt->wave.variationFlags & 0xC000)
										{
											ImGui::Text(
												"Min Frequency: %f",
												evt->wave.minFrequency
											);
											ImGui::Text(
												"Max Frequency: %f",
												evt->wave.maxFrequency
											);
											ImGui::Text(
												"Min Q-Factor: %f",
												evt->wave.minQFactor
											);
											ImGui::Text(
												"Max Q-Factor: %f",
												evt->wave.maxQFactor
											);
										}
									}
								}
								else if (	evt->type == FACTEVENT_PITCH ||
										evt->type == FACTEVENT_VOLUME ||
										evt->type == FACTEVENT_PITCHREPEATING ||
										evt->type == FACTEVENT_VOLUMEREPEATING	)
								{
									if (evt->value.settings == 0)
									{
										ImGui::Text(
											"Equation Flags: %X",
											evt->value.equation.flags
										);
										ImGui::Text(
											"Value 1: %f",
											evt->value.equation.value1
										);
										ImGui::Text(
											"Value 2: %f",
											evt->value.equation.value2
										);
									}
									else
									{
										ImGui::Text(
											"Initial Value: %f",
											evt->value.ramp.initialValue
										);
										ImGui::Text(
											"Initial Slope: %f",
											evt->value.ramp.initialSlope
										);
										ImGui::Text(
											"Slope Delta: %f",
											evt->value.ramp.slopeDelta
										);
										ImGui::Text(
											"Duration: %d",
											evt->value.ramp.duration
										);
									}
									ImGui::Text(
										"Repeats: %d",
										evt->value.repeats
									);
									ImGui::Text(
										"Frequency: %d",
										evt->value.frequency
									);
								}
								else if (	evt->type == FACTEVENT_MARKER ||
										evt->type == FACTEVENT_MARKERREPEATING	)
								{
									ImGui::Text(
										"Marker: %d",
										evt->marker.marker
									);
									ImGui::Text(
										"Repeats: %d",
										evt->marker.repeats
									);
									ImGui::Text(
										"Frequency: %d",
										evt->marker.frequency
									);
								}
								else
								{
									FAudio_assert(0 && "Unknown event type!");
								}
								ImGui::TreePop();
							}
							ImGui::TreePop();
						}
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
		}

		/* Variations */
		if (ImGui::CollapsingHeader("Variations"))
		for (uint16_t j = 0; j < soundBanks[i]->variationCount; j += 1)
		if (ImGui::TreeNode(
			(void*) (intptr_t) j,
			"Code #%d",
			soundBanks[i]->variationCodes[j]
		)) {
			ImGui::Text(
				"Flags: %X",
				soundBanks[i]->variations[j].flags
			);
			ImGui::Text(
				"Interactive Variable Index: %d",
				soundBanks[i]->variations[j].variable
			);
			ImGui::Text(
				"Entry Count: %X",
				soundBanks[i]->variations[j].entryCount
			);
			if (ImGui::TreeNode("Entries"))
			{
				for (uint16_t k = 0; k < soundBanks[i]->variations[j].entryCount; k += 1)
				if (ImGui::TreeNode(
					(void*) (intptr_t) k,
					"Entry #%d",
					k
				)) {
					if (soundBanks[i]->variations[j].isComplex)
					{
						ImGui::Text("Complex Variation");
						ImGui::Text(
							"Sound Code: %d",
							soundBanks[i]->variations[j].entries[k].soundCode
						);
					}
					else
					{
						ImGui::Text("Simple Variation");
						ImGui::Text(
							"Track Index: %d",
							soundBanks[i]->variations[j].entries[k].simple.track
						);
						ImGui::Text(
							"WaveBank Index: %d",
							soundBanks[i]->variations[j].entries[k].simple.wavebank
						);
					}
					ImGui::Text(
						"Min Weight: %f",
						soundBanks[i]->variations[j].entries[k].minWeight
					);
					ImGui::Text(
						"Max Weight: %f",
						soundBanks[i]->variations[j].entries[k].maxWeight
					);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		/* Transitions */
		if (ImGui::CollapsingHeader("Transitions"))
		for (uint16_t j = 0; j < soundBanks[i]->transitionCount; j += 1)
		if (ImGui::TreeNode(
			(void*) (intptr_t) j,
			"Code #%d",
			soundBanks[i]->transitionCodes[j]
		)) {
			ImGui::Text(
				"Entry Count: %X",
				soundBanks[i]->transitions[j].entryCount
			);
			if (ImGui::TreeNode("Entries"))
			{
				for (uint16_t k = 0; k < soundBanks[i]->transitions[j].entryCount; k += 1)
				if (ImGui::TreeNode(
					(void*) (intptr_t) k,
					"Entry #%d",
					k
				)) {
					ImGui::Text(
						"Sound Code: %d",
						soundBanks[i]->transitions[j].entries[k].soundCode
					);
					ImGui::Text(
						"Src Min Marker: %d",
						soundBanks[i]->transitions[j].entries[k].srcMarkerMin
					);
					ImGui::Text(
						"Src Max Marker: %d",
						soundBanks[i]->transitions[j].entries[k].srcMarkerMax
					);
					ImGui::Text(
						"Dst Min Marker: %d",
						soundBanks[i]->transitions[j].entries[k].dstMarkerMin
					);
					ImGui::Text(
						"Dst Max Marker: %d",
						soundBanks[i]->transitions[j].entries[k].dstMarkerMax
					);
					ImGui::Text(
						"Fade In: %d",
						soundBanks[i]->transitions[j].entries[k].fadeIn
					);
					ImGui::Text(
						"Fade Out: %d",
						soundBanks[i]->transitions[j].entries[k].fadeOut
					);
					ImGui::Text(
						"Flags: %d",
						soundBanks[i]->transitions[j].entries[k].flags
					);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		/* We out. */
		ImGui::End();
	}

	/* WaveBank windows */
	for (size_t i = 0; i < waveBanks.size(); i += 1)
	if (wavebankShows[i])
	{
		/* Early out */
		if (!ImGui::Begin(wavebankNames[i].c_str()))
		{
			ImGui::End();
			continue;
		}

		/* Close file */
		if (ImGui::Button("Close WaveBank"))
		{
			FACTWaveBank_Destroy(waveBanks[i]);
			SDL_free(wavebankMems[i]);
			waveBanks.erase(waveBanks.begin() + i);
			wavebankMems.erase(wavebankMems.begin() + i);
			wavebankNames.erase(wavebankNames.begin() + i);
			wavebankShows.erase(wavebankShows.begin() + i);
			i -= 1;
			ImGui::End();
			continue;
		}

		/* Giant table of wavedata entries */
		ImGui::Columns(12, "wavebankentries");
		ImGui::Separator();
		ImGui::Text("Entry");		ImGui::NextColumn();
		ImGui::Text("Flags");		ImGui::NextColumn();
		ImGui::Text("Duration");	ImGui::NextColumn();
		ImGui::Text("Format Tag");	ImGui::NextColumn();
		ImGui::Text("Channels");	ImGui::NextColumn();
		ImGui::Text("Sample Rate");	ImGui::NextColumn();
		ImGui::Text("Block Align");	ImGui::NextColumn();
		ImGui::Text("Bit Depth");	ImGui::NextColumn();
		ImGui::Text("Play Offset");	ImGui::NextColumn();
		ImGui::Text("Play Length");	ImGui::NextColumn();
		ImGui::Text("Loop Offset");	ImGui::NextColumn();
		ImGui::Text("Loop Length");	ImGui::NextColumn();
		ImGui::Separator();
		for (uint32_t j = 0; j < waveBanks[i]->entryCount; j += 1)
		{
			char playText[16];
			SDL_snprintf(playText, 16, "%d", j);
			if (ImGui::Button(playText))
			{
				FACTWave *wave;
				FACTWaveBank_Play(
					waveBanks[i],
					j,
					0,
					0,
					0,
					&wave
				);
				waves.push_back(wave);
			}
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].dwFlags);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Duration);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wFormatTag);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.nChannels);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.nSamplesPerSec);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wBlockAlign);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].Format.wBitsPerSample);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].PlayRegion.dwOffset);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].PlayRegion.dwLength);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].LoopRegion.dwStartSample);
			ImGui::NextColumn();
			ImGui::Text("%d", waveBanks[i]->entries[j].LoopRegion.dwTotalSamples);
			ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::Separator();

		/* We out. */
		ImGui::End();
	}

	/* Playing Waves */
	for (size_t i = 0; i < waves.size(); i += 1)
	{
		uint32_t state;
		FACTWave_GetState(waves[i], &state);
		if (state & FACT_STATE_STOPPED)
		{
			FACTWave_Destroy(waves[i]);
			waves.erase(waves.begin() + i);
			i -= 1;
		}
	}
}
