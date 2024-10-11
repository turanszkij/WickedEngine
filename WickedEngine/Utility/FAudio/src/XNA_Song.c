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

#ifndef DISABLE_XNASONG

#include "FAudio_internal.h"

/* stb_vorbis */

#define malloc FAudio_malloc
#define realloc FAudio_realloc
#define free FAudio_free
#ifndef NO_MEMSET_OVERRIDE
#ifdef memset /* Thanks, Apple! */
#undef memset
#endif
#define memset FAudio_memset
#endif /* NO_MEMSET_OVERRIDE */
#ifndef NO_MEMCPY_OVERRIDE
#ifdef memcpy /* Thanks, Apple! */
#undef memcpy
#endif
#define memcpy FAudio_memcpy
#endif /* NO_MEMCPY_OVERRIDE */
#define memcmp FAudio_memcmp

#define pow FAudio_pow
#define log(x) FAudio_log(x)
#define sin(x) FAudio_sin(x)
#define cos(x) FAudio_cos(x)
#define floor FAudio_floor
#define abs(x) FAudio_abs(x)
#define ldexp(v, e) FAudio_ldexp((v), (e))
#define exp(x) FAudio_exp(x)

#define qsort FAudio_qsort

#define assert FAudio_assert

#define FILE FAudioIOStream
#ifdef SEEK_SET
#undef SEEK_SET
#endif
#ifdef SEEK_END
#undef SEEK_END
#endif
#ifdef EOF
#undef EOF
#endif
#define SEEK_SET FAUDIO_SEEK_SET
#define SEEK_END FAUDIO_SEEK_END
#define EOF FAUDIO_EOF
#define fopen(path, mode) FAudio_fopen(path)
#define fopen_s(io, path, mode) (!(*io = FAudio_fopen(path)))
#define fclose(io) FAudio_close(io)
#define fread(dst, size, count, io) io->read(io->data, dst, size, count)
#define fseek(io, offset, whence) io->seek(io->data, offset, whence)
#define ftell(io) io->seek(io->data, 0, FAUDIO_SEEK_CUR)

#define STB_VORBIS_NO_PUSHDATA_API 1
#define STB_VORBIS_NO_INTEGER_CONVERSION 1
#include "stb_vorbis.h"

#include "qoa_decoder.h"

/* Globals */

static float songVolume = 1.0f;
static FAudio *songAudio = NULL;
static FAudioMasteringVoice *songMaster = NULL;
static unsigned int songLength = 0;
static unsigned int songOffset = 0;

static FAudioSourceVoice *songVoice = NULL;
static FAudioVoiceCallback callbacks;
static stb_vorbis *activeVorbisSong = NULL;
static stb_vorbis_info activeVorbisSongInfo;

static qoa *activeQoaSong = NULL;
static unsigned int qoaChannels = 0;
static unsigned int qoaSampleRate = 0;
static unsigned int qoaSamplesPerChannelPerFrame = 0;
static unsigned int qoaTotalSamplesPerChannel = 0;

static uint8_t *songCache;

/* Internal Functions */

static void XNA_SongSubmitBuffer(FAudioVoiceCallback *callback, void *pBufferContext)
{
	FAudioBuffer buffer;
	uint32_t decoded = 0;

	if (activeVorbisSong != NULL)
	{
		decoded = stb_vorbis_get_samples_float_interleaved(
			activeVorbisSong,
			activeVorbisSongInfo.channels,
			(float*) songCache,
			activeVorbisSongInfo.sample_rate * activeVorbisSongInfo.channels
		);
		buffer.AudioBytes = decoded * activeVorbisSongInfo.channels * sizeof(float);
	}
	else if (activeQoaSong != NULL)
	{
		/* TODO: decode multiple frames? */
		decoded = qoa_decode_next_frame(
			activeQoaSong,
			(short*) songCache
		);
		buffer.AudioBytes = decoded * qoaChannels * sizeof(short);
	}

	if (decoded == 0)
	{
		return;
	}

	songOffset += decoded;
	buffer.Flags = (songOffset >= songLength) ? FAUDIO_END_OF_STREAM : 0;
	buffer.pAudioData = songCache;
	buffer.PlayBegin = 0;
	buffer.PlayLength = decoded;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = NULL;
	FAudioSourceVoice_SubmitSourceBuffer(
		songVoice,
		&buffer,
		NULL
	);
}

static void XNA_SongKill()
{
	if (songVoice != NULL)
	{
		FAudioSourceVoice_Stop(songVoice, 0, 0);
		FAudioVoice_DestroyVoice(songVoice);
		songVoice = NULL;
	}
	if (songCache != NULL)
	{
		FAudio_free(songCache);
		songCache = NULL;
	}
	if (activeVorbisSong != NULL)
	{
		stb_vorbis_close(activeVorbisSong);
		activeVorbisSong = NULL;
	}
	if (activeQoaSong != NULL)
	{
		qoa_close(activeQoaSong);
		activeQoaSong = NULL;
	}
}

/* "Public" API */

