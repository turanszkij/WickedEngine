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

#ifdef FAUDIO_SDL3_PLATFORM

#include "FAudio_internal.h"

#include <SDL3/SDL.h>

typedef struct SDLAudioDevice
{
	FAudio *audio;
	SDL_AudioStream *stream;
	float *stagingBuffer;
	size_t stagingLen;
} SDLAudioDevice;

/* Mixer Thread */

static void FAudio_INTERNAL_MixCallback(
	void *userdata,
	SDL_AudioStream *stream,
	int additional_amount,
	int total_amount
) {
	SDLAudioDevice *dev = (SDLAudioDevice*) userdata;

	if (!dev->audio->active)
	{
		/* Nothing to do, SDL will fill in for us */
		return;
	}

	while (additional_amount > 0)
	{
		FAudio_zero(dev->stagingBuffer, dev->stagingLen);
		FAudio_INTERNAL_UpdateEngine(dev->audio, dev->stagingBuffer);
		SDL_PutAudioStreamData(
			stream,
			dev->stagingBuffer,
			dev->stagingLen
		);
		additional_amount -= dev->stagingLen;
	}
}

/* Platform Functions */

static void FAudio_INTERNAL_PrioritizeDirectSound()
{
	int numdrivers, i, wasapi, directsound;
	void *dll, *proc;

	if (SDL_GetHint("SDL_AUDIO_DRIVER") != NULL)
	{
		/* Already forced to something, ignore */
		return;
	}

	/* Windows 10+ decided to break version detection, so instead of doing
	 * it the right way we have to do something dumb like search for an
	 * export that's only in Windows 10 or newer.
	 * -flibit
	 */
	if (SDL_strcmp(SDL_GetPlatform(), "Windows") != 0)
	{
		return;
	}
	dll = SDL_LoadObject("USER32.DLL");
	if (dll == NULL)
	{
		return;
	}
	proc = SDL_LoadFunction(dll, "SetProcessDpiAwarenessContext");
	SDL_UnloadObject(dll); /* We aren't really using this, unload now */
	if (proc != NULL)
	{
		/* OS is new enough to trust WASAPI, bail */
		return;
	}

	/* Check to see if we have both Windows drivers in the list */
	numdrivers = SDL_GetNumAudioDrivers();
	wasapi = -1;
	directsound = -1;
	for (i = 0; i < numdrivers; i += 1)
	{
		const char *driver = SDL_GetAudioDriver(i);
		if (SDL_strcmp(driver, "wasapi") == 0)
		{
			wasapi = i;
		}
		else if (SDL_strcmp(driver, "directsound") == 0)
		{
			directsound = i;
		}
	}

	/* We force if and only if both drivers exist and wasapi is earlier */
	if ((wasapi > -1) && (directsound > -1))
	{
		if (wasapi < directsound)
		{
			SDL_SetHint("SDL_AUDIO_DRIVER", "directsound");
		}
	}
}

void FAudio_PlatformAddRef()
{
	FAudio_INTERNAL_PrioritizeDirectSound();

	/* SDL tracks ref counts for each subsystem */
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		SDL_Log("SDL_INIT_AUDIO failed: %s", SDL_GetError());
	}
	FAudio_INTERNAL_InitSIMDFunctions(
		SDL_HasSSE2(),
		SDL_HasNEON()
	);
}

