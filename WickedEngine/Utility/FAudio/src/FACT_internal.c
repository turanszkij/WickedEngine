/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2024 Ethan Lee, Luigi Auriemma, and the MonoGame Team
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

#include "FACT_internal.h"
#include "FAudioFX.h"

/* RNG */

#define STB_EXTERN
#define STB_DEFINE
#include "stb.h"
#define FACT_INTERNAL_rng() ((float) stb_frand())

/* XACT Versions */

#define FACT_CONTENT_VERSION_3_4 45
#define FACT_CONTENT_VERSION_3_1 44
#define FACT_CONTENT_VERSION_3_0 43
#define FACT_CONTENT_VERSION_2_4 41
#define FACT_CONTENT_VERSION_2_0 37

static inline int FACT_INTERNAL_SupportedContent(uint16_t version)
{
	return (	version == FACT_CONTENT_VERSION ||
			version == FACT_CONTENT_VERSION_3_4 ||
			version == FACT_CONTENT_VERSION_3_1 ||
			version == FACT_CONTENT_VERSION_3_0	);
}

#define WAVEBANK_HEADER_VERSION		44
#define WAVEBANK_HEADER_VERSION_3_4	43
#define WAVEBANK_HEADER_VERSION_3_1	42

static inline int FACT_INTERNAL_SupportedWBContent(uint16_t version)
{
	return (	version == WAVEBANK_HEADER_VERSION ||
			version == WAVEBANK_HEADER_VERSION_3_4 ||
			version == WAVEBANK_HEADER_VERSION_3_1	);
}

/* Helper Functions */

static inline float FACT_INTERNAL_CalculateAmplitudeRatio(float decibel)
{
	return (float) FAudio_pow(10.0, decibel / 2000.0);
}

static inline float FACT_INTERNAL_CalculateFilterFrequency(
	float desiredFrequency,
	uint32_t sampleRate
) {
	/* This is needed to convert linear frequencies to the value
	 * FAudio_INTERNAL_FilterVoice expects, in order for it to actually
	 * filter at the correct frequency.
	 *
	 * The formula is...
	 *
	 * (2 * sin(pi * (desired filter cutoff frequency) / sampleRate))
	 *
	 * ... but it behaves badly as the filter frequency gets too high as a
	 * fraction of the sample rate, hence the mins.
	 *
	 * -@Woflox
	 */
	float freq = 2.0f * FAudio_sinf(
		F3DAUDIO_PI *
		FAudio_min(desiredFrequency / sampleRate, 0.5f)
	);
	return FAudio_min(freq, 1.0f);
}

static inline void FACT_INTERNAL_ReadFile(
	FACTReadFileCallback pReadFile,
	FACTGetOverlappedResultCallback pGetOverlappedResult,
	void* io,
	uint32_t offset,
	uint32_t packetSize,
	uint8_t **packetBuffer,
	uint32_t *packetBufferLen,
	FAudioReallocFunc pRealloc,
	void* dst,
	uint32_t len
) {
	FACTOverlapped ovlp;
	uint32_t realOffset, realLen, offPacket, lenPacket, result;
	uint8_t usePacketBuffer;
	void *buf;

	ovlp.Internal = NULL;
	ovlp.InternalHigh = NULL;
	ovlp.OffsetHigh = 0; /* I sure hope so... */
	ovlp.hEvent = NULL;

	/* We have to read data in multiples of the sector size, or else
	 * Win32 ReadFile returns ERROR_INVALID_PARAMETER
	 */
	realOffset = offset;
	realLen = len;
	usePacketBuffer = 0;
	if (packetSize > 0)
	{
		offPacket = realOffset % packetSize;
		if (offPacket > 0)
		{
			usePacketBuffer = 1;
			realOffset -= offPacket;
			realLen += offPacket;
		}
		lenPacket = realLen % packetSize;
		if (lenPacket > 0)
		{
			usePacketBuffer = 1;
			realLen += (packetSize - lenPacket);
		}
	}

	/* If we're compensating for sector alignment, use a temp buffer and copy to
	 * the real destination after we're finished.
	 */
	if (usePacketBuffer)
	{
		if (*packetBufferLen < realLen)
		{
			*packetBufferLen = realLen;
			*packetBuffer = pRealloc(*packetBuffer, realLen);
		}
		buf = *packetBuffer;
	}
	else
	{
		buf = dst;
	}

	/* Read, finally. */
	ovlp.Offset = realOffset;
	if (!pReadFile(io, buf, realLen, NULL, &ovlp))
	{
		while (ovlp.Internal == (void*) 0x103) /* STATUS_PENDING */
		{
			/* Don't actually sleep, just yield the thread */
			FAudio_sleep(0);
		}
	}
	pGetOverlappedResult(io, &ovlp, &result, 1);

	/* Copy the subregion that we actually care about, if applicable */
	if (usePacketBuffer)
	{
		FAudio_memcpy(dst, *packetBuffer + offPacket, len);
	}
}

/* Internal Functions */

void FACT_INTERNAL_GetNextWave(
	FACTCue *cue,
	FACTSound *sound,
	FACTTrack *track,
	FACTTrackInstance *trackInst,
	FACTEvent *evt,
	FACTEventInstance *evtInst
) {
	FAudioSendDescriptor reverbDesc[2];
	FAudioVoiceSends reverbSends;
	const char *wbName;
	FACTWaveBank *wb = NULL;
	LinkedList *list;
	uint16_t wbTrack;
	uint8_t wbIndex;
	uint8_t loopCount = 0;
	float max, next;
	uint8_t noTrackVariation = 1;
	uint32_t i;

	/* Track Variation */
	if (evt->wave.isComplex)
	{
		if (	trackInst->activeWave.wave == NULL ||
			!(evt->wave.complex.variation & 0x00F0)	)
		{
			/* No-op, no variation on loop */
		}
		/* Ordered, Ordered From Random */
		else if (	(evt->wave.complex.variation & 0xF) == 0 ||
				(evt->wave.complex.variation & 0xF) == 1	)
		{
			evtInst->valuei += 1;
			if (evtInst->valuei >= evt->wave.complex.trackCount)
			{
				evtInst->valuei = 0;
			}
		}
		/* Random */
		else if ((evt->wave.complex.variation & 0xF) == 2)
		{
			max = 0.0f;
			for (i = 0; i < evt->wave.complex.trackCount; i += 1)
			{
				max += evt->wave.complex.weights[i];
			}
			next = FACT_INTERNAL_rng() * max;
			for (i = evt->wave.complex.trackCount; i > 0; i -= 1)
			{
				if (next > (max - evt->wave.complex.weights[i - 1]))
				{
					evtInst->valuei = i - 1;
					break;
				}
				max -= evt->wave.complex.weights[i - 1];
			}
		}
		/* Random (No Immediate Repeats), Shuffle */
		else if (	(evt->wave.complex.variation & 0xF) == 3 ||
				(evt->wave.complex.variation & 0xF) == 4	)
		{
			max = 0.0f;
			for (i = 0; i < evt->wave.complex.trackCount; i += 1)
			{
				if (i == evtInst->valuei)
				{
					continue;
				}
				max += evt->wave.complex.weights[i];
			}
			next = FACT_INTERNAL_rng() * max;
			for (i = evt->wave.complex.trackCount; i > 0; i -= 1)
			{
				if (i - 1 == evtInst->valuei)
				{
					continue;
				}
				if (next > (max - evt->wave.complex.weights[i - 1]))
				{
					evtInst->valuei = i - 1;
					break;
				}
				max -= evt->wave.complex.weights[i - 1];
			}
		}

		if (evt->wave.complex.variation & 0x00F0)
		{
			noTrackVariation = 0;
		}

		wbIndex = evt->wave.complex.wavebanks[evtInst->valuei];
		wbTrack = evt->wave.complex.tracks[evtInst->valuei];
	}
	else
	{
		wbIndex = evt->wave.simple.wavebank;
		wbTrack = evt->wave.simple.track;
	}
	wbName = cue->parentBank->wavebankNames[wbIndex];
	list = cue->parentBank->parentEngine->wbList;
	while (list != NULL)
	{
		wb = (FACTWaveBank*) list->entry;
		if (FAudio_strcmp(wbName, wb->name) == 0)
		{
			break;
		}
		list = list->next;
	}
	FAudio_assert(wb != NULL);

	/* Generate the Wave */
	if (	evtInst->loopCount == 255 &&
		noTrackVariation &&
		!(evt->wave.variationFlags & 0x0F00)	)
	{
		/* For infinite loops with no variation, let Wave do the work */
		loopCount = 255;
	}
	FACTWaveBank_Prepare(
		wb,
		wbTrack,
		evt->wave.flags,
		0,
		loopCount,
		&trackInst->upcomingWave.wave
	);
	trackInst->upcomingWave.wave->parentCue = cue;
	if (sound->dspCodeCount > 0) /* Never more than 1...? */
	{
		reverbDesc[0].Flags = 0;
		reverbDesc[0].pOutputVoice = cue->parentBank->parentEngine->master;
		reverbDesc[1].Flags = 0;
		reverbDesc[1].pOutputVoice = cue->parentBank->parentEngine->reverbVoice;
		reverbSends.SendCount = 2;
		reverbSends.pSends = reverbDesc;
		FAudioVoice_SetOutputVoices(
			trackInst->upcomingWave.wave->voice,
			&reverbSends
		);
	}

	/* 3D Audio */
	if (cue->active3D)
	{
		FACTWave_SetMatrixCoefficients(
			trackInst->upcomingWave.wave,
			cue->srcChannels,
			cue->dstChannels,
			cue->matrixCoefficients
		);
	}
	else
	{
		/* TODO: Position/Angle/UseCenterSpeaker */
	}

	/* Pitch Variation */
	if (evt->wave.variationFlags & 0x1000)
	{
		const int16_t rngPitch = (int16_t) (
			FACT_INTERNAL_rng() *
			(evt->wave.maxPitch - evt->wave.minPitch)
		) + evt->wave.minPitch;
		if (trackInst->activeWave.wave != NULL)
		{
			/* Variation on Loop */
			if (evt->wave.variationFlags & 0x0100)
			{
				/* Add/Replace */
				if (evt->wave.variationFlags & 0x0004)
				{
					trackInst->upcomingWave.basePitch =
						trackInst->activeWave.basePitch + rngPitch;
				}
				else
				{
					trackInst->upcomingWave.basePitch = rngPitch + sound->pitch;
				}
			}
		}
		else
		{
			/* Initial Pitch Variation */
			trackInst->upcomingWave.basePitch = rngPitch + sound->pitch;
		}
	}
	else
	{
		trackInst->upcomingWave.basePitch = sound->pitch;
	}

	/* Volume Variation */
	if (evt->wave.variationFlags & 0x2000)
	{
		const float rngVolume = (
			FACT_INTERNAL_rng() *
			(evt->wave.maxVolume - evt->wave.minVolume)
		) + evt->wave.minVolume;
		if (trackInst->activeWave.wave != NULL)
		{
			/* Variation on Loop */
			if (evt->wave.variationFlags & 0x0200)
			{
				/* Add/Replace */
				if (evt->wave.variationFlags & 0x0001)
				{
					trackInst->upcomingWave.baseVolume =
						trackInst->activeWave.baseVolume + rngVolume;
				}
				else
				{
					trackInst->upcomingWave.baseVolume = (
						rngVolume +
						sound->volume +
						track->volume
					);
				}
			}
		}
		else
		{
			/* Initial Volume Variation */
			trackInst->upcomingWave.baseVolume = (
				rngVolume +
				sound->volume +
				track->volume
			);
		}
	}
	else
	{
		trackInst->upcomingWave.baseVolume = sound->volume + track->volume;
	}

	/* Filter Variation, QFactor/Freq are always together */
	if (evt->wave.variationFlags & 0xC000)
	{
		const float rngQFactor = 1.0f / (
			FACT_INTERNAL_rng() *
			(evt->wave.maxQFactor - evt->wave.minQFactor) +
			evt->wave.minQFactor
		);
		const float rngFrequency = FACT_INTERNAL_CalculateFilterFrequency(
			(
				FACT_INTERNAL_rng() *
				(evt->wave.maxFrequency - evt->wave.minFrequency) +
				evt->wave.minFrequency
			),
			cue->parentBank->parentEngine->audio->master->master.inputSampleRate
		);
		if (trackInst->activeWave.wave != NULL)
		{
			/* Variation on Loop */
			if (evt->wave.variationFlags & 0x0C00)
			{
				/* TODO: Add/Replace */
				/* FIXME: Which is QFactor/Freq?
				if (evt->wave.variationFlags & 0x0010)
				{
				}
				else
				{
				}
				if (evt->wave.variationFlags & 0x0040)
				{
				}
				else
				{
				}
				*/
				trackInst->upcomingWave.baseQFactor = rngQFactor;
				trackInst->upcomingWave.baseFrequency = rngFrequency;
			}
		}
		else
		{
			/* Initial Filter Variation */
			trackInst->upcomingWave.baseQFactor = rngQFactor;
			trackInst->upcomingWave.baseFrequency = rngFrequency;
		}
	}
	else
	{
		trackInst->upcomingWave.baseQFactor = 1.0f / (track->qfactor / 3.0f);
		trackInst->upcomingWave.baseFrequency = FACT_INTERNAL_CalculateFilterFrequency(
			track->frequency,
			cue->parentBank->parentEngine->audio->master->master.inputSampleRate
		);

		/* FIXME: For some reason the 0.67 Q Factor causes problems, but it's also
		 * the only possible value until ~1 so just clamp it for now.
		 */
		trackInst->upcomingWave.baseQFactor = FAudio_min(
			trackInst->upcomingWave.baseQFactor,
			1.0f
		);
	}

	/* Try to change loop counter at the very end */
	if (loopCount == 255)
	{
		/* For infinite loops with no variation, Wave does the work */
		evtInst->loopCount = 0;
	}
	else if (evtInst->loopCount > 0)
	{
		evtInst->loopCount -= 1;
	}
}