FAUDIOAPI void XNA_SongInit()
{
	FAudioCreate(&songAudio, 0, FAUDIO_DEFAULT_PROCESSOR);
	FAudio_CreateMasteringVoice(
		songAudio,
		&songMaster,
		FAUDIO_DEFAULT_CHANNELS,
		FAUDIO_DEFAULT_SAMPLERATE,
		0,
		0,
		NULL
	);
}

FAUDIOAPI void XNA_SongQuit()
{
	XNA_SongKill();
	FAudioVoice_DestroyVoice(songMaster);
	FAudio_Release(songAudio);
}

FAUDIOAPI float XNA_PlaySong(const char *name)
{
	FAudioWaveFormatEx format;
	XNA_SongKill();

	activeVorbisSong = stb_vorbis_open_filename(name, NULL, NULL);

	if (activeVorbisSong != NULL)
	{
		activeVorbisSongInfo = stb_vorbis_get_info(activeVorbisSong);
		format.wFormatTag = FAUDIO_FORMAT_IEEE_FLOAT;
		format.nChannels = activeVorbisSongInfo.channels;
		format.nSamplesPerSec = activeVorbisSongInfo.sample_rate;
		format.wBitsPerSample = sizeof(float) * 8;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		songOffset = 0;
		songLength = stb_vorbis_stream_length_in_samples(activeVorbisSong);
	}
	else /* It's not vorbis, try qoa!*/
	{
		activeQoaSong = qoa_open_from_filename(name);

		if (activeQoaSong == NULL)
		{
			/* It's neither vorbis nor qoa, time to bail */
			return 0;
		}

		qoa_attributes(activeQoaSong, &qoaChannels, &qoaSampleRate, &qoaSamplesPerChannelPerFrame, &qoaTotalSamplesPerChannel);

		format.wFormatTag = FAUDIO_FORMAT_PCM;
		format.nChannels = qoaChannels;
		format.nSamplesPerSec = qoaSampleRate;
		format.wBitsPerSample = 16;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		songOffset = 0;
		songLength = qoaTotalSamplesPerChannel;
	}

	/* Allocate decode cache */
	songCache = (uint8_t*) FAudio_malloc(format.nAvgBytesPerSec);

	/* Init voice */
	FAudio_zero(&callbacks, sizeof(FAudioVoiceCallback));
	callbacks.OnBufferEnd = XNA_SongSubmitBuffer;
	FAudio_CreateSourceVoice(
		songAudio,
		&songVoice,
		&format,
		0,
		1.0f, /* No pitch shifting here! */
		&callbacks,
		NULL,
		NULL
	);
	FAudioVoice_SetVolume(songVoice, songVolume, 0);

	/* Okay, this song is decoding now */
	if (activeVorbisSong != NULL)
	{
		stb_vorbis_seek_start(activeVorbisSong);
	}
	else if (activeQoaSong != NULL)
	{
		qoa_seek_frame(activeQoaSong, 0);
	}

	XNA_SongSubmitBuffer(NULL, NULL);

	/* Finally. */
	FAudioSourceVoice_Start(songVoice, 0, 0);

	if (activeVorbisSong != NULL)
	{
		return stb_vorbis_stream_length_in_seconds(activeVorbisSong);
	}
	else if (activeQoaSong != NULL)
	{
		return qoaTotalSamplesPerChannel / (float) qoaSampleRate;
	}

	return 0;
}

FAUDIOAPI void XNA_PauseSong()
{
	if (songVoice == NULL)
	{
		return;
	}
	FAudioSourceVoice_Stop(songVoice, 0, 0);
}

FAUDIOAPI void XNA_ResumeSong()
{
	if (songVoice == NULL)
	{
		return;
	}
	FAudioSourceVoice_Start(songVoice, 0, 0);
}

FAUDIOAPI void XNA_StopSong()
{
	XNA_SongKill();
}

FAUDIOAPI void XNA_SetSongVolume(float volume)
{
	songVolume = volume;
	if (songVoice != NULL)
	{
		FAudioVoice_SetVolume(songVoice, songVolume, 0);
	}
}

FAUDIOAPI uint32_t XNA_GetSongEnded()
{
	FAudioVoiceState state;
	if (songVoice == NULL || (activeVorbisSong == NULL && activeQoaSong == NULL))
	{
		return 1;
	}
	FAudioSourceVoice_GetState(songVoice, &state, 0);
	return state.BuffersQueued == 0 && state.SamplesPlayed == 0;
}

FAUDIOAPI void XNA_EnableVisualization(uint32_t enable)
{
	/* TODO: Enable/Disable FAPO effect */
}

FAUDIOAPI uint32_t XNA_VisualizationEnabled()
{
	/* TODO: Query FAPO effect enabled */
	return 0;
}

FAUDIOAPI void XNA_GetSongVisualizationData(
	float *frequencies,
	float *samples,
	uint32_t count
) {
	/* TODO: Visualization FAPO that reads in Song samples, FFT analysis */
}

#endif /* DISABLE_XNASONG */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