void FAudio_PlatformRelease()
{
	/* SDL tracks ref counts for each subsystem */
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void FAudio_PlatformInit(
	FAudio *audio,
	uint32_t flags,
	uint32_t deviceIndex,
	FAudioWaveFormatExtensible *mixFormat,
	uint32_t *updateSize,
	void** platformDevice
) {
	SDLAudioDevice *result;
	SDL_AudioDeviceID devID;
	SDL_AudioSpec spec;
	int wantSamples;

	FAudio_assert(mixFormat != NULL);
	FAudio_assert(updateSize != NULL);

	/* Build the device spec */
	spec.freq = mixFormat->Format.nSamplesPerSec;
	spec.format = SDL_AUDIO_F32;
	spec.channels = mixFormat->Format.nChannels;
	if (flags & FAUDIO_1024_QUANTUM)
	{
		/* Get the sample count for a 21.33ms frame.
		 * For 48KHz this should be 1024.
		 */
		wantSamples = (int) (
			spec.freq / (1000.0 / (64.0 / 3.0))
		);
	}
	else
	{
		wantSamples = spec.freq / 100;
	}

	if (deviceIndex == 0)
	{
		devID = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
	}
	else
	{
		int devcount;
		SDL_AudioDeviceID *devs = SDL_GetAudioPlaybackDevices(&devcount);

		/* Bounds checking is done before this function is called */
		devID = devs[deviceIndex - 1];

		SDL_free(devs);
	}

	result = (SDLAudioDevice*) SDL_malloc(sizeof(SDLAudioDevice));
	result->audio = audio;
	result->stagingLen = wantSamples * spec.channels * sizeof(float);
	result->stagingBuffer = (float*) SDL_malloc(result->stagingLen);

	/* Open the device (or at least try to) */
	result->stream = SDL_OpenAudioDeviceStream(
		devID,
		&spec,
		FAudio_INTERNAL_MixCallback,
		result
	);

	/* Write up the received format for the engine */
	WriteWaveFormatExtensible(
		mixFormat,
		spec.channels,
		spec.freq,
		&DATAFORMAT_SUBTYPE_IEEE_FLOAT
	);
	*updateSize = wantSamples;

	/* SDL_AudioDeviceID is a Uint32, anybody using a 16-bit PC still? */
	*platformDevice = result;

	/* Start the thread! */
	SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(result->stream));
}

void FAudio_PlatformQuit(void* platformDevice)
{
	SDLAudioDevice *dev = (SDLAudioDevice*) platformDevice;
	SDL_AudioDeviceID devID = SDL_GetAudioStreamDevice(dev->stream);
	SDL_DestroyAudioStream(dev->stream);
	SDL_CloseAudioDevice(devID);
	SDL_free(dev->stagingBuffer);
	SDL_free(dev);
}

uint32_t FAudio_PlatformGetDeviceCount()
{
	int devcount;
	SDL_free(SDL_GetAudioPlaybackDevices(&devcount));
	if (devcount == 0)
	{
		return 0;
	}
	SDL_assert(devcount > 0);
	return devcount + 1; /* Add one for "Default Device" */
}

void FAudio_UTF8_To_UTF16(const char *src, uint16_t *dst, size_t len);