uint8_t FACT_INTERNAL_CreateSound(FACTCue *cue, uint16_t fadeInMS)
{
	int32_t i, j, k;
	float max, next, weight;
	const char *wbName;
	FACTWaveBank *wb = NULL;
	LinkedList *list;
	FACTEvent *evt;
	FACTEventInstance *evtInst;
	FACTSound *baseSound = NULL;
	FACTSoundInstance *newSound;
	FACTRPC *rpc;
	float lastX;

	union
	{
		float maxf;
		uint8_t maxi;
	} limitmax;
	FACTCue *tmp, *wnr;
	uint16_t categoryIndex;
	FACTAudioCategory *category;

	if (cue->data->flags & 0x04)
	{
		/* Sound */
		baseSound = cue->sound;
	}
	else if (cue->variation)
	{
		/* Variation */
		if (cue->variation->flags == 3)
		{
			/* Interactive */
			if (cue->parentBank->parentEngine->variables[cue->variation->variable].accessibility & 0x04)
			{
				FACTCue_GetVariable(
					cue,
					cue->variation->variable,
					&next
				);
			}
			else
			{
				FACTAudioEngine_GetGlobalVariable(
					cue->parentBank->parentEngine,
					cue->variation->variable,
					&next
				);
			}
			for (i = 0; i < cue->variation->entryCount; i += 1)
			{
				if (	next <= cue->variation->entries[i].maxWeight &&
					next >= cue->variation->entries[i].minWeight	)
				{
					break;
				}
			}

			/* This should only happen when the user control
			 * variable is none of the sound probabilities, in
			 * which case we are just silent. But, we should still
			 * claim to be "playing" in the meantime.
			 */
			if (i == cue->variation->entryCount)
			{
				return 1;
			}
		}
		else
		{
			/* Random */
			max = 0.0f;
			for (i = 0; i < cue->variation->entryCount; i += 1)
			{
				max += (
					cue->variation->entries[i].maxWeight -
					cue->variation->entries[i].minWeight
				);
			}
			next = FACT_INTERNAL_rng() * max;

			/* Use > 0, not >= 0. If we hit 0, that's it! */
			for (i = cue->variation->entryCount - 1; i > 0; i -= 1)
			{
				weight = (
					cue->variation->entries[i].maxWeight -
					cue->variation->entries[i].minWeight
				);
				if (next > (max - weight))
				{
					break;
				}
				max -= weight;
			}
		}

		if (cue->variation->isComplex)
		{
			/* Grab the Sound via the code. FIXME: Do this at load time? */
			for (j = 0; j < cue->parentBank->soundCount; j += 1)
			{
				if (cue->variation->entries[i].soundCode == cue->parentBank->soundCodes[j])
				{
					baseSound = &cue->parentBank->sounds[j];
					break;
				}
			}
		}
		else
		{
			/* Pull in the WaveBank... */
			wbName = cue->parentBank->wavebankNames[
				cue->variation->entries[i].simple.wavebank
			];
			list = cue->parentBank->parentEngine->wbList;
			while (list != NULL)
			{
				wb = (FACTWaveBank*) list->entry;
				if (FAudio_strcmp(wbName, wb->name) == 0)
				{
					break;
				}
				list = list->next;
			}
			FAudio_assert(wb != NULL);

			/* Generate the wave... */
			FACTWaveBank_Prepare(
				wb,
				cue->variation->entries[i].simple.track,
				0,
				0,
				0,
				&cue->simpleWave
			);
			cue->simpleWave->parentCue = cue;
		}
	}

	/* Alloc SoundInstance variables */
	if (baseSound != NULL)
	{
		/* Category Instance Limits */
		categoryIndex = baseSound->category;
		if (categoryIndex != FACTCATEGORY_INVALID)
		{
			category = &cue->parentBank->parentEngine->categories[categoryIndex];
			if (category->instanceCount >= category->instanceLimit)
			{
				wnr = NULL;
				tmp = cue->parentBank->cueList;
				if (category->maxInstanceBehavior == 0) /* Fail */
				{
					cue->state |= FACT_STATE_STOPPED;
					cue->state &= ~(
						FACT_STATE_PLAYING |
						FACT_STATE_STOPPING |
						FACT_STATE_PAUSED
					);
					return 0;
				}
				else if (category->maxInstanceBehavior == 1) /* Queue */
				{
					/* FIXME: How is this different from Replace Oldest? */
					while (tmp != NULL)
					{
						if (	tmp != cue &&
							tmp->playingSound != NULL &&
							tmp->playingSound->sound->category == categoryIndex &&
							!(tmp->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))	)
						{
							wnr = tmp;
							break;
						}
						tmp = tmp->next;
					}
				}
				else if (category->maxInstanceBehavior == 2) /* Replace Oldest */
				{
					while (tmp != NULL)
					{
						if (	tmp != cue &&
							tmp->playingSound != NULL &&
							tmp->playingSound->sound->category == categoryIndex &&
							!(tmp->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))	)
						{
							wnr = tmp;
							break;
						}
						tmp = tmp->next;
					}
				}
				else if (category->maxInstanceBehavior == 3) /* Replace Quietest */
				{
					limitmax.maxf = FACTVOLUME_MAX;
					while (tmp != NULL)
					{
						if (	tmp != cue &&
							tmp->playingSound != NULL &&
							tmp->playingSound->sound->category == categoryIndex &&
							/*FIXME: tmp->playingSound->volume < limitmax.maxf &&*/
							!(tmp->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))	)
						{
							wnr = tmp;
							/* limitmax.maxf = tmp->playingSound->volume; */
						}
						tmp = tmp->next;
					}
				}
				else if (category->maxInstanceBehavior == 4) /* Replace Lowest Priority */
				{
					limitmax.maxi = 0xFF;
					while (tmp != NULL)
					{
						if (	tmp != cue &&
							tmp->playingSound != NULL &&
							tmp->playingSound->sound->category == categoryIndex &&
							tmp->playingSound->sound->priority < limitmax.maxi &&
							!(tmp->state & (FACT_STATE_STOPPING | FACT_STATE_STOPPED))	)
						{
							wnr = tmp;
							limitmax.maxi = tmp->playingSound->sound->priority;
						}
						tmp = tmp->next;
					}
				}
				if (wnr != NULL)
				{
					fadeInMS = category->fadeInMS;
					if (wnr->playingSound != NULL)
					{
						FACT_INTERNAL_BeginFadeOut(wnr->playingSound, category->fadeOutMS);
					}
					else
					{
						FACTCue_Stop(wnr, 0);
					}
				}
			}
			category->instanceCount += 1;
		}

		newSound = (FACTSoundInstance*) cue->parentBank->parentEngine->pMalloc(
			sizeof(FACTSoundInstance)
		);
		newSound->parentCue = cue;
		newSound->sound = baseSound;
		newSound->rpcData.rpcVolume = 0.0f;
		newSound->rpcData.rpcPitch = 0.0f;
		newSound->rpcData.rpcReverbSend = 0.0f;
		newSound->rpcData.rpcFilterQFactor = FAUDIO_DEFAULT_FILTER_ONEOVERQ;
		newSound->rpcData.rpcFilterFreq = FAUDIO_DEFAULT_FILTER_FREQUENCY;
		newSound->fadeType = (fadeInMS > 0);
		if (newSound->fadeType)
		{
			newSound->fadeStart = FAudio_timems();
			newSound->fadeTarget = fadeInMS;
		}
		else
		{
			newSound->fadeStart = 0;
			newSound->fadeTarget = 0;
		}
		newSound->tracks = (FACTTrackInstance*) cue->parentBank->parentEngine->pMalloc(
			sizeof(FACTTrackInstance) * newSound->sound->trackCount
		);
		for (i = 0; i < newSound->sound->trackCount; i += 1)
		{
			newSound->tracks[i].rpcData.rpcVolume = 0.0f;
			newSound->tracks[i].rpcData.rpcPitch = 0.0f;
			newSound->tracks[i].rpcData.rpcReverbSend = 0.0f;
			newSound->tracks[i].rpcData.rpcFilterQFactor = FAUDIO_DEFAULT_FILTER_ONEOVERQ;
			newSound->tracks[i].rpcData.rpcFilterFreq = FAUDIO_DEFAULT_FILTER_FREQUENCY;

			newSound->tracks[i].evtVolume = 0.0f;
			newSound->tracks[i].evtPitch = 0.0f;

			newSound->tracks[i].activeWave.wave = NULL;
			newSound->tracks[i].activeWave.baseVolume = 0.0f;
			newSound->tracks[i].activeWave.basePitch = 0;
			newSound->tracks[i].activeWave.baseQFactor = FAUDIO_DEFAULT_FILTER_ONEOVERQ;
			newSound->tracks[i].activeWave.baseFrequency = FAUDIO_DEFAULT_FILTER_FREQUENCY;
			newSound->tracks[i].upcomingWave.wave = NULL;
			newSound->tracks[i].upcomingWave.baseVolume = 0.0f;
			newSound->tracks[i].upcomingWave.basePitch = 0;
			newSound->tracks[i].upcomingWave.baseQFactor = FAUDIO_DEFAULT_FILTER_ONEOVERQ;
			newSound->tracks[i].upcomingWave.baseFrequency = FAUDIO_DEFAULT_FILTER_FREQUENCY;

			newSound->tracks[i].events = (FACTEventInstance*) cue->parentBank->parentEngine->pMalloc(
				sizeof(FACTEventInstance) * newSound->sound->tracks[i].eventCount
			);
			for (j = 0; j < newSound->sound->tracks[i].eventCount; j += 1)
			{
				evt = &newSound->sound->tracks[i].events[j];

				newSound->tracks[i].events[j].timestamp =
					newSound->sound->tracks[i].events[j].timestamp;
				newSound->tracks[i].events[j].loopCount = 0;
				newSound->tracks[i].events[j].finished = 0;
				newSound->tracks[i].events[j].value = 0.0f;

				if (	evt->type == FACTEVENT_PLAYWAVE ||
					evt->type == FACTEVENT_PLAYWAVETRACKVARIATION ||
					evt->type == FACTEVENT_PLAYWAVEEFFECTVARIATION ||
					evt->type == FACTEVENT_PLAYWAVETRACKEFFECTVARIATION	)
				{
					newSound->tracks[i].events[j].loopCount =
						newSound->sound->tracks[i].events[j].wave.loopCount;

					evtInst = &newSound->tracks[i].events[j];
					if (	!evt->wave.isComplex ||
						(evt->wave.complex.variation & 0xF) == 0	)
					{
						evtInst->valuei = 0;
					}
					else
					{
						max = 0.0f;
						for (k = 0; k < evt->wave.complex.trackCount; k += 1)
						{
							max += evt->wave.complex.weights[k];
						}
						next = FACT_INTERNAL_rng() * max;
						for (k = evt->wave.complex.trackCount - 1; k >= 0; k -= 1)
						{
							if (next > (max - evt->wave.complex.weights[k]))
							{
								evtInst->valuei = k;
								break;
							}
							max -= evt->wave.complex.weights[k];
						}
					}
					FACT_INTERNAL_GetNextWave(
						cue,
						newSound->sound,
						&newSound->sound->tracks[i],
						&newSound->tracks[i],
						evt,
						evtInst
					);
					newSound->tracks[i].waveEvt = evt;
					newSound->tracks[i].waveEvtInst = evtInst;
				}
				else if (	evt->type == FACTEVENT_PITCHREPEATING ||
						evt->type == FACTEVENT_VOLUMEREPEATING	)
				{
					newSound->tracks[i].events[j].loopCount =
						newSound->sound->tracks[i].events[j].value.repeats;
				}
				else if (evt->type == FACTEVENT_MARKERREPEATING)
				{
					newSound->tracks[i].events[j].loopCount =
						newSound->sound->tracks[i].events[j].marker.repeats;
				}
			}
		}

		/* Calculate Max RPC Release Time */
		cue->maxRpcReleaseTime = 0;
		for (i = 0; i < newSound->sound->trackCount; i += 1)
		{
			for (j = 0; j < newSound->sound->tracks[i].rpcCodeCount; j += 1)
			{
				rpc = FACT_INTERNAL_GetRPC(
					newSound->parentCue->parentBank->parentEngine,
					newSound->sound->tracks[i].rpcCodes[j]
				);
				if (	rpc->parameter == RPC_PARAMETER_VOLUME &&
					cue->parentBank->parentEngine->variables[rpc->variable].accessibility & 0x04	)
				{
					if (FAudio_strcmp(
						newSound->parentCue->parentBank->parentEngine->variableNames[rpc->variable],
						"ReleaseTime"
					) == 0) {
						lastX = rpc->points[rpc->pointCount - 1].x;
						if (lastX > cue->maxRpcReleaseTime)
						{
							cue->maxRpcReleaseTime = (uint32_t) lastX /* bleh */;
						}
					}
				}
			}
		}

		cue->playingSound = newSound;
	}

	return 1;
}

void FACT_INTERNAL_SendCueNotification(FACTCue *cue, FACTNoticationsFlags flag, uint8_t type)
{
    if (cue->parentBank->parentEngine->notifications & flag)
    {
        FACTNotification note;

        note.type = type;
        note.pvContext = cue->parentBank->parentEngine->cue_context;
        note.cue.cueIndex = cue->index;
        note.cue.pSoundBank = cue->parentBank;
        note.cue.pCue = cue;

        cue->parentBank->parentEngine->notificationCallback(&note);
    }
}

void FACT_INTERNAL_DestroySound(FACTSoundInstance *sound)
{
	uint8_t i;

	sound->parentCue->playingSound = NULL;
	for (i = 0; i < sound->sound->trackCount; i += 1)
	{
		if (sound->tracks[i].activeWave.wave != NULL)
		{
			FACTWave_Destroy(
				sound->tracks[i].activeWave.wave
			);
		}
		if (sound->tracks[i].upcomingWave.wave != NULL)
		{
			FACTWave_Destroy(
				sound->tracks[i].upcomingWave.wave
			);
		}
		sound->parentCue->parentBank->parentEngine->pFree(
			sound->tracks[i].events
		);
	}
	sound->parentCue->parentBank->parentEngine->pFree(sound->tracks);

	if (sound->sound->category != FACTCATEGORY_INVALID)
	{
		sound->parentCue->parentBank->parentEngine->categories[
			sound->sound->category
		].instanceCount -= 1;
	}

	/* TODO: if (sound->parentCue->playingSounds == NULL) */
	{
		sound->parentCue->state |= FACT_STATE_STOPPED;
		sound->parentCue->state &= ~(FACT_STATE_PLAYING | FACT_STATE_STOPPING);
		sound->parentCue->data->instanceCount -= 1;

		FACT_INTERNAL_SendCueNotification(sound->parentCue, NOTIFY_CUESTOP, FACTNOTIFICATIONTYPE_CUESTOP);
	}
	sound->parentCue->parentBank->parentEngine->pFree(sound);
}

