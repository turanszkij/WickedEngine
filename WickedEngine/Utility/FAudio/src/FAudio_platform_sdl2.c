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

#ifndef FAUDIO_WIN32_PLATFORM

#include "FAudio_internal.h"

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2, 0, 9)
#error "SDL version older than 2.0.9"
#endif /* !SDL_VERSION_ATLEAST */

/* Mixer Thread */

static void FAudio_INTERNAL_MixCallback(void *userdata, Uint8 *stream, int len)
{
	FAudio *audio = (FAudio*) userdata;

	FAudio_zero(stream, len);
	if (audio->active)
	{
		FAudio_INTERNAL_UpdateEngine(
			audio,
			(float*) stream
		);
	}
}

/* Platform Functions */

void FAudio_PlatformAddRef()
{
	/* SDL tracks ref counts for each subsystem */
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
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
	SDL_AudioDeviceID device;
	SDL_AudioSpec want, have;
	const char *driver;
	int changes = 0;

	FAudio_assert(mixFormat != NULL);
	FAudio_assert(updateSize != NULL);

	/* Build the device spec */
	want.freq = mixFormat->Format.nSamplesPerSec;
	want.format = AUDIO_F32;
	want.channels = mixFormat->Format.nChannels;
	want.silence = 0;
	want.callback = FAudio_INTERNAL_MixCallback;
	want.userdata = audio;
	if (flags & FAUDIO_1024_QUANTUM)
	{
		/* Get the sample count for a 21.33ms frame.
		 * For 48KHz this should be 1024.
		 */
		want.samples = (int) (
			want.freq / (1000.0 / (64.0 / 3.0))
		);
	}
	else
	{
		want.samples = want.freq / 100;
	}

	/* FIXME: SDL bug!
	 * The PulseAudio backend does this annoying thing where it halves the
	 * buffer size to prevent latency issues:
	 *
	 * https://hg.libsdl.org/SDL/file/df343364c6c5/src/audio/pulseaudio/SDL_pulseaudio.c#l577
	 *
	 * To get the _actual_ quantum size we want, we just double the buffer
	 * size and allow SDL to set the quantum size back to normal.
	 * -flibit
	 */
	driver = SDL_GetCurrentAudioDriver();
	if (SDL_strcmp(driver, "pulseaudio") == 0)
	{
		want.samples *= 2;
		changes = SDL_AUDIO_ALLOW_SAMPLES_CHANGE;
	}

	/* FIXME: SDL bug!
	 * The most common backends support varying samples values, but many
	 * require a power-of-two value, which XAudio2 is not a fan of.
	 * Normally SDL creates an intermediary stream to handle this, but this
	 * has not been written yet:
	 * https://bugzilla.libsdl.org/show_bug.cgi?id=5136
	 * -flibit
	 */
	else if (	SDL_strcmp(driver, "emscripten") == 0 ||
			SDL_strcmp(driver, "dsp") == 0	)
	{
		want.samples -= 1;
		want.samples |= want.samples >> 1;
		want.samples |= want.samples >> 2;
		want.samples |= want.samples >> 4;
		want.samples |= want.samples >> 8;
		want.samples |= want.samples >> 16;
		want.samples += 1;
		SDL_Log(
			"Forcing FAudio quantum to a power-of-two.\n"
			"You don't actually want this, it's technically a bug:\n"
			"https://bugzilla.libsdl.org/show_bug.cgi?id=5136"
		);
	}

	/* Open the device (or at least try to) */
iosretry:
	device = SDL_OpenAudioDevice(
		deviceIndex > 0 ? SDL_GetAudioDeviceName(deviceIndex - 1, 0) : NULL,
		0,
		&want,
		&have,
		changes
	);
	if (device == 0)
	{
		const char *err = SDL_GetError();
		SDL_Log("OpenAudioDevice failed: %s", err);

		/* iOS has a weird thing where you can't open a stream when the
		 * app is in the background, even though the program is meant
		 * to be suspended and thus not trip this in the first place.
		 *
		 * Startup suspend behavior when an app is opened then closed
		 * is a big pile of crap, basically.
		 *
		 * Google the error code and you'll find that this has been a
		 * long-standing issue that nobody seems to care about.
		 * -flibit
		 */
		if (SDL_strstr(err, "Code=561015905") != NULL)
		{
			goto iosretry;
		}

		FAudio_assert(0 && "Failed to open audio device!");
		return;
	}

	/* Write up the received format for the engine */
	WriteWaveFormatExtensible(
		mixFormat,
		have.channels,
		have.freq,
		&DATAFORMAT_SUBTYPE_IEEE_FLOAT
	);
	*updateSize = have.samples;

	/* SDL_AudioDeviceID is a Uint32, anybody using a 16-bit PC still? */
	*platformDevice = (void*) ((size_t) device);

	/* Start the thread! */
	SDL_PauseAudioDevice(device, 0);
}