uint32_t FAudio_PlatformGetDeviceDetails(
	uint32_t index,
	FAudioDeviceDetails *details
) {
	const char *name, *envvar;
	int channels, rate;
	SDL_AudioSpec spec;
	int devcount;
	SDL_AudioDeviceID *devs;

	FAudio_zero(details, sizeof(FAudioDeviceDetails));

	devs = SDL_GetAudioPlaybackDevices(&devcount);
	if (index > devcount)
	{
		SDL_free(devs);
		return FAUDIO_E_INVALID_CALL;
	}

	details->DeviceID[0] = L'0' + index;
	if (index == 0)
	{
		name = "Default Device";
		details->Role = FAudioGlobalDefaultDevice;

		/* This variable will look like a DSound GUID or WASAPI ID, i.e.
		 * "{0.0.0.00000000}.{FD47D9CC-4218-4135-9CE2-0C195C87405B}"
		 */
		envvar = SDL_getenv("FAUDIO_FORCE_DEFAULT_DEVICEID");
		if (envvar != NULL)
		{
			FAudio_UTF8_To_UTF16(
				envvar,
				(uint16_t*) details->DeviceID,
				sizeof(details->DeviceID)
			);
		}
	}
	else
	{
		name = SDL_GetAudioDeviceName(devs[index - 1]);
		details->Role = FAudioNotDefaultDevice;
	}
	FAudio_UTF8_To_UTF16(
		name,
		(uint16_t*) details->DisplayName,
		sizeof(details->DisplayName)
	);

	/* Environment variables take precedence over all possible values */
	envvar = SDL_getenv("SDL_AUDIO_FREQUENCY");
	if (envvar != NULL)
	{
		rate = SDL_atoi(envvar);
	}
	else
	{
		rate = 0;
	}
	envvar = SDL_getenv("SDL_AUDIO_CHANNELS");
	if (envvar != NULL)
	{
		channels = SDL_atoi(envvar);
	}
	else
	{
		channels = 0;
	}

	/* Get the device format from the OS */
	if (index == 0)
	{
		if (!SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL))
		{
			SDL_zero(spec);
		}
	}
	else
	{
		if (!SDL_GetAudioDeviceFormat(devs[index - 1], &spec, NULL))
		{
			SDL_zero(spec);
		}
	}
	SDL_free(devs);
	if ((spec.freq > 0) && (rate <= 0))
	{
		rate = spec.freq;
	}
	if ((spec.channels > 0) && (spec.channels < 9) && (channels <= 0))
	{
		channels = spec.channels;
	}

	/* If we make it all the way here with no format, hardcode a sane one */
	if (rate <= 0)
	{
		rate = 48000;
	}
	if (channels <= 0)
	{
		channels = 2;
	}

	/* Write the format, finally. */
	WriteWaveFormatExtensible(
		&details->OutputFormat,
		channels,
		rate,
		&DATAFORMAT_SUBTYPE_PCM
	);
	return 0;
}

/* Threading */

FAudioThread FAudio_PlatformCreateThread(
	FAudioThreadFunc func,
	const char *name,
	void* data
) {
	return (FAudioThread) SDL_CreateThread(
		(SDL_ThreadFunction) func,
		name,
		data
	);
}

void FAudio_PlatformWaitThread(FAudioThread thread, int32_t *retval)
{
	SDL_WaitThread((SDL_Thread*) thread, retval);
}

void FAudio_PlatformThreadPriority(FAudioThreadPriority priority)
{
	SDL_SetCurrentThreadPriority((SDL_ThreadPriority) priority);
}

uint64_t FAudio_PlatformGetThreadID(void)
{
	return (uint64_t) SDL_GetCurrentThreadID();
}

FAudioMutex FAudio_PlatformCreateMutex()
{
	return (FAudioMutex) SDL_CreateMutex();
}

void FAudio_PlatformDestroyMutex(FAudioMutex mutex)
{
	SDL_DestroyMutex((SDL_Mutex*) mutex);
}

void FAudio_PlatformLockMutex(FAudioMutex mutex)
{
	SDL_LockMutex((SDL_Mutex*) mutex);
}

void FAudio_PlatformUnlockMutex(FAudioMutex mutex)
{
	SDL_UnlockMutex((SDL_Mutex*) mutex);
}

void FAudio_sleep(uint32_t ms)
{
	SDL_Delay(ms);
}

/* Time */

uint32_t FAudio_timems()
{
	return SDL_GetTicks();
}

/* FAudio I/O */

static size_t FAUDIOCALL FAudio_INTERNAL_ioread(
	void *data,
	void *dst,
	size_t size,
	size_t count
) {
	return SDL_ReadIO((SDL_IOStream*) data, dst, size * count) / size;
}

static int64_t FAUDIOCALL FAudio_INTERNAL_ioseek(
	void *data,
	int64_t offset,
	int whence
) {
	return SDL_SeekIO((SDL_IOStream*) data, offset, whence);
}

static int FAUDIOCALL FAudio_INTERNAL_ioclose(
	void *data
) {
	return SDL_CloseIO((SDL_IOStream*) data);
}