void FACT_INTERNAL_BeginFadeOut(FACTSoundInstance *sound, uint16_t fadeOutMS)
{
	if (fadeOutMS == 0)
	{
		/* No fade? Screw it, just delete us */
		FACT_INTERNAL_DestroySound(sound);
		return;
	}

	sound->fadeType = 2; /* Out */
	sound->fadeStart = FAudio_timems();
	sound->fadeTarget = fadeOutMS;

	sound->parentCue->state |= FACT_STATE_STOPPING;
}

void FACT_INTERNAL_BeginReleaseRPC(FACTSoundInstance *sound, uint16_t releaseMS)
{
	if (releaseMS == 0)
	{
		/* No release RPC? Screw it, just delete us */
		FACT_INTERNAL_DestroySound(sound);
		return;
	}

	sound->fadeType = 3; /* Release RPC */
	sound->fadeStart = FAudio_timems();
	sound->fadeTarget = releaseMS;

	sound->parentCue->state |= FACT_STATE_STOPPING;
}

/* RPC Helper Functions */

FACTRPC* FACT_INTERNAL_GetRPC(
	FACTAudioEngine *engine,
	uint32_t code
) {
	uint16_t i;
	for (i = 0; i < engine->rpcCount; i += 1)
	{
		if (engine->rpcCodes[i] == code)
		{
			return &engine->rpcs[i];
		}
	}

	FAudio_assert(0 && "RPC code not found!");
	return NULL;
}

float FACT_INTERNAL_CalculateRPC(
	FACTRPC *rpc,
	float var
) {
	float result;
	uint8_t i;

	/* Min/Max */
	if (var <= rpc->points[0].x)
	{
		/* Zero to first defined point */
		return rpc->points[0].y;
	}
	if (var >= rpc->points[rpc->pointCount - 1].x)
	{
		/* Last defined point to infinity */
		return rpc->points[rpc->pointCount - 1].y;
	}

	/* Something between points */
	result = 0.0f;
	for (i = 0; i < rpc->pointCount - 1; i += 1)
	{
		/* y = b */
		result = rpc->points[i].y;
		if (var >= rpc->points[i].x && var <= rpc->points[i + 1].x)
		{
			const float maxX = rpc->points[i + 1].x - rpc->points[i].x;
			const float maxY = rpc->points[i + 1].y - rpc->points[i].y;
			const float deltaX = (var - rpc->points[i].x);
			const float deltaXNormalized = deltaX / maxX;

			if (rpc->points[i].type == 0) /* Linear */
			{
				result += maxY * deltaXNormalized;
			}
			else if (rpc->points[i].type == 1) /* Fast */
			{
				result += maxY * (1.0f - FAudio_powf(1.0f - FAudio_powf(deltaXNormalized, 1.0f / 1.5f), 1.5f));
			}
			else if (rpc->points[i].type == 2) /* Slow */
			{
				result += maxY * (1.0f - FAudio_powf(1.0f - FAudio_powf(deltaXNormalized, 1.5f), 1.0f / 1.5f));
			}
			else if (rpc->points[i].type == 3) /* SinCos */
			{
				if (maxY > 0.0f)
				{
					result += maxY * (1.0f - FAudio_powf(1.0f - FAudio_sqrtf(deltaXNormalized), 2.0f));
				}
				else
				{
					result += maxY * (1.0f - FAudio_sqrtf(1.0f - FAudio_powf(deltaXNormalized, 2.0f)));
				}
			}
			else
			{
				FAudio_assert(0 && "Unrecognized curve type!");
			}

			break;
		}
	}
	return result;
}

void FACT_INTERNAL_UpdateRPCs(
	FACTCue *cue,
	uint8_t codeCount,
	uint32_t *codes,
	FACTInstanceRPCData *data,
	uint32_t timestamp,
	uint32_t elapsedTrack
) {
	uint8_t i;
	FACTRPC *rpc;
	float rpcResult;
	float variableValue;
	FACTAudioEngine *engine = cue->parentBank->parentEngine;

	if (codeCount > 0)
	{
		/* Do NOT overwrite Frequency/QFactor! */
		data->rpcVolume = 0.0f;
		data->rpcPitch = 0.0f;
		data->rpcReverbSend = 0.0f;
		for (i = 0; i < codeCount; i += 1)
		{
			rpc = FACT_INTERNAL_GetRPC(
				engine,
				codes[i]
			);
			if (engine->variables[rpc->variable].accessibility & 0x04)
			{
				if (FAudio_strcmp(
					engine->variableNames[rpc->variable],
					"AttackTime"
				) == 0) {
					variableValue = (float) elapsedTrack;
				}
				else if (FAudio_strcmp(
					engine->variableNames[rpc->variable],
					"ReleaseTime"
				) == 0) {
					if (cue->playingSound->fadeType == 3) /* Release RPC */
					{
						variableValue = (float) (timestamp - cue->playingSound->fadeStart);
					}
					else
					{
						variableValue = 0.0f;
					}
				}
				else
				{
					variableValue = cue->variableValues[rpc->variable];
				}

				rpcResult = FACT_INTERNAL_CalculateRPC(
					rpc,
					variableValue
				);
			}
			else
			{
				rpcResult = FACT_INTERNAL_CalculateRPC(
					rpc,
					engine->globalVariableValues[rpc->variable]
				);
			}
			if (rpc->parameter == RPC_PARAMETER_VOLUME)
			{
				data->rpcVolume += rpcResult;
			}
			else if (rpc->parameter == RPC_PARAMETER_PITCH)
			{
				data->rpcPitch += rpcResult;
			}
			else if (rpc->parameter == RPC_PARAMETER_REVERBSEND)
			{
				data->rpcReverbSend += rpcResult;
			}
			else if (rpc->parameter == RPC_PARAMETER_FILTERFREQUENCY)
			{
				/* Yes, just overwrite... */
				data->rpcFilterFreq = FACT_INTERNAL_CalculateFilterFrequency(
					rpcResult,
					engine->audio->master->master.inputSampleRate
				);
			}
			else if (rpc->parameter == RPC_PARAMETER_FILTERQFACTOR)
			{
				/* Yes, just overwrite... */
				data->rpcFilterQFactor = 1.0f / rpcResult;
			}
			else
			{
				FAudio_assert(0 && "Unhandled RPC parameter type!");
			}
		}
	}
}

/* Engine Update Function */

void FACT_INTERNAL_UpdateEngine(FACTAudioEngine *engine)
{
	FAudioFXReverbParameters rvbPar;
	uint16_t i, j, par;
	float rpcResult;
	for (i = 0; i < engine->rpcCount; i += 1)
	{
		if (engine->rpcs[i].parameter >= RPC_PARAMETER_COUNT)
		{
			/* FIXME: Why did I make this global vars only...? */
			if (!(engine->variables[engine->rpcs[i].variable].accessibility & 0x04))
			{
				for (j = 0; j < engine->dspPresetCount; j += 1)
				{
					/* FIXME: This affects all DSP presets!
					 * What if there's more than one?
					 */
					par = engine->rpcs[i].parameter - RPC_PARAMETER_COUNT;
					rpcResult = FACT_INTERNAL_CalculateRPC(
						&engine->rpcs[i],
						engine->globalVariableValues[engine->rpcs[i].variable]
					);
					engine->dspPresets[j].parameters[par].value = FAudio_clamp(
						rpcResult,
						engine->dspPresets[j].parameters[par].minVal,
						engine->dspPresets[j].parameters[par].maxVal
					);
				}
			}
		}
	}

	/* Set Effect parameters from above RPC changes */
	if (engine->reverbVoice != NULL)
	{
		rvbPar.WetDryMix = engine->dspPresets[0].parameters[21].value;
		rvbPar.ReflectionsDelay = (uint32_t) engine->dspPresets[0].parameters[0].value;
		rvbPar.ReverbDelay = (uint8_t) engine->dspPresets[0].parameters[1].value;
		rvbPar.RearDelay = (uint8_t) engine->dspPresets[0].parameters[12].value;
		rvbPar.PositionLeft = (uint8_t) engine->dspPresets[0].parameters[2].value;
		rvbPar.PositionRight = (uint8_t) engine->dspPresets[0].parameters[3].value;
		rvbPar.PositionMatrixLeft = (uint8_t) engine->dspPresets[0].parameters[4].value;
		rvbPar.PositionMatrixRight = (uint8_t) engine->dspPresets[0].parameters[5].value;
		rvbPar.HighEQGain = (uint8_t) engine->dspPresets[0].parameters[10].value;
		rvbPar.LowEQCutoff = (uint8_t) engine->dspPresets[0].parameters[9].value;
		rvbPar.LowEQGain = (uint8_t) engine->dspPresets[0].parameters[8].value;
		rvbPar.LateDiffusion = (uint8_t) engine->dspPresets[0].parameters[7].value;
		rvbPar.EarlyDiffusion = (uint8_t) engine->dspPresets[0].parameters[6].value;
		rvbPar.HighEQCutoff = (uint8_t) engine->dspPresets[0].parameters[11].value;
		rvbPar.RoomFilterMain = engine->dspPresets[0].parameters[14].value;
		rvbPar.RoomFilterFreq = engine->dspPresets[0].parameters[13].value;
		rvbPar.RoomFilterHF = engine->dspPresets[0].parameters[15].value;
		rvbPar.ReflectionsGain = engine->dspPresets[0].parameters[16].value;
		rvbPar.ReverbGain = engine->dspPresets[0].parameters[17].value;
		rvbPar.DecayTime = engine->dspPresets[0].parameters[18].value;
		rvbPar.Density = engine->dspPresets[0].parameters[19].value;
		rvbPar.RoomSize = engine->dspPresets[0].parameters[20].value;

		FAudioVoice_SetEffectParameters(
			engine->reverbVoice,
			0,
			&rvbPar,
			sizeof(FAudioFXReverbParameters),
			0
		);
	}
}

/* Cue Update Functions */

static inline void FACT_INTERNAL_StopTrack(
	FACTTrack *track,
	FACTTrackInstance *trackInst,
	uint8_t immediate
) {
	uint8_t i;

	/* Stop the wave (may as-authored or immedate */
	if (trackInst->activeWave.wave != NULL)
	{
		FACTWave_Stop(
			trackInst->activeWave.wave,
			immediate
		);
	}

	/* If there was another sound coming, it ain't now! */
	if (trackInst->upcomingWave.wave != NULL)
	{
		FACTWave_Destroy(trackInst->upcomingWave.wave);
		trackInst->upcomingWave.wave = NULL;
	}

	/* Kill the loop count too */
	for (i = 0; i < track->eventCount; i += 1)
	{
		trackInst->events[i].loopCount = 0;
		trackInst->events[i].finished = 1;
	}
}

void FACT_INTERNAL_ActivateEvent(
	FACTSoundInstance *sound,
	FACTTrack *track,
	FACTTrackInstance *trackInst,
	FACTEvent *evt,
	FACTEventInstance *evtInst,
	uint32_t elapsed
) {
	uint8_t i;
	float svResult;
	uint8_t skipLoopCheck = 0;

	/* STOP */
	if (evt->type == FACTEVENT_STOP)
	{
		/* Stop Cue */
		if (evt->stop.flags & 0x02)
		{
			if (	evt->stop.flags & 0x01 ||
				(	sound->parentCue->parentBank->cues[sound->parentCue->index].fadeOutMS == 0 &&
					sound->parentCue->maxRpcReleaseTime == 0	)	)
			{
				for (i = 0; i < sound->sound->trackCount; i += 1)
				{
					FACT_INTERNAL_StopTrack(
						&sound->sound->tracks[i],
						&sound->tracks[i],
						1
					);
				}
			}
			else
			{
				if (sound->parentCue->parentBank->cues[sound->parentCue->index].fadeOutMS > 0)
				{
					FACT_INTERNAL_BeginFadeOut(
						sound,
						sound->parentCue->parentBank->cues[sound->parentCue->index].fadeOutMS
					);
				}
				else if (sound->parentCue->maxRpcReleaseTime > 0)
				{
					FACT_INTERNAL_BeginReleaseRPC(
						sound,
						sound->parentCue->maxRpcReleaseTime
					);
				}
				else
				{
					/* Pretty sure this doesn't happen, but just in case? */
					sound->parentCue->state |= FACT_STATE_STOPPING;
				}
			}
		}

		/* Stop track */
		else
		{
			FACT_INTERNAL_StopTrack(
				track,
				trackInst,
				evt->stop.flags & 0x01
			);
		}
	}

	/* PLAYWAVE */
	else if (	evt->type == FACTEVENT_PLAYWAVE ||
			evt->type == FACTEVENT_PLAYWAVETRACKVARIATION ||
			evt->type == FACTEVENT_PLAYWAVEEFFECTVARIATION ||
			evt->type == FACTEVENT_PLAYWAVETRACKEFFECTVARIATION	)
	{
		FAudio_assert(trackInst->activeWave.wave == NULL);
		FAudio_assert(trackInst->upcomingWave.wave != NULL);
		FAudio_memcpy(
			&trackInst->activeWave,
			&trackInst->upcomingWave,
			sizeof(trackInst->activeWave)
		);
		trackInst->upcomingWave.wave = NULL;
		FACTWave_Play(trackInst->activeWave.wave);
	}

	/* SETVALUE */
	else if (	evt->type == FACTEVENT_PITCH ||
			evt->type == FACTEVENT_PITCHREPEATING ||
			evt->type == FACTEVENT_VOLUME ||
			evt->type == FACTEVENT_VOLUMEREPEATING	)
	{
		/* Ramp/Equation */
		if (evt->value.settings & 0x01)
		{
			/* FIXME: Incorporate 2nd derivative into the interpolated pitch (slopeDelta) */
			skipLoopCheck = elapsed <= (evtInst->timestamp + evt->value.ramp.duration);
			svResult = (
				evt->value.ramp.initialSlope *
				evt->value.ramp.duration / 1000 *
				10 /* "Slices" */
			) + evt->value.ramp.initialValue;
			svResult = (
				(svResult - evt->value.ramp.initialValue)
			) * FAudio_clamp(
				(float) (elapsed - evtInst->timestamp) / evt->value.ramp.duration,
				0.0f,
				1.0f
			) + evt->value.ramp.initialValue;

			evtInst->value = svResult;
		}
		else
		{
			/* Value/Random */
			if (evt->value.equation.flags & 0x04)
			{
				svResult = evt->value.equation.value1;
			}
			else if (evt->value.equation.flags & 0x08)
			{
				svResult = evt->value.equation.value1 + FACT_INTERNAL_rng() * (
					evt->value.equation.value2 -
					evt->value.equation.value1
				);
			}
			else
			{
				svResult = 0.0f;
				FAudio_assert(0 && "Equation flags?");
			}

			/* Add/Replace */
			if (evt->value.equation.flags & 0x01)
			{
				if(	evt->type == FACTEVENT_PITCH ||
					evt->type == FACTEVENT_PITCHREPEATING	)
				{
					evtInst->value = trackInst->evtPitch + svResult;
				}
				else
				{
					evtInst->value = trackInst->evtVolume + svResult;
				}
			}
			else
			{
				evtInst->value = svResult;
			}
		}

		/* Set the result, finally. */
		if (	evt->type == FACTEVENT_PITCH ||
			evt->type == FACTEVENT_PITCHREPEATING	)
		{
			trackInst->evtPitch = evtInst->value;
		}
		else
		{
			trackInst->evtVolume = evtInst->value;
		}

		if (skipLoopCheck)
		{
			return;
		}
		if (evtInst->loopCount > 0)
		{
			if (evtInst->loopCount != 0xFF && evtInst->loopCount != 0xFFFF)
			{
				evtInst->loopCount -= 1;
			}

			evtInst->timestamp += evt->value.frequency;
			return;
		}
	}

	/* MARKER */
	else if (	evt->type == FACTEVENT_MARKER ||
			evt->type == FACTEVENT_MARKERREPEATING	)
	{
		/* TODO: FACT_INTERNAL_Marker(evt->marker*) */
		if (evtInst->loopCount > 0)
		{
			if (evtInst->loopCount != 0xFF)
			{
				evtInst->loopCount -= 1;
			}

			evtInst->timestamp += evt->marker.frequency;
			return;
		}
	}

	/* ??? */
	else
	{
		FAudio_assert(0 && "Unknown event type!");
	}

	/* If we made it here, we're done! */
	evtInst->finished = 1;
}

