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

/* FAudio_operationset.c originally written by Tyler Glaiel */

#include "FAudio_internal.h"

/* Core OperationSet Types */

typedef enum FAudio_OPERATIONSET_Type
{
	FAUDIOOP_ENABLEEFFECT,
	FAUDIOOP_DISABLEEFFECT,
	FAUDIOOP_SETEFFECTPARAMETERS,
	FAUDIOOP_SETFILTERPARAMETERS,
	FAUDIOOP_SETOUTPUTFILTERPARAMETERS,
	FAUDIOOP_SETVOLUME,
	FAUDIOOP_SETCHANNELVOLUMES,
	FAUDIOOP_SETOUTPUTMATRIX,
	FAUDIOOP_START,
	FAUDIOOP_STOP,
	FAUDIOOP_EXITLOOP,
	FAUDIOOP_SETFREQUENCYRATIO
} FAudio_OPERATIONSET_Type;

struct FAudio_OPERATIONSET_Operation
{
	FAudio_OPERATIONSET_Type Type;
	uint32_t OperationSet;
	FAudioVoice *Voice;

	union
	{
		struct
		{
			uint32_t EffectIndex;
		} EnableEffect;
		struct
		{
			uint32_t EffectIndex;
		} DisableEffect;
		struct
		{
			uint32_t EffectIndex;
			void *pParameters;
			uint32_t ParametersByteSize;
		} SetEffectParameters;
		struct
		{
			FAudioFilterParametersEXT Parameters;
		} SetFilterParameters;
		struct
		{
			FAudioVoice *pDestinationVoice;
			FAudioFilterParametersEXT Parameters;
		} SetOutputFilterParameters;
		struct
		{
			float Volume;
		} SetVolume;
		struct
		{
			uint32_t Channels;
			float *pVolumes;
		} SetChannelVolumes;
		struct
		{
			FAudioVoice *pDestinationVoice;
			uint32_t SourceChannels;
			uint32_t DestinationChannels;
			float *pLevelMatrix;
		} SetOutputMatrix;
		struct
		{
			uint32_t Flags;
		} Start;
		struct
		{
			uint32_t Flags;
		} Stop;
		/* No special data for ExitLoop
		struct
		{
		} ExitLoop;
		*/
		struct
		{
			float Ratio;
		} SetFrequencyRatio;
	} Data;

	FAudio_OPERATIONSET_Operation *next;
};

/* Used by both Commit and Clear routines */

static inline void DeleteOperation(
	FAudio_OPERATIONSET_Operation *op,
	FAudioFreeFunc pFree
) {
	if (op->Type == FAUDIOOP_SETEFFECTPARAMETERS)
	{
		pFree(op->Data.SetEffectParameters.pParameters);
	}
	else if (op->Type == FAUDIOOP_SETCHANNELVOLUMES)
	{
		pFree(op->Data.SetChannelVolumes.pVolumes);
	}
	else if (op->Type == FAUDIOOP_SETOUTPUTMATRIX)
	{
		pFree(op->Data.SetOutputMatrix.pLevelMatrix);
	}
	pFree(op);
}

/* OperationSet Execution */

