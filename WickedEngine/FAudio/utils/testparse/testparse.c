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

#include <FACT_internal.h> /* DO NOT INCLUDE THIS IN REAL CODE! */
#include <SDL.h>

static void print_soundbank(FACTAudioEngine *engine, uint8_t *buf, size_t len)
{
	FACTSoundBank *sb;
	uint32_t i, j, k, l;
	FACTAudioEngine_CreateSoundBank(
		engine,
		buf,
		len,
		0,
		0,
		&sb
	);
	printf("SoundBank \"%s\"\n", sb->name);
	printf("\tWaveBank Dependencies:\n");
	for (i = 0; i < sb->wavebankCount; i += 1)
	{
		printf("\t\tWaveBank %d, \"%s\"\n", i, sb->wavebankNames[i]);
	}
	for (i = 0; i < sb->cueCount; i += 1)
	{
		printf(
			"\tCue %d, \"%s\"\n"
			"\t\tFlags: %X\n"
			"\t\tSound/Variation Code: %d\n"
			"\t\tTransition Offset: %d\n"
			"\t\tInstance Limit: %d\n"
			"\t\tFade-in (ms): %d\n"
			"\t\tFade-out (ms): %d\n"
			"\t\tMax Instance Behavior: %d\n",
			i,
			(sb->cueNames != NULL) ? sb->cueNames[i] : "NONAME",
			sb->cues[i].flags,
			sb->cues[i].sbCode,
			sb->cues[i].transitionOffset,
			sb->cues[i].instanceLimit,
			sb->cues[i].fadeInMS,
			sb->cues[i].fadeOutMS,
			sb->cues[i].maxInstanceBehavior
		);
	}
	for (i = 0; i < sb->soundCount; i += 1)
	{
		printf(
			"\tSound %d, Code %d:\n"
			"\t\tFlags: %X\n"
			"\t\tCategory Index: %d\n"
			"\t\tVolume: %f\n"
			"\t\tPitch: %d\n"
			"\t\tPriority: %d\n",
			i,
			sb->soundCodes[i],
			sb->sounds[i].flags,
			sb->sounds[i].category,
			sb->sounds[i].volume,
			sb->sounds[i].pitch,
			sb->sounds[i].priority
		);
		printf("\t\tRPC Codes:");
		for (j = 0; j < sb->sounds[i].rpcCodeCount; j += 1)
		{
			printf(" %d", sb->sounds[i].rpcCodes[j]);
		}
		printf("\n");
		printf("\t\tDSP Preset Codes:");
		for (j = 0; j < sb->sounds[i].dspCodeCount; j += 1)
		{
			printf(" %d", sb->sounds[i].dspCodes[j]);
		}
		printf("\n");
		printf("\t\tTrack Count: %d\n", sb->sounds[i].trackCount);
		for (j = 0; j < sb->sounds[i].trackCount; j += 1)
		{
			printf(
				"\t\t\tTrack %d:\n"
				"\t\t\t\tVolume: %f\n"
				"\t\t\t\tFilter Type: %d\n"
				"\t\t\t\tFilter Q-Factor: %d\n"
				"\t\t\t\tFilter Frequency: %d\n",
				sb->sounds[i].tracks[j].code,
				sb->sounds[i].tracks[j].volume,
				sb->sounds[i].tracks[j].filter,
				sb->sounds[i].tracks[j].qfactor,
				sb->sounds[i].tracks[j].frequency
			);
			printf("\t\t\t\tRPC Codes:");
			for (k = 0; k < sb->sounds[i].tracks[j].rpcCodeCount; k += 1)
			{
				printf(
					" %d",
					sb->sounds[i].tracks[j].rpcCodes[k]
				);
			}
			printf("\n");
			printf(
				"\t\t\t\tEvent Count: %d\n",
				sb->sounds[i].tracks[j].eventCount
			);
			for (k = 0; k < sb->sounds[i].tracks[j].eventCount; k += 1)
			{
				const FACTEvent *evt = &sb->sounds[i].tracks[j].events[k];
				printf(
					"\t\t\t\t\tEvent %d:\n"
					"\t\t\t\t\t\tType: %d\n"
					"\t\t\t\t\t\tTimestamp: %d\n"
					"\t\t\t\t\t\tRandom Offset: %d\n",
					k,
					evt->type,
					evt->timestamp,
					evt->randomOffset
				);
				if (evt->type == FACTEVENT_STOP)
				{
					printf(
						"\t\t\t\t\t\tFlags: %X\n",
						evt->stop.flags
					);
				}
				else if (	evt->type == FACTEVENT_PLAYWAVE ||
						evt->type == FACTEVENT_PLAYWAVETRACKVARIATION ||
						evt->type == FACTEVENT_PLAYWAVEEFFECTVARIATION ||
						evt->type == FACTEVENT_PLAYWAVETRACKEFFECTVARIATION	)
				{
					printf(
						"\t\t\t\t\t\tPlay Flags: %X\n"
						"\t\t\t\t\t\tPosition: %d\n"
						"\t\t\t\t\t\tAngle: %d\n"
						"\t\t\t\t\t\tLoop Count: %d\n",
						evt->wave.flags,
						evt->wave.position,
						evt->wave.angle,
						evt->wave.loopCount
					);
					if (evt->wave.isComplex)
					{
						printf(
							"\t\t\t\t\t\tTrack Variation Type: %d\n"
							"\t\t\t\t\t\tTrack Count: %d\n",
							evt->wave.complex.variation,
							evt->wave.complex.trackCount
						);
						for (l = 0; l < evt->wave.complex.trackCount; l += 1)
						{
							printf(
								"\t\t\t\t\t\t\tTrack %d:\n"
								"\t\t\t\t\t\t\t\tTrack Index: %d\n"
								"\t\t\t\t\t\t\t\tWaveBank Index: %d\n"
								"\t\t\t\t\t\t\t\tWeight: %d\n",
								l,
								evt->wave.complex.tracks[l],
								evt->wave.complex.wavebanks[l],
								evt->wave.complex.weights[l]
							);
						}
					}
					else
					{
						printf(
							"\t\t\t\t\t\tTrack Index: %d\n"
							"\t\t\t\t\t\tWaveBank Index: %d\n",
							evt->wave.simple.track,
							evt->wave.simple.wavebank
						);
					}

					if (	evt->wave.variationFlags != 0 &&
						evt->wave.variationFlags != 0xFFFF	)
					{
						printf(
							"\t\t\t\t\t\tEffect Variation Flags: %X\n",
							evt->wave.variationFlags
						);
						if (evt->wave.variationFlags & 0x1000)
						{
							printf(
								"\t\t\t\t\t\tMin Pitch: %d\n"
								"\t\t\t\t\t\tMax Pitch: %d\n",
								evt->wave.minPitch,
								evt->wave.maxPitch
							);
						}
						if (evt->wave.variationFlags & 0x2000)
						{
							printf(
								"\t\t\t\t\t\tMin Volume: %f\n"
								"\t\t\t\t\t\tMax Volume: %f\n",
								evt->wave.minVolume,
								evt->wave.maxVolume
							);
						}
						/* FIXME: Frequency/QFactor flags???
						if (evt->wave.variationFlags & 0x4000)
						{
							printf(
								"\t\t\t\t\t\tMin Frequency: %f\n"
								"\t\t\t\t\t\tMax Frequency: %f\n",
								evt->wave.minFrequency,
								evt->wave.maxFrequency
							);
						}
						if (evt->wave.variationFlags & 0x8000)
						{
							printf(
								"\t\t\t\t\t\tMin Q-Factor: %f\n"
								"\t\t\t\t\t\tMax Q-Factor: %f\n",
								evt->wave.minQFactor,
								evt->wave.maxQFactor
							);
						}
						*/
					}
				}
				else if (	evt->type == FACTEVENT_PITCH ||
						evt->type == FACTEVENT_VOLUME ||
						evt->type == FACTEVENT_PITCHREPEATING ||
						evt->type == FACTEVENT_VOLUMEREPEATING	)
				{
					if (evt->value.settings == 0)
					{
						printf(
							"\t\t\t\t\t\tEquation Flags: %X\n"
							"\t\t\t\t\t\tValue 1: %f\n"
							"\t\t\t\t\t\tValue 2: %f\n",
							evt->value.equation.flags,
							evt->value.equation.value1,
							evt->value.equation.value2
						);
					}
					else
					{
						printf(
							"\t\t\t\t\t\tInitial Value: %f\n"
							"\t\t\t\t\t\tInitial Slope: %f\n"
							"\t\t\t\t\t\tSlope Delta: %f\n"
							"\t\t\t\t\t\tDuration: %d\n",
							evt->value.ramp.initialValue,
							evt->value.ramp.initialSlope,
							evt->value.ramp.slopeDelta,
							evt->value.ramp.duration
						);
					}
					printf(
						"\t\t\t\t\t\tRepeats: %d\n"
						"\t\t\t\t\t\tFrequency: %d\n",
						evt->value.repeats,
						evt->value.frequency
					);
				}
				else if (	evt->type == FACTEVENT_MARKER ||
						evt->type == FACTEVENT_MARKERREPEATING	)
				{
					printf(
						"\t\t\t\t\t\tMarker: %d\n"
						"\t\t\t\t\t\tRepeats: %d\n"
						"\t\t\t\t\t\tFrequency: %d\n",
						evt->marker.marker,
						evt->marker.repeats,
						evt->marker.frequency
					);
				}
				else
				{
					FAudio_assert(0 && "Unknown event type!");
				}
			}
		}
	}
	for (i = 0; i < sb->variationCount; i += 1)
	{
		printf(
			"\tVariation %d, Code %d:\n"
			"\t\tFlags: %X\n"
			"\t\tInteractive Variable Index: %d\n"
			"\t\tEntry Count: %d\n",
			i,
			sb->variationCodes[i],
			sb->variations[i].flags,
			sb->variations[i].variable,
			sb->variations[i].entryCount
		);
		for (j = 0; j < sb->variations[i].entryCount; j += 1)
		{
			if (sb->variations[i].isComplex)
			{
				printf(
					"\t\t\tVariation %d, Complex:\n"
					"\t\t\t\tSound Code: %d\n",
					j,
					sb->variations[i].entries[j].soundCode
				);
			}
			else
			{
				printf(
					"\t\t\tVariation %d, Simple:\n"
					"\t\t\t\tTrack Index: %d\n"
					"\t\t\t\tWaveBank Index: %d\n",
					j,
					sb->variations[i].entries[j].simple.track,
					sb->variations[i].entries[j].simple.wavebank
				);
			}
			printf(
				"\t\t\t\tMin Weight: %f\n"
				"\t\t\t\tMax Weight: %f\n",
				sb->variations[i].entries[j].minWeight,
				sb->variations[i].entries[j].maxWeight
			);
		}
	}
	for (i = 0; i < sb->transitionCount; i += 1)
	{
		printf(
			"\tTransition %d, Code %d:\n"
			"\t\tEntry Count: %d\n",
			i,
			sb->transitionCodes[i],
			sb->transitions[i].entryCount
		);
		for (j = 0; j < sb->transitions[i].entryCount; j += 1)
		{
			printf(
				"\t\t\tTransition %d:\n"
				"\t\t\t\tSound Code: %d\n"
				"\t\t\t\tSrc Marker Min: %d\n"
				"\t\t\t\tSrc Marker Max: %d\n"
				"\t\t\t\tDst Marker Min: %d\n"
				"\t\t\t\tDst Marker Max: %d\n"
				"\t\t\t\tFade In: %d\n"
				"\t\t\t\tFade Out: %d\n"
				"\t\t\t\tFlags: %d\n",
				j,
				sb->transitions[i].entries[j].soundCode,
				sb->transitions[i].entries[j].srcMarkerMin,
				sb->transitions[i].entries[j].srcMarkerMax,
				sb->transitions[i].entries[j].dstMarkerMin,
				sb->transitions[i].entries[j].dstMarkerMax,
				sb->transitions[i].entries[j].fadeIn,
				sb->transitions[i].entries[j].fadeOut,
				sb->transitions[i].entries[j].flags
			);
		}
	}
	FACTSoundBank_Destroy(sb);
}