uint8_t FACT_INTERNAL_UpdateSound(FACTSoundInstance *sound, uint32_t timestamp)
{
	uint8_t i, j;
	uint32_t waveState;
	uint32_t elapsedCue;
	FACTEventInstance *evtInst;
	FAudioFilterParameters filterParams;
	uint8_t finished = 1;

	/* Instance limiting Fade in/out */
	float fadeVolume;
	if (sound->fadeType == 1) /* Fade In */
	{
		if ((timestamp - sound->fadeStart) >= sound->fadeTarget)
		{
			/* We've faded in! */
			fadeVolume = 1.0f;
			sound->fadeStart = 0;
			sound->fadeTarget = 0;
			sound->fadeType = 0;
		}
		else
		{
			fadeVolume = (
				(float) (timestamp - sound->fadeStart) /
				(float) sound->fadeTarget
			);
		}
	}
	else if (sound->fadeType == 2) /* Fade Out */
	{
		if ((timestamp - sound->fadeStart) >= sound->fadeTarget)
		{
			/* We've faded out! */
			return 1;
		}
		fadeVolume = 1.0f - (
			(float) (timestamp - sound->fadeStart) /
			(float) sound->fadeTarget
		);
	}
	else if (sound->fadeType == 3) /* Release RPC */
	{
		if ((timestamp - sound->fadeStart) >= sound->fadeTarget)
		{
			/* We've faded out! */
			return 1;
		}
		fadeVolume = 1.0f;
	}
	else
	{
		fadeVolume = 1.0f;
	}

	/* To get the time on a single Cue, subtract from the global time
	 * the latest start time minus the total time elapsed (minus pause time)
	 */
	elapsedCue = timestamp - (sound->parentCue->start - sound->parentCue->elapsed);

	/* RPC updates */
	sound->rpcData.rpcFilterFreq = -1.0f;
	sound->rpcData.rpcFilterQFactor = -1.0f;
	FACT_INTERNAL_UpdateRPCs(
		sound->parentCue,
		sound->sound->rpcCodeCount,
		sound->sound->rpcCodes,
		&sound->rpcData,
		timestamp,
		elapsedCue - sound->tracks[0].events[0].timestamp
	);
	for (i = 0; i < sound->sound->trackCount; i += 1)
	{
		sound->tracks[i].rpcData.rpcFilterFreq = sound->rpcData.rpcFilterFreq;
		sound->tracks[i].rpcData.rpcFilterQFactor = sound->rpcData.rpcFilterQFactor;
		FACT_INTERNAL_UpdateRPCs(
			sound->parentCue,
			sound->sound->tracks[i].rpcCodeCount,
			sound->sound->tracks[i].rpcCodes,
			&sound->tracks[i].rpcData,
			timestamp,
			elapsedCue - sound->sound->tracks[i].events[0].timestamp
		);
	}

	/* Go through each event for each track */
	for (i = 0; i < sound->sound->trackCount; i += 1)
	{
		/* Event updates */
		for (j = 0; j < sound->sound->tracks[i].eventCount; j += 1)
		{
			evtInst = &sound->tracks[i].events[j];
			if (!evtInst->finished)
			{
				/* Cue's not done yet...! */
				finished = 0;

				/* Trigger events at the right time */
				if (elapsedCue >= evtInst->timestamp)
				{
					FACT_INTERNAL_ActivateEvent(
						sound,
						&sound->sound->tracks[i],
						&sound->tracks[i],
						&sound->sound->tracks[i].events[j],
						evtInst,
						elapsedCue
					);
				}
			}
		}

		/* Wave updates */
		if (sound->tracks[i].activeWave.wave == NULL)
		{
			continue;
		}
		finished = 0;

		/* Clear out Waves as they finish */
		FACTWave_GetState(
			sound->tracks[i].activeWave.wave,
			&waveState
		);
		if (waveState & FACT_STATE_STOPPED)
		{
			FACTWave_Destroy(sound->tracks[i].activeWave.wave);
			FAudio_memcpy(
				&sound->tracks[i].activeWave,
				&sound->tracks[i].upcomingWave,
				sizeof(sound->tracks[i].activeWave)
			);
			sound->tracks[i].upcomingWave.wave = NULL;
			if (sound->tracks[i].activeWave.wave == NULL)
			{
				continue;
			}
			FACTWave_Play(sound->tracks[i].activeWave.wave);
		}

		FACTWave_SetVolume(
			sound->tracks[i].activeWave.wave,
			FACT_INTERNAL_CalculateAmplitudeRatio(
				sound->tracks[i].activeWave.baseVolume +
				sound->rpcData.rpcVolume +
				sound->tracks[i].rpcData.rpcVolume +
				sound->tracks[i].evtVolume
			) * sound->parentCue->parentBank->parentEngine->categories[
				sound->sound->category
			].currentVolume *
			fadeVolume
		);
		FACTWave_SetPitch(
			sound->tracks[i].activeWave.wave,
			(int16_t) (
				sound->tracks[i].activeWave.basePitch +
				sound->rpcData.rpcPitch +
				sound->tracks[i].rpcData.rpcPitch +
				sound->tracks[i].evtPitch
			)
		);
		if (sound->sound->tracks[i].filter != 0xFF)
		{
			/* FIXME: From what I can gather, filter parameters get
			 * overwritten by the RPC value if a filter RPC exists.
			 * Priority is Sound < Sound RPC < Track RPC, I think?
			 */
			filterParams.Type = (FAudioFilterType) sound->sound->tracks[i].filter;
			if (sound->tracks[i].rpcData.rpcFilterFreq >= 0.0f)
			{
				filterParams.Frequency = sound->tracks[i].rpcData.rpcFilterFreq;
			}
			else
			{
				filterParams.Frequency = sound->tracks[i].activeWave.baseFrequency;
			}
			if (sound->tracks[i].rpcData.rpcFilterQFactor >= 0.0f)
			{
				filterParams.OneOverQ = sound->tracks[i].rpcData.rpcFilterQFactor;
			}
			else
			{
				filterParams.OneOverQ = sound->tracks[i].activeWave.baseQFactor;
			}
			FAudioVoice_SetFilterParameters(
				sound->tracks[i].activeWave.wave->voice,
				&filterParams,
				0
			);
		}
		/* TODO: Wave updates:
		 * - ReverbSend (SetOutputMatrix on index 1, submix voice)
		 */
	}

	return finished;
}

void FACT_INTERNAL_UpdateCue(FACTCue *cue)
{
	uint32_t i;
	float next;
	FACTSoundInstance *sound;

	/* Interactive sound selection */
	if (!(cue->data->flags & 0x04) && cue->variation && cue->variation->flags == 3)
	{
		/* Interactive */
		if (cue->parentBank->parentEngine->variables[cue->variation->variable].accessibility & 0x04)
		{
			FACTCue_GetVariable(
				cue,
				cue->variation->variable,
				&next
			);
		}
		else
		{
			FACTAudioEngine_GetGlobalVariable(
				cue->parentBank->parentEngine,
				cue->variation->variable,
				&next
			);
		}
		if (next != cue->interactive)
		{
			cue->interactive = next;

			/* New sound, time for death! */
			if (cue->playingSound != NULL)
			{
				/* Copy of DestroySound but does not set Cue to STOPPED */
				sound = cue->playingSound;
				sound->parentCue->playingSound = NULL;
				for (i = 0; i < sound->sound->trackCount; i += 1)
				{
					if (sound->tracks[i].activeWave.wave != NULL)
					{
						FACTWave_Destroy(
							sound->tracks[i].activeWave.wave
						);
					}
					if (sound->tracks[i].upcomingWave.wave != NULL)
					{
						FACTWave_Destroy(
							sound->tracks[i].upcomingWave.wave
						);
					}
					cue->parentBank->parentEngine->pFree(
						sound->tracks[i].events
					);
				}
				cue->parentBank->parentEngine->pFree(sound->tracks);

				if (sound->sound->category != FACTCATEGORY_INVALID)
				{
					sound->parentCue->parentBank->parentEngine->categories[
						sound->sound->category
					].instanceCount -= 1;
				}
			}

			/* TODO: Reset cue times? Transition tables...?
			cue->start = elapsed;
			cue->elapsed = 0;
			 */

			FACT_INTERNAL_CreateSound(cue, 0 /* fadeIn */);
		}
	}
}

/* FACT Thread */

int32_t FACT_INTERNAL_APIThread(void* enginePtr)
{
	FACTAudioEngine *engine = (FACTAudioEngine*) enginePtr;
	LinkedList *sbList;
	FACTCue *cue, *cBackup;
	uint32_t timestamp, updateTime;

	/* Needs to match the audio thread priority, or else the scheduler will
	 * let this thread sit around with a lock while the audio thread spins
	 * infinitely!
	 */
	FAudio_PlatformThreadPriority(FAUDIO_THREAD_PRIORITY_HIGH);

threadstart:
	FAudio_PlatformLockMutex(engine->apiLock);

	/* We want the timestamp to be uniform across all Cues.
	 * Oftentimes many Cues are played at once with the expectation
	 * that they will sync, so give them all the same timestamp
	 * so all the various actions will go together even if it takes
	 * an extra millisecond to get through the whole Cue list.
	 */
	timestamp = FAudio_timems();

	FACT_INTERNAL_UpdateEngine(engine);

	sbList = engine->sbList;
	while (sbList != NULL)
	{
		cue = ((FACTSoundBank*) sbList->entry)->cueList;
		while (cue != NULL)
		{
			FACT_INTERNAL_UpdateCue(cue);

			if (cue->state & FACT_STATE_PAUSED)
			{
				cue = cue->next;
				continue;
			}

			if (cue->playingSound != NULL)
			{
				if (FACT_INTERNAL_UpdateSound(cue->playingSound, timestamp))
				{
					FACT_INTERNAL_DestroySound(cue->playingSound);
				}
			}

			/* Destroy if it's done and not user-handled. */
			if (cue->managed && (cue->state & FACT_STATE_STOPPED))
			{
				cBackup = cue->next;
				FACTCue_Destroy(cue);
				cue = cBackup;
			}
			else
			{
				cue = cue->next;
			}
		}
		sbList = sbList->next;
	}

	FAudio_PlatformUnlockMutex(engine->apiLock);

	if (engine->initialized)
	{
		/* FIXME: 10ms is based on the XAudio2 update time...? */
		updateTime = FAudio_timems() - timestamp;
		if (updateTime < 10)
		{
			FAudio_sleep(10 - updateTime);
		}
		goto threadstart;
	}

	return 0;
}

/* FAudio callbacks */