static inline void ExecuteOperation(FAudio_OPERATIONSET_Operation *op)
{
	switch (op->Type)
	{
	case FAUDIOOP_ENABLEEFFECT:
		FAudioVoice_EnableEffect(
			op->Voice,
			op->Data.EnableEffect.EffectIndex,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_DISABLEEFFECT:
		FAudioVoice_DisableEffect(
			op->Voice,
			op->Data.DisableEffect.EffectIndex,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETEFFECTPARAMETERS:
		FAudioVoice_SetEffectParameters(
			op->Voice,
			op->Data.SetEffectParameters.EffectIndex,
			op->Data.SetEffectParameters.pParameters,
			op->Data.SetEffectParameters.ParametersByteSize,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETFILTERPARAMETERS:
		FAudioVoice_SetFilterParametersEXT(
			op->Voice,
			&op->Data.SetFilterParameters.Parameters,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETOUTPUTFILTERPARAMETERS:
		FAudioVoice_SetOutputFilterParametersEXT(
			op->Voice,
			op->Data.SetOutputFilterParameters.pDestinationVoice,
			&op->Data.SetOutputFilterParameters.Parameters,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETVOLUME:
		FAudioVoice_SetVolume(
			op->Voice,
			op->Data.SetVolume.Volume,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETCHANNELVOLUMES:
		FAudioVoice_SetChannelVolumes(
			op->Voice,
			op->Data.SetChannelVolumes.Channels,
			op->Data.SetChannelVolumes.pVolumes,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETOUTPUTMATRIX:
		FAudioVoice_SetOutputMatrix(
			op->Voice,
			op->Data.SetOutputMatrix.pDestinationVoice,
			op->Data.SetOutputMatrix.SourceChannels,
			op->Data.SetOutputMatrix.DestinationChannels,
			op->Data.SetOutputMatrix.pLevelMatrix,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_START:
		FAudioSourceVoice_Start(
			op->Voice,
			op->Data.Start.Flags,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_STOP:
		FAudioSourceVoice_Stop(
			op->Voice,
			op->Data.Stop.Flags,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_EXITLOOP:
		FAudioSourceVoice_ExitLoop(
			op->Voice,
			FAUDIO_COMMIT_NOW
		);
	break;

	case FAUDIOOP_SETFREQUENCYRATIO:
		FAudioSourceVoice_SetFrequencyRatio(
			op->Voice,
			op->Data.SetFrequencyRatio.Ratio,
			FAUDIO_COMMIT_NOW
		);
	break;

	default:
		FAudio_assert(0 && "Unrecognized operation type!");
	break;
	}
}

void FAudio_OPERATIONSET_CommitAll(FAudio *audio)
{
	FAudio_OPERATIONSET_Operation *op, *next, **committed_end;

	FAudio_PlatformLockMutex(audio->operationLock);
	LOG_MUTEX_LOCK(audio, audio->operationLock)

	if (audio->queuedOperations == NULL)
	{
		FAudio_PlatformUnlockMutex(audio->operationLock);
		LOG_MUTEX_UNLOCK(audio, audio->operationLock)
		return;
	}

	committed_end = &audio->committedOperations;
	while (*committed_end)
	{
		committed_end = &((*committed_end)->next);
	}

	op = audio->queuedOperations;
	do
	{
		next = op->next;

		*committed_end = op;
		op->next = NULL;
		committed_end = &op->next;

		op = next;
	} while (op != NULL);
	audio->queuedOperations = NULL;

	FAudio_PlatformUnlockMutex(audio->operationLock);
	LOG_MUTEX_UNLOCK(audio, audio->operationLock)
}

void FAudio_OPERATIONSET_Commit(FAudio *audio, uint32_t OperationSet)
{
	FAudio_OPERATIONSET_Operation *op, *next, *prev, **committed_end;

	FAudio_PlatformLockMutex(audio->operationLock);
	LOG_MUTEX_LOCK(audio, audio->operationLock)

	if (audio->queuedOperations == NULL)
	{
		FAudio_PlatformUnlockMutex(audio->operationLock);
		LOG_MUTEX_UNLOCK(audio, audio->operationLock)
		return;
	}

	committed_end = &audio->committedOperations;
	while (*committed_end)
	{
		committed_end = &((*committed_end)->next);
	}

	op = audio->queuedOperations;
	prev = NULL;
	do
	{
		next = op->next;
		if (op->OperationSet == OperationSet)
		{
			if (prev == NULL) /* Start of linked list */
			{
				audio->queuedOperations = next;
			}
			else
			{
				prev->next = next;
			}

			*committed_end = op;
			op->next = NULL;
			committed_end = &op->next;
		}
		else
		{
			prev = op;
		}
		op = next;
	} while (op != NULL);

	FAudio_PlatformUnlockMutex(audio->operationLock);
	LOG_MUTEX_UNLOCK(audio, audio->operationLock)
}

void FAudio_OPERATIONSET_Execute(FAudio *audio)
{
	FAudio_OPERATIONSET_Operation *op, *next;

	FAudio_PlatformLockMutex(audio->operationLock);
	LOG_MUTEX_LOCK(audio, audio->operationLock)

	op = audio->committedOperations;
	while (op != NULL)
	{
		next = op->next;
		ExecuteOperation(op);
		DeleteOperation(op, audio->pFree);
		op = next;
	}
	audio->committedOperations = NULL;

	FAudio_PlatformUnlockMutex(audio->operationLock);
	LOG_MUTEX_UNLOCK(audio, audio->operationLock)
}

/* OperationSet Compilation */

static inline FAudio_OPERATIONSET_Operation* QueueOperation(
	FAudioVoice *voice,
	FAudio_OPERATIONSET_Type type,
	uint32_t operationSet
) {
	FAudio_OPERATIONSET_Operation *latest;
	FAudio_OPERATIONSET_Operation *newop = voice->audio->pMalloc(
		sizeof(FAudio_OPERATIONSET_Operation)
	);

	newop->Type = type;
	newop->Voice = voice;
	newop->OperationSet = operationSet;
	newop->next = NULL;

	if (voice->audio->queuedOperations == NULL)
	{
		voice->audio->queuedOperations = newop;
	}
	else
	{
		latest = voice->audio->queuedOperations;
		while (latest->next != NULL)
		{
			latest = latest->next;
		}
		latest->next = newop;
	}

	return newop;
}

void FAudio_OPERATIONSET_QueueEnableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_ENABLEEFFECT,
		OperationSet
	);

	op->Data.EnableEffect.EffectIndex = EffectIndex;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueDisableEffect(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_DISABLEEFFECT,
		OperationSet
	);

	op->Data.DisableEffect.EffectIndex = EffectIndex;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetEffectParameters(
	FAudioVoice *voice,
	uint32_t EffectIndex,
	const void *pParameters,
	uint32_t ParametersByteSize,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETEFFECTPARAMETERS,
		OperationSet
	);

	op->Data.SetEffectParameters.EffectIndex = EffectIndex;
	op->Data.SetEffectParameters.pParameters = voice->audio->pMalloc(
		ParametersByteSize
	);
	FAudio_memcpy(
		op->Data.SetEffectParameters.pParameters,
		pParameters,
		ParametersByteSize
	);
	op->Data.SetEffectParameters.ParametersByteSize = ParametersByteSize;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetFilterParameters(
	FAudioVoice *voice,
	const FAudioFilterParametersEXT *pParameters,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETFILTERPARAMETERS,
		OperationSet
	);

	FAudio_memcpy(
		&op->Data.SetFilterParameters.Parameters,
		pParameters,
		sizeof(FAudioFilterParametersEXT)
	);

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetOutputFilterParameters(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	const FAudioFilterParametersEXT *pParameters,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETOUTPUTFILTERPARAMETERS,
		OperationSet
	);

	op->Data.SetOutputFilterParameters.pDestinationVoice = pDestinationVoice;
	FAudio_memcpy(
		&op->Data.SetOutputFilterParameters.Parameters,
		pParameters,
		sizeof(FAudioFilterParametersEXT)
	);

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetVolume(
	FAudioVoice *voice,
	float Volume,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETVOLUME,
		OperationSet
	);

	op->Data.SetVolume.Volume = Volume;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetChannelVolumes(
	FAudioVoice *voice,
	uint32_t Channels,
	const float *pVolumes,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETCHANNELVOLUMES,
		OperationSet
	);

	op->Data.SetChannelVolumes.Channels = Channels;
	op->Data.SetChannelVolumes.pVolumes = voice->audio->pMalloc(
		sizeof(float) * Channels
	);
	FAudio_memcpy(
		op->Data.SetChannelVolumes.pVolumes,
		pVolumes,
		sizeof(float) * Channels
	);

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetOutputMatrix(
	FAudioVoice *voice,
	FAudioVoice *pDestinationVoice,
	uint32_t SourceChannels,
	uint32_t DestinationChannels,
	const float *pLevelMatrix,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETOUTPUTMATRIX,
		OperationSet
	);

	op->Data.SetOutputMatrix.pDestinationVoice = pDestinationVoice;
	op->Data.SetOutputMatrix.SourceChannels = SourceChannels;
	op->Data.SetOutputMatrix.DestinationChannels = DestinationChannels;
	op->Data.SetOutputMatrix.pLevelMatrix = voice->audio->pMalloc(
		sizeof(float) * SourceChannels * DestinationChannels
	);
	FAudio_memcpy(
		op->Data.SetOutputMatrix.pLevelMatrix,
		pLevelMatrix,
		sizeof(float) * SourceChannels * DestinationChannels
	);

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueStart(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_START,
		OperationSet
	);

	op->Data.Start.Flags = Flags;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueStop(
	FAudioSourceVoice *voice,
	uint32_t Flags,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_STOP,
		OperationSet
	);

	op->Data.Stop.Flags = Flags;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueExitLoop(
	FAudioSourceVoice *voice,
	uint32_t OperationSet
) {
	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	QueueOperation(
		voice,
		FAUDIOOP_EXITLOOP,
		OperationSet
	);

	/* No special data for ExitLoop */

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

void FAudio_OPERATIONSET_QueueSetFrequencyRatio(
	FAudioSourceVoice *voice,
	float Ratio,
	uint32_t OperationSet
) {
	FAudio_OPERATIONSET_Operation *op;

	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	op = QueueOperation(
		voice,
		FAUDIOOP_SETFREQUENCYRATIO,
		OperationSet
	);

	op->Data.SetFrequencyRatio.Ratio = Ratio;

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

/* Called when releasing the engine */

void FAudio_OPERATIONSET_ClearAll(FAudio *audio)
{
	FAudio_OPERATIONSET_Operation *current, *next;

	FAudio_PlatformLockMutex(audio->operationLock);
	LOG_MUTEX_LOCK(audio, audio->operationLock)

	current = audio->queuedOperations;
	while (current != NULL)
	{
		next = current->next;
		DeleteOperation(current, audio->pFree);
		current = next;
	}
	audio->queuedOperations = NULL;

	FAudio_PlatformUnlockMutex(audio->operationLock);
	LOG_MUTEX_UNLOCK(audio, audio->operationLock)
}

/* Called when releasing a voice */

static inline void RemoveFromList(
	FAudioVoice *voice,
	FAudio_OPERATIONSET_Operation **list
) {
	FAudio_OPERATIONSET_Operation *current, *next, *prev;

	current = *list;
	prev = NULL;
	while (current != NULL)
	{
		const uint8_t baseVoice = (voice == current->Voice);
		const uint8_t dstVoice = (
			current->Type == FAUDIOOP_SETOUTPUTFILTERPARAMETERS &&
			voice == current->Data.SetOutputFilterParameters.pDestinationVoice
		) || (
			current->Type == FAUDIOOP_SETOUTPUTMATRIX &&
			voice == current->Data.SetOutputMatrix.pDestinationVoice
		);

		next = current->next;
		if (baseVoice || dstVoice)
		{
			if (prev == NULL) /* Start of linked list */
			{
				*list = next;
			}
			else
			{
				prev->next = next;
			}

			DeleteOperation(current, voice->audio->pFree);
		}
		else
		{
			prev = current;
		}
		current = next;
	}
}

void FAudio_OPERATIONSET_ClearAllForVoice(FAudioVoice *voice)
{
	FAudio_PlatformLockMutex(voice->audio->operationLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->operationLock)

	RemoveFromList(voice, &voice->audio->queuedOperations);
	RemoveFromList(voice, &voice->audio->committedOperations);

	FAudio_PlatformUnlockMutex(voice->audio->operationLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->operationLock)
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