static void print_wavebank(FACTAudioEngine *engine, uint8_t *buf, size_t len)
{
	FACTWaveBank *wb;
	uint32_t i, j;
	FACTAudioEngine_CreateInMemoryWaveBank(
		engine,
		buf,
		len,
		0,
		0,
		&wb
	);
	printf("WaveBank \"%s\"\n", wb->name);
	for (i = 0; i < wb->entryCount; i += 1)
	{
		printf(
			"\tEntry %d\n"
			"\t\tFlags: %d\n"
			"\t\tDuration: %d\n"
			"\t\tFormat:\n"
			"\t\t\tTag: %d\n"
			"\t\t\tChannels: %d\n"
			"\t\t\tSample Rate: %d\n"
			"\t\t\tBlock Align: %d\n"
			"\t\t\tBit Depth: %d\n"
			"\t\tPlay Region:\n"
			"\t\t\tOffset: %d\n"
			"\t\t\tLength: %d\n"
			"\t\tLoop Region:\n"
			"\t\t\tStart Sample: %d\n"
			"\t\t\tTotal Samples: %d\n",
			i,
			wb->entries[i].dwFlags,
			wb->entries[i].Duration,
			wb->entries[i].Format.wFormatTag,
			wb->entries[i].Format.nChannels,
			wb->entries[i].Format.nSamplesPerSec,
			wb->entries[i].Format.wBlockAlign,
			wb->entries[i].Format.wBitsPerSample,
			wb->entries[i].PlayRegion.dwOffset,
			wb->entries[i].PlayRegion.dwLength,
			wb->entries[i].LoopRegion.dwStartSample,
			wb->entries[i].LoopRegion.dwTotalSamples
		);
		if (wb->seekTables != NULL && wb->seekTables[i].entryCount > 0)
		{
			printf(
				"\t\tSeek Table Entry Count: %d\n",
				wb->seekTables[i].entryCount
			);
			for (j = 0; j < wb->seekTables[i].entryCount; j += 1)
			{
				printf("\t\t\t%d\n", wb->seekTables[i].entries[j]);
			}
		}
	}
	FACTWaveBank_Destroy(wb);
}