void FACT_INTERNAL_OnBufferEnd(FAudioVoiceCallback *callback, void* pContext)
{
	FAudioBuffer buffer;
	FAudioBufferWMA bufferWMA;
	FACTWaveCallback *c = (FACTWaveCallback*) callback;
	FACTWaveBankEntry *entry;
	uint32_t end, left, length;

	entry = &c->wave->parentBank->entries[c->wave->index];

	/* Calculate total bytes left in this wave iteration */
	if (c->wave->loopCount > 0 && entry->LoopRegion.dwTotalSamples > 0)
	{
		length = entry->LoopRegion.dwStartSample + entry->LoopRegion.dwTotalSamples;
		if (entry->Format.wFormatTag == 0x0)
		{
			length = (
				length *
				entry->Format.nChannels *
				(1 << entry->Format.wBitsPerSample)
			);
		}
		else if (entry->Format.wFormatTag == 0x2)
		{
			length = (
				length /
				/* wSamplesPerBlock */
				((entry->Format.wBlockAlign + 16) * 2) *
				/* nBlockAlign */
				((entry->Format.wBlockAlign + 22) * entry->Format.nChannels)
			);
		}
		else
		{
			length = entry->PlayRegion.dwLength;
		}
	}
	else
	{
		length = entry->PlayRegion.dwLength;
	}
	end = entry->PlayRegion.dwOffset + length;
	left = length - (c->wave->streamOffset - entry->PlayRegion.dwOffset);

	/* Don't bother if we're EOS or the Wave has stopped */
	if (	(c->wave->streamOffset >= end) ||
		(c->wave->state & FACT_STATE_STOPPED)	)
	{
		return;
	}

	/* Assign buffer memory */
	buffer.pAudioData = c->wave->streamCache;
	buffer.AudioBytes = FAudio_min(
		c->wave->streamSize,
		left
	);

	/* Read! */
	FACT_INTERNAL_ReadFile(
		c->wave->parentBank->parentEngine->pReadFile,
		c->wave->parentBank->parentEngine->pGetOverlappedResult,
		c->wave->parentBank->io,
		c->wave->streamOffset,
		c->wave->parentBank->packetSize,
		&c->wave->parentBank->packetBuffer,
		&c->wave->parentBank->packetBufferLen,
		c->wave->parentBank->parentEngine->pRealloc,
		c->wave->streamCache,
		buffer.AudioBytes
	);
	c->wave->streamOffset += buffer.AudioBytes;

	/* Last buffer in the stream? */
	buffer.Flags = 0;
	if (c->wave->streamOffset >= end)
	{
		/* Loop if applicable */
		if (c->wave->loopCount > 0)
		{
			if (c->wave->loopCount != 255)
			{
				c->wave->loopCount -= 1;
			}
			c->wave->streamOffset = entry->PlayRegion.dwOffset;

			/* Loop start */
			if (entry->Format.wFormatTag == 0x0)
			{
				c->wave->streamOffset += (
					entry->LoopRegion.dwStartSample *
					entry->Format.nChannels *
					(1 << entry->Format.wBitsPerSample)
				);
			}
			else if (entry->Format.wFormatTag == 0x2)
			{
				c->wave->streamOffset += (
					entry->LoopRegion.dwStartSample /
					/* wSamplesPerBlock */
					((entry->Format.wBlockAlign + 16) * 2) *
					/* nBlockAlign */
					((entry->Format.wBlockAlign + 22) * entry->Format.nChannels)
				);
			}
		}
		else
		{
			buffer.Flags = FAUDIO_END_OF_STREAM;
		}
	}

	/* Unused properties */
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = NULL;

	/* Submit, finally. */
	if (entry->Format.wFormatTag == 0x3)
	{
		bufferWMA.pDecodedPacketCumulativeBytes =
			c->wave->parentBank->seekTables[c->wave->index].entries;
		bufferWMA.PacketCount =
			c->wave->parentBank->seekTables[c->wave->index].entryCount;
		FAudioSourceVoice_SubmitSourceBuffer(
			c->wave->voice,
			&buffer,
			&bufferWMA
		);
	}
	else
	{
		FAudioSourceVoice_SubmitSourceBuffer(
			c->wave->voice,
			&buffer,
			NULL
		);
	}
}

void FACT_INTERNAL_OnStreamEnd(FAudioVoiceCallback *callback)
{
	FACTWaveCallback *c = (FACTWaveCallback*) callback;

	c->wave->state = FACT_STATE_STOPPED;

	if (	c->wave->parentCue != NULL &&
		c->wave->parentCue->simpleWave == c->wave	)
	{
		c->wave->parentCue->state |= FACT_STATE_STOPPED;
		c->wave->parentCue->state &= ~(
			FACT_STATE_PLAYING |
			FACT_STATE_STOPPING
		);
		c->wave->parentCue->data->instanceCount -= 1;
	}
}

/* FAudioIOStream functions */

int32_t FACTCALL FACT_INTERNAL_DefaultReadFile(
	void *hFile,
	void *buffer,
	uint32_t nNumberOfBytesToRead,
	uint32_t *lpNumberOfBytesRead, /* Not referenced! */
	FACTOverlapped *lpOverlapped
) {
	FAudioIOStream *io = (FAudioIOStream*) hFile;
	lpOverlapped->Internal = (void*) 0x00000103; /* STATUS_PENDING */
	FAudio_PlatformLockMutex((FAudioMutex) io->lock);
	io->seek(io->data, (size_t) lpOverlapped->Pointer, FAUDIO_SEEK_SET);
	lpOverlapped->InternalHigh = (void*) (size_t) (io->read(
		io->data,
		buffer,
		nNumberOfBytesToRead,
		1
	) * nNumberOfBytesToRead);
	FAudio_PlatformUnlockMutex((FAudioMutex) io->lock);
	lpOverlapped->Internal = 0; /* STATUS_SUCCESS */
	return 1;
}

int32_t FACTCALL FACT_INTERNAL_DefaultGetOverlappedResult(
	void *hFile,
	FACTOverlapped *lpOverlapped,
	uint32_t *lpNumberOfBytesTransferred,
	int32_t bWait
) {
	*lpNumberOfBytesTransferred = (uint32_t) (size_t) lpOverlapped->InternalHigh;
	return 1;
}

/* Parsing functions */

#define READ_FUNC(type, size, bitsize, suffix) \
	static inline type read_##suffix(uint8_t **ptr, const uint8_t swapendian) \
	{ \
		type result = *((type*) *ptr); \
		*ptr += size; \
		return swapendian ? \
			FAudio_swap##bitsize##BE(result) : \
			FAudio_swap##bitsize##LE(result); \
	}

static inline uint8_t read_u8(uint8_t **ptr)
{
	uint8_t result = *((uint8_t*) *ptr);
	*ptr += 1;
	return result;
}
READ_FUNC(uint16_t, 2, 16, u16)
READ_FUNC(uint32_t, 4, 32, u32)
READ_FUNC(int16_t, 2, 16, s16)
READ_FUNC(int32_t, 4, 32, s32)
static inline float read_f32(uint8_t **ptr, const uint8_t swapendian)
{
	float result = *((float*) *ptr);
	*ptr += 4;
	return result;
}

#undef READ_FUNC

static inline float read_volbyte(uint8_t **ptr)
{
	/* FIXME: This magnificent beauty came from Mathematica!
	 * The byte values for all possible input dB values from the .xap are here:
	 * http://www.flibitijibibo.com/XACTVolume.txt
	 * Yes, this is actually what the XACT builder really does.
	 *
	 * Thanks to Kenny for plotting all that data.
	 * -flibit
	 */
	return (float) ((3969.0 * FAudio_log10(read_u8(ptr) / 28240.0)) + 8715.0);
}

uint32_t FACT_INTERNAL_ParseAudioEngine(
	FACTAudioEngine *pEngine,
	const FACTRuntimeParameters *pParams
) {
	uint32_t	categoryOffset,
			variableOffset,
			blob1Offset,
			categoryNameIndexOffset,
			blob2Offset,
			variableNameIndexOffset,
			categoryNameOffset,
			variableNameOffset,
			rpcOffset,
			dspPresetOffset,
			dspParameterOffset;
	uint16_t blob1Count, blob2Count, tool;
	uint8_t version;
	uint8_t se;
	uint32_t magic;
	size_t memsize;
	uint16_t i, j;

	uint8_t *ptr = (uint8_t*) pParams->pGlobalSettingsBuffer;
	uint8_t *start = ptr;

	magic = read_u32(&ptr, 0);
	se = magic == 0x58475346; /* Swap Endian */
	if (magic != 0x46534758 && magic != 0x58475346) /* 'XGSF' */
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	if (!FACT_INTERNAL_SupportedContent(read_u16(&ptr, se)))
	{
		return -2;
	}

	tool = read_u16(&ptr, se); /* Tool version */
	if (tool != 42)
	{
		return -3;
	}

	ptr += 2; /* Unknown value */

	/* Last modified, unused */
	ptr += 8;

	/* XACT Version (Windows == 3, Xbox == 7) */
	version = read_u8(&ptr);
	if (	version != 3 &&
		version != 7	)
	{
		return -4; /* TODO: VERSION TOO OLD */
	}

	/* Object counts */
	pEngine->categoryCount = read_u16(&ptr, se);
	pEngine->variableCount = read_u16(&ptr, se);
	blob1Count = read_u16(&ptr, se);
	blob2Count = read_u16(&ptr, se);
	pEngine->rpcCount = read_u16(&ptr, se);
	pEngine->dspPresetCount = read_u16(&ptr, se);
	pEngine->dspParameterCount = read_u16(&ptr, se);

	/* Object offsets */
	categoryOffset = read_u32(&ptr, se);
	variableOffset = read_u32(&ptr, se);
	blob1Offset = read_u32(&ptr, se);
	categoryNameIndexOffset = read_u32(&ptr, se);
	blob2Offset = read_u32(&ptr, se);
	variableNameIndexOffset = read_u32(&ptr, se);
	categoryNameOffset = read_u32(&ptr, se);
	variableNameOffset = read_u32(&ptr, se);
	rpcOffset = read_u32(&ptr, se);
	dspPresetOffset = read_u32(&ptr, se);
	dspParameterOffset = read_u32(&ptr, se);

	/* Category data */
	FAudio_assert((ptr - start) == categoryOffset);
	pEngine->categories = (FACTAudioCategory*) pEngine->pMalloc(
		sizeof(FACTAudioCategory) * pEngine->categoryCount
	);
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		pEngine->categories[i].instanceLimit = read_u8(&ptr);
		pEngine->categories[i].fadeInMS = read_u16(&ptr, se);
		pEngine->categories[i].fadeOutMS = read_u16(&ptr, se);
		pEngine->categories[i].maxInstanceBehavior = read_u8(&ptr) >> 3;
		pEngine->categories[i].parentCategory = read_u16(&ptr, se);
		pEngine->categories[i].volume = FACT_INTERNAL_CalculateAmplitudeRatio(
			read_volbyte(&ptr)
		);
		pEngine->categories[i].visibility = read_u8(&ptr);
		pEngine->categories[i].instanceCount = 0;
		pEngine->categories[i].currentVolume = 1.0f;
	}

	/* Variable data */
	FAudio_assert((ptr - start) == variableOffset);
	pEngine->variables = (FACTVariable*) pEngine->pMalloc(
		sizeof(FACTVariable) * pEngine->variableCount
	);
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		pEngine->variables[i].accessibility = read_u8(&ptr);
		pEngine->variables[i].initialValue = read_f32(&ptr, se);
		pEngine->variables[i].minValue = read_f32(&ptr, se);
		pEngine->variables[i].maxValue = read_f32(&ptr, se);
	}

	/* Global variable storage. Some unused data for non-global vars */
	pEngine->globalVariableValues = (float*) pEngine->pMalloc(
		sizeof(float) * pEngine->variableCount
	);
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		pEngine->globalVariableValues[i] = pEngine->variables[i].initialValue;
	}

	/* RPC data */
	if (pEngine->rpcCount > 0)
	{
		FAudio_assert((ptr - start) == rpcOffset);
		pEngine->rpcs = (FACTRPC*) pEngine->pMalloc(
			sizeof(FACTRPC) *
			pEngine->rpcCount
		);
		pEngine->rpcCodes = (uint32_t*) pEngine->pMalloc(
			sizeof(uint32_t) *
			pEngine->rpcCount
		);
		for (i = 0; i < pEngine->rpcCount; i += 1)
		{
			pEngine->rpcCodes[i] = (uint32_t) (ptr - start);
			pEngine->rpcs[i].variable = read_u16(&ptr, se);
			pEngine->rpcs[i].pointCount = read_u8(&ptr);
			pEngine->rpcs[i].parameter = read_u16(&ptr, se);
			pEngine->rpcs[i].points = (FACTRPCPoint*) pEngine->pMalloc(
				sizeof(FACTRPCPoint) *
				pEngine->rpcs[i].pointCount
			);
			for (j = 0; j < pEngine->rpcs[i].pointCount; j += 1)
			{
				pEngine->rpcs[i].points[j].x = read_f32(&ptr, se);
				pEngine->rpcs[i].points[j].y = read_f32(&ptr, se);
				pEngine->rpcs[i].points[j].type = read_u8(&ptr);
			}
		}
	}

	/* DSP Preset data */
	if (pEngine->dspPresetCount > 0)
	{
		FAudio_assert((ptr - start) == dspPresetOffset);
		pEngine->dspPresets = (FACTDSPPreset*) pEngine->pMalloc(
			sizeof(FACTDSPPreset) *
			pEngine->dspPresetCount
		);
		pEngine->dspPresetCodes = (uint32_t*) pEngine->pMalloc(
			sizeof(uint32_t) *
			pEngine->dspPresetCount
		);
		for (i = 0; i < pEngine->dspPresetCount; i += 1)
		{
			pEngine->dspPresetCodes[i] = (uint32_t) (ptr - start);
			pEngine->dspPresets[i].accessibility = read_u8(&ptr);
			pEngine->dspPresets[i].parameterCount = read_u16(&ptr, se);
			ptr += 2; /* Unknown value */
			pEngine->dspPresets[i].parameters = (FACTDSPParameter*) pEngine->pMalloc(
				sizeof(FACTDSPParameter) *
				pEngine->dspPresets[i].parameterCount
			); /* This will be filled in just a moment... */
		}

		/* DSP Parameter data */
		FAudio_assert((ptr - start) == dspParameterOffset);
		for (i = 0; i < pEngine->dspPresetCount; i += 1)
		{
			for (j = 0; j < pEngine->dspPresets[i].parameterCount; j += 1)
			{
				pEngine->dspPresets[i].parameters[j].type = read_u8(&ptr);
				pEngine->dspPresets[i].parameters[j].value = read_f32(&ptr, se);
				pEngine->dspPresets[i].parameters[j].minVal = read_f32(&ptr, se);
				pEngine->dspPresets[i].parameters[j].maxVal = read_f32(&ptr, se);
				pEngine->dspPresets[i].parameters[j].unknown = read_u16(&ptr, se);
			}
		}
	}

	/* Blob #1, no idea what this is... */
	FAudio_assert((ptr - start) == blob1Offset);
	ptr += blob1Count * 2;

	/* Category Name Index data */
	FAudio_assert((ptr - start) == categoryNameIndexOffset);
	ptr += pEngine->categoryCount * 6; /* FIXME: index as assert value? */

	/* Category Name data */
	FAudio_assert((ptr - start) == categoryNameOffset);
	pEngine->categoryNames = (char**) pEngine->pMalloc(
		sizeof(char*) *
		pEngine->categoryCount
	);
	for (i = 0; i < pEngine->categoryCount; i += 1)
	{
		memsize = FAudio_strlen((char*) ptr) + 1; /* Dastardly! */
		pEngine->categoryNames[i] = (char*) pEngine->pMalloc(memsize);
		FAudio_memcpy(pEngine->categoryNames[i], ptr, memsize);
		ptr += memsize;
	}

	/* Blob #2, no idea what this is... */
	FAudio_assert((ptr - start) == blob2Offset);
	ptr += blob2Count * 2;

	/* Variable Name Index data */
	FAudio_assert((ptr - start) == variableNameIndexOffset);
	ptr += pEngine->variableCount * 6; /* FIXME: index as assert value? */

	/* Variable Name data */
	FAudio_assert((ptr - start) == variableNameOffset);
	pEngine->variableNames = (char**) pEngine->pMalloc(
		sizeof(char*) *
		pEngine->variableCount
	);
	for (i = 0; i < pEngine->variableCount; i += 1)
	{
		memsize = FAudio_strlen((char*) ptr) + 1; /* Dastardly! */
		pEngine->variableNames[i] = (char*) pEngine->pMalloc(memsize);
		FAudio_memcpy(pEngine->variableNames[i], ptr, memsize);
		ptr += memsize;
	}

	/* Store this pointer in case we're asked to free it */
	if (pParams->globalSettingsFlags & FACT_FLAG_MANAGEDATA)
	{
		pEngine->settings = pParams->pGlobalSettingsBuffer;
	}

	/* Finally. */
	FAudio_assert((ptr - start) == pParams->globalSettingsBufferSize);
	return 0;
}