void FAudio_PlatformQuit(void* platformDevice)
{
	SDL_CloseAudioDevice((SDL_AudioDeviceID) ((size_t) platformDevice));
}

uint32_t FAudio_PlatformGetDeviceCount()
{
	uint32_t devCount = SDL_GetNumAudioDevices(0);
	if (devCount == 0)
	{
		return 0;
	}
	return devCount + 1; /* Add one for "Default Device" */
}

void FAudio_UTF8_To_UTF16(const char *src, uint16_t *dst, size_t len);

uint32_t FAudio_PlatformGetDeviceDetails(
	uint32_t index,
	FAudioDeviceDetails *details
) {
	const char *name, *envvar;
	int channels, rate;
	SDL_AudioSpec spec;
	uint32_t devcount, i;

	FAudio_zero(details, sizeof(FAudioDeviceDetails));

	devcount = FAudio_PlatformGetDeviceCount();
	if (index >= devcount)
	{
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
		name = SDL_GetAudioDeviceName(index - 1, 0);
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

#if SDL_VERSION_ATLEAST(2, 0, 15)
	if (index == 0)
	{
		/* Okay, so go grab something from the liquor cabinet and get
		 * ready, because this loop is a bit of a trip:
		 *
		 * We can't get the spec for the default device, because in
		 * audio land a "default device" is a completely foreign idea,
		 * some APIs support it but in reality you just have to pass
		 * NULL as a driver string and the sound server figures out the
		 * rest. In some psychotic universe the device can even be a
		 * network address. No, seriously.
		 *
		 * So what do we do? Well, at least in my experience shipping
		 * for the PC, the easiest thing to do is assume that the
		 * highest spec in the list is what you should target, even if
		 * it turns out that's not the default at the time you create
		 * your device.
		 *
		 * Consider a laptop that has built-in stereo speakers, but is
		 * connected to a home theater system with 5.1 audio. It may be
		 * the case that the stereo audio is active, but the user may
		 * at some point move audio to 5.1, at which point the server
		 * will simply move the endpoint from underneath us and move our
		 * output stream to the new device. At that point, you _really_
		 * want to already be pushing out 5.1, because if not the user
		 * will be stuck recreating the whole program, which on many
		 * platforms is an instant cert failure. The tradeoff is that
		 * you're potentially downmixing a 5.1 stream to stereo, which
		 * is a bit wasteful, but presumably the hardware can handle it
		 * if they were able to use a 5.1 system to begin with.
		 *
		 * So, we just aim for the highest channel count on the system.
		 * We also do this with sample rate to a lesser degree; we try
		 * to use a single device spec at a time, so it may be that
		 * the sample rate you get isn't the highest from the list if
		 * another device had a higher channel count.
		 *
		 * Lastly, if you set SDL_AUDIO_CHANNELS but not
		 * SDL_AUDIO_FREQUENCY, we don't bother checking for a sample
		 * rate, we fall through to the hardcoded value at the bottom of
		 * this function.
		 *
		 * I'm so tired.
		 *
		 * -flibit
		 */
		if (channels <= 0)
		{
			const uint8_t setRate = (rate <= 0);
			devcount -= 1; /* Subtracting the default index */
			for (i = 0; i < devcount; i += 1)
			{
				SDL_GetAudioDeviceSpec(i, 0, &spec);
				if (	(spec.channels > channels) &&
					(spec.channels <= 8)	)
				{
					channels = spec.channels;
					if (setRate)
					{
						/* May be 0! That's okay! */
						rate = spec.freq;
					}
				}
			}
		}
	}
	else
	{
		SDL_GetAudioDeviceSpec(index - 1, 0, &spec);
		if ((spec.freq > 0) && (rate <= 0))
		{
			rate = spec.freq;
		}
		if ((spec.channels > 0) && (channels <= 0))
		{
			channels = spec.channels;
		}
	}
#endif /* SDL >= 2.0.15 */

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
	SDL_SetThreadPriority((SDL_ThreadPriority) priority);
}

uint64_t FAudio_PlatformGetThreadID(void)
{
	return (uint64_t) SDL_ThreadID();
}

FAudioMutex FAudio_PlatformCreateMutex()
{
	return (FAudioMutex) SDL_CreateMutex();
}

void FAudio_PlatformDestroyMutex(FAudioMutex mutex)
{
	SDL_DestroyMutex((SDL_mutex*) mutex);
}

void FAudio_PlatformLockMutex(FAudioMutex mutex)
{
	SDL_LockMutex((SDL_mutex*) mutex);
}

void FAudio_PlatformUnlockMutex(FAudioMutex mutex)
{
	SDL_UnlockMutex((SDL_mutex*) mutex);
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

FAudioIOStream* FAudio_fopen(const char *path)
{
	FAudioIOStream *io = (FAudioIOStream*) FAudio_malloc(
		sizeof(FAudioIOStream)
	);
	SDL_RWops *rwops = SDL_RWFromFile(path, "rb");
	io->data = rwops;
	io->read = (FAudio_readfunc) rwops->read;
	io->seek = (FAudio_seekfunc) rwops->seek;
	io->close = (FAudio_closefunc) rwops->close;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

FAudioIOStream* FAudio_memopen(void *mem, int len)
{
	FAudioIOStream *io = (FAudioIOStream*) FAudio_malloc(
		sizeof(FAudioIOStream)
	);
	SDL_RWops *rwops = SDL_RWFromMem(mem, len);
	io->data = rwops;
	io->read = (FAudio_readfunc) rwops->read;
	io->seek = (FAudio_seekfunc) rwops->seek;
	io->close = (FAudio_closefunc) rwops->close;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

uint8_t* FAudio_memptr(FAudioIOStream *io, size_t offset)
{
	SDL_RWops *rwops = (SDL_RWops*) io->data;
	FAudio_assert(rwops->type == SDL_RWOPS_MEMORY);
	return rwops->hidden.mem.base + offset;
}

void FAudio_close(FAudioIOStream *io)
{
	io->close(io->data);
	FAudio_PlatformDestroyMutex((FAudioMutex) io->lock);
	FAudio_free(io);
}

#ifdef FAUDIO_DUMP_VOICES
FAudioIOStreamOut* FAudio_fopen_out(const char *path, const char *mode)
{
	FAudioIOStreamOut *io = (FAudioIOStreamOut*) FAudio_malloc(
		sizeof(FAudioIOStreamOut)
	);
	SDL_RWops *rwops = SDL_RWFromFile(path, mode);
	io->data = rwops;
	io->read = (FAudio_readfunc) rwops->read;
	io->write = (FAudio_writefunc) rwops->write;
	io->seek = (FAudio_seekfunc) rwops->seek;
	io->size = (FAudio_sizefunc) rwops->size;
	io->close = (FAudio_closefunc) rwops->close;
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

#endif /* FAUDIO_WIN32_PLATFORM */