int main(int argc, char **argv)
{
	FACTAudioEngine *engine;
	FACTRuntimeParameters params;
	uint8_t *buf;
	size_t len;
	uint32_t i, j;

	/* We need an AudioEngine, SoundBank and WaveBank! */
	if (argc < 2)
	{
		return 0;
	}

	/* Parse the AudioEngine */
	buf = (uint8_t*) SDL_LoadFile(argv[1], &len);
	SDL_memset(&params, '\0', sizeof(params));
	params.pGlobalSettingsBuffer = buf;
	params.globalSettingsBufferSize = len;
	FACTCreateEngine(0, &engine);
	FACTAudioEngine_Initialize(engine, &params);
	SDL_free(buf);

	/* Print AudioEngine information */
	printf("AudioEngine:\n");
	for (i = 0; i < engine->categoryCount; i += 1)
	{
		printf(
			"\tCategory %d, \"%s\":\n"
			"\t\tMax Instances: %d\n"
			"\t\tFade-in (ms): %d\n"
			"\t\tFade-out (ms): %d\n"
			"\t\tInstance Behavior: %X\n"
			"\t\tParent Category Index: %d\n"
			"\t\tBase Volume: %f\n"
			"\t\tVisibility: %X\n",
			i,
			engine->categoryNames[i],
			engine->categories[i].instanceLimit,
			engine->categories[i].fadeInMS,
			engine->categories[i].fadeOutMS,
			engine->categories[i].maxInstanceBehavior,
			engine->categories[i].parentCategory,
			engine->categories[i].volume,
			engine->categories[i].visibility
		);
	}
	for (i = 0; i < engine->variableCount; i += 1)
	{
		printf(
			"\tVariable %d, \"%s\":\n"
			"\t\tAccessibility: %X\n"
			"\t\tInitial Value: %f\n"
			"\t\tMin Value: %f\n"
			"\t\tMax Value: %f\n",
			i,
			engine->variableNames[i],
			engine->variables[i].accessibility,
			engine->variables[i].initialValue,
			engine->variables[i].minValue,
			engine->variables[i].maxValue
		);
	}
	for (i = 0; i < engine->rpcCount; i += 1)
	{
		printf(
			"\tRPC %d, Code %d:\n"
			"\t\tVariable Index: %d\n"
			"\t\tParameter: %d\n"
			"\t\tPoint Count: %d\n",
			i,
			engine->rpcCodes[i],
			engine->rpcs[i].variable,
			engine->rpcs[i].parameter,
			engine->rpcs[i].pointCount
		);
		for (j = 0; j < engine->rpcs[i].pointCount; j += 1)
		{
			printf(
				"\t\t\tPoint %d:\n"
				"\t\t\t\tCoordinate: (%f, %f)\n"
				"\t\t\t\tType: %d\n",
				j,
				engine->rpcs[i].points[j].x,
				engine->rpcs[i].points[j].y,
				engine->rpcs[i].points[j].type
			);
		}
	}
	for (i = 0; i < engine->dspPresetCount; i += 1)
	{
		printf(
			"\tDSP Preset %d, Code %d:\n"
			"\t\tAccessibility: %X\n"
			"\t\tParameter Count: %d\n",
			i,
			engine->dspPresetCodes[i],
			engine->dspPresets[i].accessibility,
			engine->dspPresets[i].parameterCount
		);
		for (j = 0; j < engine->dspPresets[i].parameterCount; j += 1)
		{
			printf(
				"\t\t\tParameter %d:\n"
				"\t\t\t\tInitial Value: %f\n"
				"\t\t\t\tMin Value: %f\n"
				"\t\t\t\tMax Value: %f\n"
				"\t\t\t\tUnknown u16: %d\n",
				j,
				engine->dspPresets[i].parameters[j].value,
				engine->dspPresets[i].parameters[j].minVal,
				engine->dspPresets[i].parameters[j].maxVal,
				engine->dspPresets[i].parameters[j].unknown
			);
		}
	}

	/* Print SoundBank/WaveBank information */
	for (i = 2; i < argc; i += 1)
	{
		buf = (uint8_t*) SDL_LoadFile(argv[i], &len);
		if (buf == NULL || len < 4)
		{
			printf("%s invalid input!\n", argv[i]);
		}
		else if (*((uint32_t*) buf) == 0x4B424453)
		{
			print_soundbank(engine, buf, len);
		}
		else if (*((uint32_t*) buf) == 0x444E4257)
		{
			print_wavebank(engine, buf, len);
		}
		else
		{
			printf("%s not an XACT file!\n", argv[i]);
		}
		SDL_free(buf);
	}

	/* Clean up. We out. */
	FACTAudioEngine_ShutDown(engine);

	return 0;
}