void FACT_INTERNAL_ParseTrackEvents(
	uint8_t **ptr,
	uint8_t se,
	FACTTrack *track,
	FAudioMallocFunc pMalloc
) {
	uint32_t evtInfo;
	uint8_t minWeight, maxWeight, separator;
	uint8_t i;
	uint16_t j;

	track->eventCount = read_u8(ptr);
	track->events = (FACTEvent*) pMalloc(
		sizeof(FACTEvent) *
		track->eventCount
	);
	FAudio_zero(track->events, sizeof(FACTEvent) * track->eventCount);
	for (i = 0; i < track->eventCount; i += 1)
	{
		evtInfo = read_u32(ptr, se);
		track->events[i].randomOffset = read_u16(ptr, se);

		track->events[i].type = evtInfo & 0x001F;
		track->events[i].timestamp = (evtInfo >> 5) & 0xFFFF;

		separator = read_u8(ptr);
		FAudio_assert(separator == 0xFF); /* Separator? */

		#define EVTTYPE(t) (track->events[i].type == t)
		if (EVTTYPE(FACTEVENT_STOP))
		{
			track->events[i].stop.flags = read_u8(ptr);
		}
		else if (EVTTYPE(FACTEVENT_PLAYWAVE))
		{
			/* Basic Wave */
			track->events[i].wave.isComplex = 0;
			track->events[i].wave.flags = read_u8(ptr);
			track->events[i].wave.simple.track = read_u16(ptr, se);
			track->events[i].wave.simple.wavebank = read_u8(ptr);
			track->events[i].wave.loopCount = read_u8(ptr);
			track->events[i].wave.position = read_u16(ptr, se);
			track->events[i].wave.angle = read_u16(ptr, se);

			/* No Effect Variation */
			track->events[i].wave.variationFlags = 0;
		}
		else if (EVTTYPE(FACTEVENT_PLAYWAVETRACKVARIATION))
		{
			/* Complex Wave */
			track->events[i].wave.isComplex = 1;
			track->events[i].wave.flags = read_u8(ptr);
			track->events[i].wave.loopCount = read_u8(ptr);
			track->events[i].wave.position = read_u16(ptr, se);
			track->events[i].wave.angle = read_u16(ptr, se);

			/* Track Variation */
			evtInfo = read_u32(ptr, se);
			track->events[i].wave.complex.trackCount = evtInfo & 0xFFFF;
			track->events[i].wave.complex.variation = (evtInfo >> 16) & 0xFFFF;
			*ptr += 4; /* Unknown values */
			track->events[i].wave.complex.tracks = (uint16_t*) pMalloc(
				sizeof(uint16_t) *
				track->events[i].wave.complex.trackCount
			);
			track->events[i].wave.complex.wavebanks = (uint8_t*) pMalloc(
				sizeof(uint8_t) *
				track->events[i].wave.complex.trackCount
			);
			track->events[i].wave.complex.weights = (uint8_t*) pMalloc(
				sizeof(uint8_t) *
				track->events[i].wave.complex.trackCount
			);
			for (j = 0; j < track->events[i].wave.complex.trackCount; j += 1)
			{
				track->events[i].wave.complex.tracks[j] = read_u16(ptr, se);
				track->events[i].wave.complex.wavebanks[j] = read_u8(ptr);
				minWeight = read_u8(ptr);
				maxWeight = read_u8(ptr);
				track->events[i].wave.complex.weights[j] = (
					maxWeight - minWeight
				);
			}

			/* No Effect Variation */
			track->events[i].wave.variationFlags = 0;
		}
		else if (EVTTYPE(FACTEVENT_PLAYWAVEEFFECTVARIATION))
		{
			/* Basic Wave */
			track->events[i].wave.isComplex = 0;
			track->events[i].wave.flags = read_u8(ptr);
			track->events[i].wave.simple.track = read_u16(ptr, se);
			track->events[i].wave.simple.wavebank = read_u8(ptr);
			track->events[i].wave.loopCount = read_u8(ptr);
			track->events[i].wave.position = read_u16(ptr, se);
			track->events[i].wave.angle = read_u16(ptr, se);

			/* Effect Variation */
			track->events[i].wave.minPitch = read_s16(ptr, se);
			track->events[i].wave.maxPitch = read_s16(ptr, se);
			track->events[i].wave.minVolume = read_volbyte(ptr);
			track->events[i].wave.maxVolume = read_volbyte(ptr);
			track->events[i].wave.minFrequency = read_f32(ptr, se);
			track->events[i].wave.maxFrequency = read_f32(ptr, se);
			track->events[i].wave.minQFactor = read_f32(ptr, se);
			track->events[i].wave.maxQFactor = read_f32(ptr, se);
			track->events[i].wave.variationFlags = read_u16(ptr, se);
		}
		else if (EVTTYPE(FACTEVENT_PLAYWAVETRACKEFFECTVARIATION))
		{
			/* Complex Wave */
			track->events[i].wave.isComplex = 1;
			track->events[i].wave.flags = read_u8(ptr);
			track->events[i].wave.loopCount = read_u8(ptr);
			track->events[i].wave.position = read_u16(ptr, se);
			track->events[i].wave.angle = read_u16(ptr, se);

			/* Effect Variation */
			track->events[i].wave.minPitch = read_s16(ptr, se);
			track->events[i].wave.maxPitch = read_s16(ptr, se);
			track->events[i].wave.minVolume = read_volbyte(ptr);
			track->events[i].wave.maxVolume = read_volbyte(ptr);
			track->events[i].wave.minFrequency = read_f32(ptr, se);
			track->events[i].wave.maxFrequency = read_f32(ptr, se);
			track->events[i].wave.minQFactor = read_f32(ptr, se);
			track->events[i].wave.maxQFactor = read_f32(ptr, se);
			track->events[i].wave.variationFlags = read_u16(ptr, se);

			/* Track Variation */
			evtInfo = read_u32(ptr, se);
			track->events[i].wave.complex.trackCount = evtInfo & 0xFFFF;
			track->events[i].wave.complex.variation = (evtInfo >> 16) & 0xFFFF;
			*ptr += 4; /* Unknown values */
			track->events[i].wave.complex.tracks = (uint16_t*) pMalloc(
				sizeof(uint16_t) *
				track->events[i].wave.complex.trackCount
			);
			track->events[i].wave.complex.wavebanks = (uint8_t*) pMalloc(
				sizeof(uint8_t) *
				track->events[i].wave.complex.trackCount
			);
			track->events[i].wave.complex.weights = (uint8_t*) pMalloc(
				sizeof(uint8_t) *
				track->events[i].wave.complex.trackCount
			);
			for (j = 0; j < track->events[i].wave.complex.trackCount; j += 1)
			{
				track->events[i].wave.complex.tracks[j] = read_u16(ptr, se);
				track->events[i].wave.complex.wavebanks[j] = read_u8(ptr);
				minWeight = read_u8(ptr);
				maxWeight = read_u8(ptr);
				track->events[i].wave.complex.weights[j] = (
					maxWeight - minWeight
				);
			}
		}
		else if (	EVTTYPE(FACTEVENT_PITCH) ||
				EVTTYPE(FACTEVENT_VOLUME) ||
				EVTTYPE(FACTEVENT_PITCHREPEATING) ||
				EVTTYPE(FACTEVENT_VOLUMEREPEATING)	)
		{
			track->events[i].value.settings = read_u8(ptr);
			if (track->events[i].value.settings & 1) /* Ramp */
			{
				track->events[i].value.repeats = 0;
				track->events[i].value.ramp.initialValue = read_f32(ptr, se);
				track->events[i].value.ramp.initialSlope = read_f32(ptr, se) * 100;
				track->events[i].value.ramp.slopeDelta = read_f32(ptr, se);
				track->events[i].value.ramp.duration = read_u16(ptr, se);
			}
			else /* Equation */
			{
				track->events[i].value.equation.flags = read_u8(ptr);

				/* SetValue, SetRandomValue, anything else? */
				FAudio_assert(track->events[i].value.equation.flags & 0x0C);

				track->events[i].value.equation.value1 = read_f32(ptr, se);
				track->events[i].value.equation.value2 = read_f32(ptr, se);

				*ptr += 5; /* Unknown values */

				if (	EVTTYPE(FACTEVENT_PITCHREPEATING) ||
					EVTTYPE(FACTEVENT_VOLUMEREPEATING)	)
				{
					track->events[i].value.repeats = read_u16(ptr, se);
					track->events[i].value.frequency = read_u16(ptr, se);
				}
				else
				{
					track->events[i].value.repeats = 0;
				}
			}
		}
		else if (EVTTYPE(FACTEVENT_MARKER))
		{
			track->events[i].marker.marker = read_u32(ptr, se);
			track->events[i].marker.repeats = 0;
			track->events[i].marker.frequency = 0;
		}
		else if (EVTTYPE(FACTEVENT_MARKERREPEATING))
		{
			track->events[i].marker.marker = read_u32(ptr, se);
			track->events[i].marker.repeats = read_u16(ptr, se);
			track->events[i].marker.frequency = read_u16(ptr, se);
		}
		else
		{
			FAudio_assert(0 && "Unknown event type!");
		}
		#undef EVTTYPE
	}
}