FAudioIOStream* FAudio_fopen(const char *path)
{
	FAudioIOStream *io = (FAudioIOStream*) FAudio_malloc(
		sizeof(FAudioIOStream)
	);
	SDL_IOStream *stream = SDL_IOFromFile(path, "rb");
	io->data = stream;
	io->read = FAudio_INTERNAL_ioread;
	io->seek = FAudio_INTERNAL_ioseek;
	io->close = FAudio_INTERNAL_ioclose;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

FAudioIOStream* FAudio_memopen(void *mem, int len)
{
	FAudioIOStream *io = (FAudioIOStream*) FAudio_malloc(
		sizeof(FAudioIOStream)
	);
	SDL_IOStream *stream = SDL_IOFromMem(mem, len);
	io->data = stream;
	io->read = FAudio_INTERNAL_ioread;
	io->seek = FAudio_INTERNAL_ioseek;
	io->close = FAudio_INTERNAL_ioclose;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

uint8_t* FAudio_memptr(FAudioIOStream *io, size_t offset)
{
	SDL_PropertiesID props = SDL_GetIOProperties((SDL_IOStream*) io->data);
	FAudio_assert(SDL_HasProperty(props, SDL_PROP_IOSTREAM_MEMORY_POINTER));
	return ((uint8_t*) SDL_GetPointerProperty(props, SDL_PROP_IOSTREAM_MEMORY_POINTER, NULL)) + offset;
}

void FAudio_close(FAudioIOStream *io)
{
	io->close(io->data);
	FAudio_PlatformDestroyMutex((FAudioMutex) io->lock);
	FAudio_free(io);
}

#ifdef FAUDIO_DUMP_VOICES
static size_t FAUDIOCALL FAudio_INTERNAL_iowrite(
	void *data,
	const void *src,
	size_t size,
	size_t count
) {
	SDL_WriteIO((SDL_IOStream*) data, src, size * count);
}

static size_t FAUDIOCALL FAudio_INTERNAL_iosize(
	void *data
) {
	return SDL_GetIOSize((SDL_IOStream*) data);
}

FAudioIOStreamOut* FAudio_fopen_out(const char *path, const char *mode)
{
	FAudioIOStreamOut *io = (FAudioIOStreamOut*) FAudio_malloc(
		sizeof(FAudioIOStreamOut)
	);
	SDL_IOStream *stream = SDL_IOFromFile(path, mode);
	io->data = stream;
	io->read = FAudio_INTERNAL_ioread;
	io->write = FAudio_INTERNAL_iowrite;
	io->seek = FAudio_INTERNAL_ioseek;
	io->size = FAudio_INTERNAL_iosize;
	io->close = FAudio_INTERNAL_ioclose;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

void FAudio_close_out(FAudioIOStreamOut *io)
{
	io->close(io->data);
	FAudio_PlatformDestroyMutex((FAudioMutex) io->lock);
	FAudio_free(io);
}
#endif /* FAUDIO_DUMP_VOICES */

/* UTF8->UTF16 Conversion, taken from PhysicsFS */

#define UNICODE_BOGUS_CHAR_VALUE 0xFFFFFFFF
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

static uint32_t FAudio_UTF8_CodePoint(const char **_str)
{
    const char *str = *_str;
    uint32_t retval = 0;
    uint32_t octet = (uint32_t) ((uint8_t) *str);
    uint32_t octet2, octet3, octet4;

    if (octet == 0)  /* null terminator, end of string. */
        return 0;

    else if (octet < 128)  /* one octet char: 0 to 127 */
    {
        (*_str)++;  /* skip to next possible start of codepoint. */
        return octet;
    } /* else if */

    else if ((octet > 127) && (octet < 192))  /* bad (starts with 10xxxxxx). */
    {
        /*
         * Apparently each of these is supposed to be flagged as a bogus
         *  char, instead of just resyncing to the next valid codepoint.
         */
        (*_str)++;  /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    } /* else if */

    else if (octet < 224)  /* two octets */
    {
        (*_str)++;  /* advance at least one byte in case of an error */
        octet -= (128+64);
        octet2 = (uint32_t) ((uint8_t) *(++str));
        if ((octet2 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 1;  /* skip to next possible start of codepoint. */
        retval = ((octet << 6) | (octet2 - 128));
        if ((retval >= 0x80) && (retval <= 0x7FF))
            return retval;
    } /* else if */

    else if (octet < 240)  /* three octets */
    {
        (*_str)++;  /* advance at least one byte in case of an error */
        octet -= (128+64+32);
        octet2 = (uint32_t) ((uint8_t) *(++str));
        if ((octet2 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (uint32_t) ((uint8_t) *(++str));
        if ((octet3 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 2;  /* skip to next possible start of codepoint. */
        retval = ( ((octet << 12)) | ((octet2-128) << 6) | ((octet3-128)) );

        /* There are seven "UTF-16 surrogates" that are illegal in UTF-8. */
        switch (retval)
        {
            case 0xD800:
            case 0xDB7F:
            case 0xDB80:
            case 0xDBFF:
            case 0xDC00:
            case 0xDF80:
            case 0xDFFF:
                return UNICODE_BOGUS_CHAR_VALUE;
        } /* switch */

        /* 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge. */
        if ((retval >= 0x800) && (retval <= 0xFFFD))
            return retval;
    } /* else if */

    else if (octet < 248)  /* four octets */
    {
        (*_str)++;  /* advance at least one byte in case of an error */
        octet -= (128+64+32+16);
        octet2 = (uint32_t) ((uint8_t) *(++str));
        if ((octet2 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet3 = (uint32_t) ((uint8_t) *(++str));
        if ((octet3 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet4 = (uint32_t) ((uint8_t) *(++str));
        if ((octet4 & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 3;  /* skip to next possible start of codepoint. */
        retval = ( ((octet << 18)) | ((octet2 - 128) << 12) |
                   ((octet3 - 128) << 6) | ((octet4 - 128)) );
        if ((retval >= 0x10000) && (retval <= 0x10FFFF))
            return retval;
    } /* else if */

    /*
     * Five and six octet sequences became illegal in rfc3629.
     *  We throw the codepoint away, but parse them to make sure we move
     *  ahead the right number of bytes and don't overflow the buffer.
     */

    else if (octet < 252)  /* five octets */
    {
        (*_str)++;  /* advance at least one byte in case of an error */
        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 4;  /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    } /* else if */

    else  /* six octets */
    {
        (*_str)++;  /* advance at least one byte in case of an error */
        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        octet = (uint32_t) ((uint8_t) *(++str));
        if ((octet & (128+64)) != 128)  /* Format isn't 10xxxxxx? */
            return UNICODE_BOGUS_CHAR_VALUE;

        *_str += 6;  /* skip to next possible start of codepoint. */
        return UNICODE_BOGUS_CHAR_VALUE;
    } /* else if */

    return UNICODE_BOGUS_CHAR_VALUE;
}

void FAudio_UTF8_To_UTF16(const char *src, uint16_t *dst, size_t len)
{
    len -= sizeof (uint16_t);   /* save room for null char. */
    while (len >= sizeof (uint16_t))
    {
        uint32_t cp = FAudio_UTF8_CodePoint(&src);
        if (cp == 0)
            break;
        else if (cp == UNICODE_BOGUS_CHAR_VALUE)
            cp = UNICODE_BOGUS_CHAR_CODEPOINT;

        if (cp > 0xFFFF)  /* encode as surrogate pair */
        {
            if (len < (sizeof (uint16_t) * 2))
                break;  /* not enough room for the pair, stop now. */

            cp -= 0x10000;  /* Make this a 20-bit value */

            *(dst++) = 0xD800 + ((cp >> 10) & 0x3FF);
            len -= sizeof (uint16_t);

            cp = 0xDC00 + (cp & 0x3FF);
        } /* if */

        *(dst++) = cp;
        len -= sizeof (uint16_t);
    } /* while */

    *dst = 0;
}

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */

#else

extern int this_tu_is_empty;

#endif /* FAUDIO_SDL3_PLATFORM */