uint32_t FACT_INTERNAL_ParseSoundBank(
	FACTAudioEngine *pEngine,
	const void *pvBuffer,
	uint32_t dwSize,
	FACTSoundBank **ppSoundBank
) {
	FACTSoundBank *sb;
	uint16_t	contentVersion,
			cueSimpleCount,
			cueComplexCount,
			cueTotalAlign;
	int32_t	cueSimpleOffset,
		cueComplexOffset,
		cueNameOffset,
		variationOffset,
		transitionOffset,
		wavebankNameOffset,
		cueHashOffset,
		cueNameIndexOffset,
		soundOffset;
	uint32_t entryCountAndFlags;
	uint16_t filterData;
	uint8_t platform;
	size_t memsize;
	uint16_t i, j, k, cur, tool;
	uint8_t *ptrBookmark;

	uint8_t *ptr = (uint8_t*) pvBuffer;
	uint8_t *start = ptr;

	uint32_t magic = read_u32(&ptr, 0);
	uint8_t se = magic == 0x5344424B; /* Swap Endian */

	if (magic != 0x4B424453 && magic != 0x5344424B)  /* 'SDBK' */
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	contentVersion = read_u16(&ptr, se);
	if (!FACT_INTERNAL_SupportedContent(contentVersion))
	{
		return -2;
	}

	tool = read_u16(&ptr, se); /* Tool version */
	if (tool != 43)
	{
		return -3;
	}

	/* CRC, unused */
	ptr += 2;

	/* Last modified, unused */
	ptr += 8;

	/* Windows == 1, Xbox == 3. 0 is unknown, probably old content */
	platform = read_u8(&ptr);
	if (	platform != 0 &&
		platform != 1 &&
		platform != 3	)
	{
		return -4; /* TODO: WRONG PLATFORM */
	}

	sb = (FACTSoundBank*) pEngine->pMalloc(sizeof(FACTSoundBank));
	sb->parentEngine = pEngine;
	sb->cueList = NULL;
	sb->notifyOnDestroy = 0;
	sb->usercontext = NULL;

	cueSimpleCount = read_u16(&ptr, se);
	cueComplexCount = read_u16(&ptr, se);

	ptr += 2; /* Unknown value */

	cueTotalAlign = read_u16(&ptr, se); /* FIXME: Why? */
	sb->cueCount = cueSimpleCount + cueComplexCount;
	sb->wavebankCount = read_u8(&ptr);
	sb->soundCount = read_u16(&ptr, se);

	/* Cue name length, unused */
	ptr += 2;

	ptr += 2; /* Unknown value */

	cueSimpleOffset = read_s32(&ptr, se);
	cueComplexOffset = read_s32(&ptr, se);
	cueNameOffset = read_s32(&ptr, se);

	ptr += 4; /* Unknown value */

	variationOffset = read_s32(&ptr, se);
	transitionOffset = read_s32(&ptr, se);
	wavebankNameOffset = read_s32(&ptr, se);
	cueHashOffset = read_s32(&ptr, se);
	cueNameIndexOffset = read_s32(&ptr, se);
	soundOffset = read_s32(&ptr, se);

	/* SoundBank Name */
	memsize = FAudio_strlen((char*) ptr) + 1; /* Dastardly! */
	sb->name = (char*) pEngine->pMalloc(memsize);
	FAudio_memcpy(sb->name, ptr, memsize);
	ptr += 64;

	/* WaveBank Name data */
	FAudio_assert((ptr - start) == wavebankNameOffset);
	ptr = start + wavebankNameOffset;
	sb->wavebankNames = (char**) pEngine->pMalloc(
		sizeof(char*) *
		sb->wavebankCount
	);
	for (i = 0; i < sb->wavebankCount; i += 1)
	{
		memsize = FAudio_strlen((char*) ptr) + 1;
		sb->wavebankNames[i] = (char*) pEngine->pMalloc(memsize);
		FAudio_memcpy(sb->wavebankNames[i], ptr, memsize);
		ptr += 64;
	}

	/* Sound data */
	FAudio_assert((ptr - start) == soundOffset);
	sb->sounds = (FACTSound*) pEngine->pMalloc(
		sizeof(FACTSound) *
		sb->soundCount
	);
	sb->soundCodes = (uint32_t*) pEngine->pMalloc(
		sizeof(uint32_t) *
		sb->soundCount
	);
	for (i = 0; i < sb->soundCount; i += 1)
	{
		sb->soundCodes[i] = (uint32_t) (ptr - start);
		sb->sounds[i].flags = read_u8(&ptr);
		sb->sounds[i].category = read_u16(&ptr, se);
		sb->sounds[i].volume = read_volbyte(&ptr);
		sb->sounds[i].pitch = read_s16(&ptr, se);
		sb->sounds[i].priority = read_u8(&ptr);

		/* Length of sound entry, unused */
		ptr += 2;

		/* Simple/Complex Track data */
		if (sb->sounds[i].flags & 0x01)
		{
			sb->sounds[i].trackCount = read_u8(&ptr);
			memsize = sizeof(FACTTrack) * sb->sounds[i].trackCount;
			sb->sounds[i].tracks = (FACTTrack*) pEngine->pMalloc(memsize);
			FAudio_zero(sb->sounds[i].tracks, memsize);
		}
		else
		{
			sb->sounds[i].trackCount = 1;
			memsize = sizeof(FACTTrack) * sb->sounds[i].trackCount;
			sb->sounds[i].tracks = (FACTTrack*) pEngine->pMalloc(memsize);
			FAudio_zero(sb->sounds[i].tracks, memsize);
			sb->sounds[i].tracks[0].volume = 0.0f;
			sb->sounds[i].tracks[0].filter = 0xFF;
			sb->sounds[i].tracks[0].eventCount = 1;
			sb->sounds[i].tracks[0].events = (FACTEvent*) pEngine->pMalloc(
				sizeof(FACTEvent)
			);
			FAudio_zero(
				sb->sounds[i].tracks[0].events,
				sizeof(FACTEvent)
			);
			sb->sounds[i].tracks[0].events[0].type = FACTEVENT_PLAYWAVE;
			sb->sounds[i].tracks[0].events[0].wave.position = 0; /* FIXME */
			sb->sounds[i].tracks[0].events[0].wave.angle = 0; /* FIXME */
			sb->sounds[i].tracks[0].events[0].wave.simple.track = read_u16(&ptr, se);
			sb->sounds[i].tracks[0].events[0].wave.simple.wavebank = read_u8(&ptr);
		}

		/* RPC Code data */
		if (sb->sounds[i].flags & 0x0E)
		{
			const uint16_t rpcDataLength = read_u16(&ptr, se);
			ptrBookmark = ptr - 2;

			#define COPYRPCBLOCK(loc) \
				loc.rpcCodeCount = read_u8(&ptr); \
				memsize = sizeof(uint32_t) * loc.rpcCodeCount; \
				loc.rpcCodes = (uint32_t*) pEngine->pMalloc(memsize); \
				for (k = 0; k < loc.rpcCodeCount; k += 1) \
				{ \
					loc.rpcCodes[k] = read_u32(&ptr, se); \
				} \

			/* Sound has attached RPCs */
			if (sb->sounds[i].flags & 0x02)
			{
				COPYRPCBLOCK(sb->sounds[i])
			}
			else
			{
				sb->sounds[i].rpcCodeCount = 0;
				sb->sounds[i].rpcCodes = NULL;
			}

			/* Tracks have attached RPCs */
			if (sb->sounds[i].flags & 0x04)
			{
				for (j = 0; j < sb->sounds[i].trackCount; j += 1)
				{
					COPYRPCBLOCK(sb->sounds[i].tracks[j])
				}
			}
			else
			{
				for (j = 0; j < sb->sounds[i].trackCount; j += 1)
				{
					sb->sounds[i].tracks[j].rpcCodeCount = 0;
					sb->sounds[i].tracks[j].rpcCodes = NULL;
				}
			}

			#undef COPYRPCBLOCK

			/* FIXME: Does 0x08 mean something for RPCs...? */
			FAudio_assert((ptr - ptrBookmark) == rpcDataLength);
		}
		else
		{
			sb->sounds[i].rpcCodeCount = 0;
			sb->sounds[i].rpcCodes = NULL;
			for (j = 0; j < sb->sounds[i].trackCount; j += 1)
			{
				sb->sounds[i].tracks[j].rpcCodeCount = 0;
				sb->sounds[i].tracks[j].rpcCodes = NULL;
			}
		}

		/* DSP Preset Code data */
		if (sb->sounds[i].flags & 0x10)
		{
			/* DSP presets length, unused */
			ptr += 2;

			sb->sounds[i].dspCodeCount = read_u8(&ptr);
			memsize = sizeof(uint32_t) * sb->sounds[i].dspCodeCount;
			sb->sounds[i].dspCodes = (uint32_t*) pEngine->pMalloc(memsize);
			for (j = 0; j < sb->sounds[i].dspCodeCount; j += 1)
			{
				sb->sounds[i].dspCodes[j] = read_u32(&ptr, se);
			}
		}
		else
		{
			sb->sounds[i].dspCodeCount = 0;
			sb->sounds[i].dspCodes = NULL;
		}

		/* Track data */
		if (sb->sounds[i].flags & 0x01)
		{
			for (j = 0; j < sb->sounds[i].trackCount; j += 1)
			{
				sb->sounds[i].tracks[j].volume = read_volbyte(&ptr);

				sb->sounds[i].tracks[j].code = read_u32(&ptr, se);

				if (contentVersion == FACT_CONTENT_VERSION_3_0)
				{
					/* 3.0 doesn't have track filter data */
					sb->sounds[i].tracks[j].filter = 0xFF;
					sb->sounds[i].tracks[j].qfactor = 0;
					sb->sounds[i].tracks[j].frequency = 0;
					continue;
				}

				filterData = read_u16(&ptr, se);
				if (filterData & 0x0001)
				{
					sb->sounds[i].tracks[j].filter =
						(filterData >> 1) & 0x02;
				}
				else
				{
					/* Huh...? */
					sb->sounds[i].tracks[j].filter = 0xFF;
				}
				sb->sounds[i].tracks[j].qfactor = (filterData >> 8) & 0xFF;
				sb->sounds[i].tracks[j].frequency = read_u16(&ptr, se);
			}

			/* All Track events are stored at the end of the block */
			for (j = 0; j < sb->sounds[i].trackCount; j += 1)
			{
				FAudio_assert((ptr - start) == sb->sounds[i].tracks[j].code);
				FACT_INTERNAL_ParseTrackEvents(
					&ptr,
					se,
					&sb->sounds[i].tracks[j],
					pEngine->pMalloc
				);
			}
		}
	}

	/* All Cue data */
	sb->variationCount = 0;
	sb->transitionCount = 0;
	sb->cues = (FACTCueData*) pEngine->pMalloc(
		sizeof(FACTCueData) *
		sb->cueCount
	);
	cur = 0;

	/* Simple Cue data */
	FAudio_assert(cueSimpleCount == 0 || (ptr - start) == cueSimpleOffset);
	if (cueSimpleCount > 0)
	{
		ptr = start + cueSimpleOffset;

		for (i = 0; i < cueSimpleCount; i += 1, cur += 1)
		{
			sb->cues[cur].flags = read_u8(&ptr);
			sb->cues[cur].sbCode = read_u32(&ptr, se);
			sb->cues[cur].transitionOffset = 0;
			sb->cues[cur].instanceLimit = 0xFF;
			sb->cues[cur].fadeInMS = 0;
			sb->cues[cur].fadeOutMS = 0;
			sb->cues[cur].maxInstanceBehavior = 0;
			sb->cues[cur].instanceCount = 0;
		}
	}

	/* Complex Cue data */
	FAudio_assert(cueComplexCount == 0 || (ptr - start) == cueComplexOffset);
	if (cueComplexCount > 0)
	{
		ptr = start + cueComplexOffset;

		for (i = 0; i < cueComplexCount; i += 1, cur += 1)
		{
			sb->cues[cur].flags = read_u8(&ptr);
			sb->cues[cur].sbCode = read_u32(&ptr, se);
			sb->cues[cur].transitionOffset = read_u32(&ptr, se);
			if (sb->cues[cur].transitionOffset == 0xFFFFFFFF)
			{
				/* FIXME: Why */
				sb->cues[cur].transitionOffset = 0;
			}
			sb->cues[cur].instanceLimit = read_u8(&ptr);
			sb->cues[cur].fadeInMS = read_u16(&ptr, se);
			sb->cues[cur].fadeOutMS = read_u16(&ptr, se);
			sb->cues[cur].maxInstanceBehavior = read_u8(&ptr) >> 3;
			sb->cues[cur].instanceCount = 0;

			if (!(sb->cues[cur].flags & 0x04))
			{
				/* FIXME: Is this the only way to get this...? */
				sb->variationCount += 1;
			}
			if (sb->cues[cur].transitionOffset > 0)
			{
				/* FIXME: Is this the only way to get this...? */
				sb->transitionCount += 1;
			}
		}
	}

	/* Variation data */
	if (sb->variationCount > 0)
	{
		FAudio_assert((ptr - start) == variationOffset);
		ptr = start + variationOffset;
		sb->variations = (FACTVariationTable*) pEngine->pMalloc(
			sizeof(FACTVariationTable) *
			sb->variationCount
		);
		sb->variationCodes = (uint32_t*) pEngine->pMalloc(
			sizeof(uint32_t) *
			sb->variationCount
		);
	}
	else
	{
		sb->variations = NULL;
		sb->variationCodes = NULL;
	}
	for (i = 0; i < sb->variationCount; i += 1)
	{
		sb->variationCodes[i] = (uint32_t) (ptr - start);
		entryCountAndFlags = read_u32(&ptr, se);
		sb->variations[i].entryCount = entryCountAndFlags & 0xFFFF;
		sb->variations[i].flags = (entryCountAndFlags >> (16 + 3)) & 0x07;
		ptr += 2; /* Unknown value */
		sb->variations[i].variable = read_s16(&ptr, se);
		memsize = sizeof(FACTVariation) * sb->variations[i].entryCount;
		sb->variations[i].entries = (FACTVariation*) pEngine->pMalloc(
			memsize
		);
		FAudio_zero(sb->variations[i].entries, memsize);

		if (sb->variations[i].flags == 0)
		{
			/* Wave with byte min/max */
			sb->variations[i].isComplex = 0;
			for (j = 0; j < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].simple.track = read_u16(&ptr, se);
				sb->variations[i].entries[j].simple.wavebank = read_u8(&ptr);
				sb->variations[i].entries[j].minWeight = read_u8(&ptr) / 255.0f;
				sb->variations[i].entries[j].maxWeight = read_u8(&ptr) / 255.0f;
			}
		}
		else if (sb->variations[i].flags == 1)
		{
			/* Complex with byte min/max */
			sb->variations[i].isComplex = 1;
			for (j = 0; j < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].soundCode = read_u32(&ptr, se);
				sb->variations[i].entries[j].minWeight = read_u8(&ptr) / 255.0f;
				sb->variations[i].entries[j].maxWeight = read_u8(&ptr) / 255.0f;
			}
		}
		else if (sb->variations[i].flags == 3)
		{
			/* Complex Interactive Variation with float min/max */
			sb->variations[i].isComplex = 1;
			for (j = 0; j < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].soundCode = read_u32(&ptr, se);
				sb->variations[i].entries[j].minWeight = read_f32(&ptr, se);
				sb->variations[i].entries[j].maxWeight = read_f32(&ptr, se);
				sb->variations[i].entries[j].linger = read_u32(&ptr, se);
			}
		}
		else if (sb->variations[i].flags == 4)
		{
			/* Compact Wave */
			sb->variations[i].isComplex = 0;
			for (j = 0; j < sb->variations[i].entryCount; j += 1)
			{
				sb->variations[i].entries[j].simple.track = read_u16(&ptr, se);
				sb->variations[i].entries[j].simple.wavebank = read_u8(&ptr);
				sb->variations[i].entries[j].minWeight = 0.0f;
				sb->variations[i].entries[j].maxWeight = 1.0f;
			}
		}
		else
		{
			FAudio_assert(0 && "Unknown variation type!");
		}
	}

	/* Transition data */
	if (sb->transitionCount > 0)
	{
		FAudio_assert((ptr - start) == transitionOffset);
		ptr = start + transitionOffset;
		sb->transitions = (FACTTransitionTable*) pEngine->pMalloc(
			sizeof(FACTTransitionTable) *
			sb->transitionCount
		);
		sb->transitionCodes = (uint32_t*) pEngine->pMalloc(
			sizeof(uint32_t) *
			sb->transitionCount
		);
	}
	else
	{
		sb->transitions = NULL;
		sb->transitionCodes = NULL;
	}
	for (i = 0; i < sb->transitionCount; i += 1)
	{
		sb->transitionCodes[i] = (uint32_t) (ptr - start);
		sb->transitions[i].entryCount = read_u32(&ptr, se);
		memsize = sizeof(FACTTransition) * sb->transitions[i].entryCount;
		sb->transitions[i].entries = (FACTTransition*) pEngine->pMalloc(
			memsize
		);
		FAudio_zero(sb->transitions[i].entries, memsize);
		for (j = 0; j < sb->transitions[i].entryCount; j += 1)
		{
			sb->transitions[i].entries[j].soundCode = read_s32(&ptr, se);
			sb->transitions[i].entries[j].srcMarkerMin = read_u32(&ptr, se);
			sb->transitions[i].entries[j].srcMarkerMax = read_u32(&ptr, se);
			sb->transitions[i].entries[j].dstMarkerMin = read_u32(&ptr, se);
			sb->transitions[i].entries[j].dstMarkerMax = read_u32(&ptr, se);
			sb->transitions[i].entries[j].fadeIn = read_u16(&ptr, se);
			sb->transitions[i].entries[j].fadeOut = read_u16(&ptr, se);
			sb->transitions[i].entries[j].flags = read_u16(&ptr, se);
		}
	}

	/* Cue Hash data? No idea what this is... */
	if (cueHashOffset != -1)
	{
		FAudio_assert((ptr - start) == cueHashOffset);
		ptr += 2 * cueTotalAlign;
	}

	/* Cue Name Index data */
	if (cueNameIndexOffset != -1)
	{
		FAudio_assert((ptr - start) == cueNameIndexOffset);
		ptr = start + cueNameIndexOffset;
		ptr += 6 * sb->cueCount; /* FIXME: index as assert value? */
	}

	/* Cue Name data */
	if (cueNameOffset != -1)
	{
		FAudio_assert((ptr - start) == cueNameOffset);
		ptr = start + cueNameOffset;
		sb->cueNames = (char**) pEngine->pMalloc(
			sizeof(char*) *
			sb->cueCount
		);
		for (i = 0; i < sb->cueCount; i += 1)
		{
			memsize = FAudio_strlen((char*) ptr) + 1;
			sb->cueNames[i] = (char*) pEngine->pMalloc(memsize);
			FAudio_memcpy(sb->cueNames[i], ptr, memsize);
			ptr += memsize;
		}
	}
	else
	{
		sb->cueNames = NULL;
	}

	/* Add to the Engine SoundBank list */
	LinkedList_AddEntry(
		&pEngine->sbList,
		sb,
		pEngine->sbLock,
		pEngine->pMalloc
	);

	/* Finally. */
	FAudio_assert((ptr - start) == dwSize);
	*ppSoundBank = sb;
	return 0;
}

/* This parser is based on the unxwb project, written by Luigi Auriemma.
 *
 * While the unxwb project was released under the GPL, Luigi has given us
 * permission to use the unxwb sources under the zlib license.
 *
 * The unxwb website can be found here:
 *
 * http://aluigi.altervista.org/papers.htm#xbox
 */
uint32_t FACT_INTERNAL_ParseWaveBank(
	FACTAudioEngine *pEngine,
	void* io,
	uint32_t offset,
	uint32_t packetSize,
	FACTReadFileCallback pRead,
	FACTGetOverlappedResultCallback pOverlap,
	uint16_t isStreaming,
	FACTWaveBank **ppWaveBank
) {
	uint8_t se = 0; /* Swap Endian */
	FACTWaveBank *wb;
	size_t memsize;
	uint32_t i, j;
	FACTWaveBankHeader header;
	FACTWaveBankData wbinfo;
	uint32_t compactEntry;
	int32_t seekTableOffset;
	uint32_t fileOffset;
	uint8_t *packetBuffer = NULL;
	uint32_t packetBufferLen = 0;
	uint16_t *pcm;

	#define SEEKSET(loc) \
		fileOffset = offset + loc;
	#define SEEKCUR(loc) \
		fileOffset += loc;
	#define READ(dst, size) \
		FACT_INTERNAL_ReadFile( \
			pRead, \
			pOverlap, \
			io, \
			fileOffset, \
			packetSize, \
			&packetBuffer, \
			&packetBufferLen, \
			pEngine->pRealloc, \
			dst, \
			size \
		); \
		SEEKCUR(size)

	#define DOSWAP_16(x) x = FAudio_swap16BE(x)
	#define DOSWAP_32(x) x = FAudio_swap32BE(x)
	#define DOSWAP_64(x) x = FAudio_swap64BE(x)

	fileOffset = offset;

	FAudio_zero(&header, sizeof(header));
	READ(&header.dwSignature, sizeof(header.dwSignature));
	READ(&header.dwVersion, sizeof(header.dwVersion));
	if (header.dwVersion > FACT_CONTENT_VERSION_2_4)
	{
		READ(&header.dwHeaderVersion, sizeof(header.dwHeaderVersion));
	}

	READ(&header.Segments, sizeof(header.Segments));

	se = header.dwSignature == 0x57424E44;
	if (se)
	{
		DOSWAP_32(header.dwSignature);
		DOSWAP_32(header.dwVersion);
		DOSWAP_32(header.dwHeaderVersion);
		for (i = 0; i < FACT_WAVEBANK_SEGIDX_COUNT; i += 1)
		{
			DOSWAP_32(header.Segments[i].dwOffset);
			DOSWAP_32(header.Segments[i].dwLength);
		}
	}
	if (header.dwSignature != 0x444E4257)
	{
		return -1; /* TODO: NOT XACT FILE */
	}

	/* We support all Wavebank versions - Restore when SoundBank support them also. */
	/*if (!FACT_INTERNAL_SupportedContent(header.dwVersion))
	{
		return -2;
	}
	*/
	if (	header.dwVersion < FACT_CONTENT_VERSION_2_4 ||
		header.dwVersion > FACT_CONTENT_VERSION	)
	{
		return -2;
	}

	if (	header.dwVersion > FACT_CONTENT_VERSION_2_4 &&
		!FACT_INTERNAL_SupportedWBContent(header.dwHeaderVersion)	)
	{
		return -3;
	}

	wb = (FACTWaveBank*) pEngine->pMalloc(sizeof(FACTWaveBank));
	wb->parentEngine = pEngine;
	wb->waveList = NULL;
	wb->waveLock = FAudio_PlatformCreateMutex();
	wb->packetSize = packetSize;
	wb->io = io;
	wb->notifyOnDestroy = 0;
	wb->usercontext = NULL;

	/* WaveBank Data */
	SEEKSET(header.Segments[FACT_WAVEBANK_SEGIDX_BANKDATA].dwOffset)
	READ(&wbinfo, sizeof(wbinfo))
	if (se)
	{
		DOSWAP_32(wbinfo.dwFlags);
		DOSWAP_32(wbinfo.dwEntryCount);
		DOSWAP_32(wbinfo.dwEntryMetaDataElementSize);
		DOSWAP_32(wbinfo.dwEntryNameElementSize);
		DOSWAP_32(wbinfo.dwAlignment);
		DOSWAP_32(wbinfo.CompactFormat.dwValue);
		DOSWAP_64(wbinfo.BuildTime);
	}
	wb->streaming = (wbinfo.dwFlags & FACT_WAVEBANK_TYPE_STREAMING);
	wb->entryCount = wbinfo.dwEntryCount;
	memsize = FAudio_strlen(wbinfo.szBankName) + 1;
	wb->name = (char*) pEngine->pMalloc(memsize);
	FAudio_memcpy(wb->name, wbinfo.szBankName, memsize);
	memsize = sizeof(FACTWaveBankEntry) * wbinfo.dwEntryCount;
	wb->entries = (FACTWaveBankEntry*) pEngine->pMalloc(memsize);
	FAudio_zero(wb->entries, memsize);
	memsize = sizeof(uint32_t) * wbinfo.dwEntryCount;
	wb->entryRefs = (uint32_t*) pEngine->pMalloc(memsize);
	FAudio_zero(wb->entryRefs, memsize);

	/* FIXME: How much do we care about this? */
	FAudio_assert(wb->streaming == isStreaming);
	wb->streaming = isStreaming;

	/* WaveBank Entry Metadata */
	SEEKSET(header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYMETADATA].dwOffset)
	if (wbinfo.dwFlags & FACT_WAVEBANK_FLAGS_COMPACT)
	{
		for (i = 0; i < wbinfo.dwEntryCount - 1; i += 1)
		{
			READ(&compactEntry, sizeof(compactEntry))
			if (se)
			{
				DOSWAP_32(compactEntry);
			}
			wb->entries[i].PlayRegion.dwOffset = (
				(compactEntry & ((1 << 21) - 1)) *
				wbinfo.dwAlignment
			);
			wb->entries[i].PlayRegion.dwLength = (
				(compactEntry >> 21) & ((1 << 11) - 1)
			);

			/* TODO: Deviation table */
			SEEKCUR(wbinfo.dwEntryMetaDataElementSize)
			wb->entries[i].PlayRegion.dwLength = (
				(compactEntry & ((1 << 21) - 1)) *
				wbinfo.dwAlignment
			) - wb->entries[i].PlayRegion.dwOffset;

			wb->entries[i].PlayRegion.dwOffset +=
				header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwOffset;
		}

		READ(&compactEntry, sizeof(compactEntry))
		if (se)
		{
			DOSWAP_32(compactEntry);
		}
		wb->entries[i].PlayRegion.dwOffset = (
			(compactEntry & ((1 << 21) - 1)) *
			wbinfo.dwAlignment
		);

		/* TODO: Deviation table */
		SEEKCUR(wbinfo.dwEntryMetaDataElementSize)
		wb->entries[i].PlayRegion.dwLength = (
			header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwLength -
			wb->entries[i].PlayRegion.dwOffset
		);

		wb->entries[i].PlayRegion.dwOffset +=
			header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwOffset;
	}
	else
	{
		for (i = 0; i < wbinfo.dwEntryCount; i += 1)
		{
			READ(&wb->entries[i], wbinfo.dwEntryMetaDataElementSize)
			if (se)
			{
				DOSWAP_32(wb->entries[i].dwFlagsAndDuration);
				DOSWAP_32(wb->entries[i].Format.dwValue);
				DOSWAP_32(wb->entries[i].PlayRegion.dwOffset);
				DOSWAP_32(wb->entries[i].PlayRegion.dwLength);
				DOSWAP_32(wb->entries[i].LoopRegion.dwStartSample);
				DOSWAP_32(wb->entries[i].LoopRegion.dwTotalSamples);
			}
			wb->entries[i].PlayRegion.dwOffset +=
				header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwOffset;

			/* If it's in-memory big-endian PCM, swap! */
			if (	se &&
				!wb->streaming &&
				wb->entries[i].Format.wFormatTag == 0x0 &&
				wb->entries[i].Format.wBitsPerSample == 1	)
			{
				pcm = (uint16_t*) FAudio_memptr(
					(FAudioIOStream*) wb->io,
					wb->entries[i].PlayRegion.dwOffset
				);
				for (j = 0; j < wb->entries[i].PlayRegion.dwLength; j += 2, pcm += 1)
				{
					DOSWAP_16(*pcm);
				}
			}
		}

		/* FIXME: This is a bit hacky. */
		if (wbinfo.dwEntryMetaDataElementSize < 24)
		{
			for (i = 0; i < wbinfo.dwEntryCount; i += 1)
			{
				wb->entries[i].PlayRegion.dwLength =
					header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwLength;
			}
		}
	}

	/* WaveBank Seek Tables */
	if (	wbinfo.dwFlags & FACT_WAVEBANK_FLAGS_SEEKTABLES &&
		header.Segments[FACT_WAVEBANK_SEGIDX_SEEKTABLES].dwLength > 0	)
	{
		/* The seek table data layout is an absolute disaster! */
		wb->seekTables = (FACTSeekTable*) pEngine->pMalloc(
			wbinfo.dwEntryCount * sizeof(FACTSeekTable)
		);
		for (i = 0; i < wbinfo.dwEntryCount; i += 1)
		{
			/* Get the table offset... */
			SEEKSET(
				header.Segments[FACT_WAVEBANK_SEGIDX_SEEKTABLES].dwOffset +
				i * sizeof(uint32_t)
			)
			READ(&seekTableOffset, sizeof(int32_t))
			if (se)
			{
				DOSWAP_32(seekTableOffset);
			}

			/* If the offset is -1, this wave needs no table */
			if (seekTableOffset == -1)
			{
				wb->seekTables[i].entryCount = 0;
				wb->seekTables[i].entries = NULL;
				continue;
			}

			/* Go to the table offset, after the offset table... */
			SEEKSET(
				header.Segments[FACT_WAVEBANK_SEGIDX_SEEKTABLES].dwOffset +
				(wbinfo.dwEntryCount * sizeof(uint32_t)) +
				seekTableOffset
			)

			/* Read the table, finally. */
			READ(&wb->seekTables[i].entryCount, sizeof(uint32_t))
			if (se)
			{
				DOSWAP_32(wb->seekTables[i].entryCount);
			}
			wb->seekTables[i].entries = (uint32_t*) pEngine->pMalloc(
				wb->seekTables[i].entryCount * sizeof(uint32_t)
			);
			READ(
				wb->seekTables[i].entries,
				wb->seekTables[i].entryCount * sizeof(uint32_t)
			)
			if (se)
			{
				for (j = 0; j < wb->seekTables[i].entryCount; j += 1)
				{
					DOSWAP_32(wb->seekTables[i].entries[j]);
				}
			}
		}
	}
	else
	{
		wb->seekTables = NULL;
	}

	/* WaveBank Entry Names */
	if (wbinfo.dwFlags & FACT_WAVEBANK_FLAGS_ENTRYNAMES)
	{
		SEEKSET(header.Segments[FACT_WAVEBANK_SEGIDX_ENTRYNAMES].dwOffset)
		wb->waveBankNames = (char*) pEngine->pMalloc(64 * wbinfo.dwEntryCount);
		READ(wb->waveBankNames, 64 * wbinfo.dwEntryCount);
	}
	else
	{
		wb->waveBankNames = NULL;
	}

	/* Add to the Engine WaveBank list */
	LinkedList_AddEntry(
		&pEngine->wbList,
		wb,
		pEngine->wbLock,
		pEngine->pMalloc
	);

	/* Finally. */
	wb->packetBuffer = packetBuffer;
	wb->packetBufferLen = packetBufferLen;
	*ppWaveBank = wb;
	return 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
