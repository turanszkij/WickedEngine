/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2020 Ethan Lee, Luigi Auriemma, and the MonoGame Team
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

#ifdef FAUDIO_WIN32_PLATFORM

#include "FAudio_internal.h"

#include <stddef.h>

#define COBJMACROS
#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfreadwrite.h>
#include <propvarutil.h>

#include <initguid.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <devpkey.h>

DEFINE_GUID(CLSID_CWMADecMediaObject, 0x2eeb4adf, 0x4578, 0x4d10, 0xbc, 0xa7, 0xbb, 0x95, 0x5f, 0x56, 0x32, 0x0a);

#ifdef _MSC_VER
DEFINE_GUID(IID_IAudioClient,         0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient,   0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_GUID(IID_IMMDeviceEnumerator,  0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
#endif

DEFINE_MEDIATYPE_GUID(MFAudioFormat_XMAudio2, FAUDIO_FORMAT_XMAUDIO2);

static CRITICAL_SECTION faudio_cs = { NULL, -1, 0, 0, 0, 0 };
static IMMDeviceEnumerator *device_enumerator;
static HRESULT init_hr;

struct FAudioWin32PlatformData
{
	IAudioClient *client;
	HANDLE audioThread;
	HANDLE stopEvent;
};

struct FAudioAudioClientThreadArgs
{
	WAVEFORMATEXTENSIBLE format;
	IAudioClient *client;
	HANDLE events[2];
	FAudio *audio;
	UINT updateSize;
};

void FAudio_Log(char const *msg)
{
	OutputDebugStringA(msg);
}

static HMODULE kernelbase = NULL;
static HRESULT (WINAPI *my_SetThreadDescription)(HANDLE, PCWSTR) = NULL;

static void FAudio_resolve_SetThreadDescription(void)
{
	kernelbase = LoadLibraryA("kernelbase.dll");
	if (!kernelbase)
		return;

	my_SetThreadDescription = (HRESULT (WINAPI *)(HANDLE, PCWSTR)) GetProcAddress(kernelbase, "SetThreadDescription");
	if (!my_SetThreadDescription)
	{
		FreeLibrary(kernelbase);
		kernelbase = NULL;
	}
}

static void FAudio_set_thread_name(char const *name)
{
	int ret;
	WCHAR *nameW;

	if (!my_SetThreadDescription)
		return;

	ret = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);

	nameW = FAudio_malloc(ret * sizeof(WCHAR));
	if (!nameW)
		return;

	ret = MultiByteToWideChar(CP_UTF8, 0, name, -1, nameW, ret);
	if (ret)
		my_SetThreadDescription(GetCurrentThread(), nameW);

	FAudio_free(nameW);
}

static HRESULT FAudio_FillAudioClientBuffer(
	struct FAudioAudioClientThreadArgs *args,
	IAudioRenderClient *client,
	UINT frames,
	UINT padding
) {
	HRESULT hr = S_OK;
	BYTE *buffer;

	while (padding + args->updateSize <= frames)
	{
		hr = IAudioRenderClient_GetBuffer(
			client,
			frames - padding,
			&buffer
		);
		if (FAILED(hr)) return hr;

		FAudio_zero(
			buffer,
			args->updateSize * args->format.Format.nBlockAlign
		);

		if (args->audio->active)
		{
			FAudio_INTERNAL_UpdateEngine(
				args->audio,
				(float*) buffer
			);
		}

		hr = IAudioRenderClient_ReleaseBuffer(
			client,
			args->updateSize,
			0
		);
		if (FAILED(hr)) return hr;

		padding += args->updateSize;
	}

	return hr;
}

static DWORD WINAPI FAudio_AudioClientThread(void *user)
{
	struct FAudioAudioClientThreadArgs *args = user;
	IAudioRenderClient *render_client;
	HRESULT hr = S_OK;
	UINT frames, padding = 0;

	FAudio_set_thread_name(__func__);

	hr = IAudioClient_GetService(
		args->client,
		&IID_IAudioRenderClient,
		(void **)&render_client
	);
	FAudio_assert(!FAILED(hr) && "Failed to get IAudioRenderClient service!");

	hr = IAudioClient_GetBufferSize(args->client, &frames);
	FAudio_assert(!FAILED(hr) && "Failed to get IAudioClient buffer size!");

	hr = FAudio_FillAudioClientBuffer(args, render_client, frames, 0);
	FAudio_assert(!FAILED(hr) && "Failed to initialize IAudioClient buffer!");

	hr = IAudioClient_Start(args->client);
	FAudio_assert(!FAILED(hr) && "Failed to start IAudioClient!");

	while (WaitForMultipleObjects(2, args->events, FALSE, INFINITE) == WAIT_OBJECT_0)
	{
		hr = IAudioClient_GetCurrentPadding(args->client, &padding);
		if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
		{
			/* Device was removed, just exit */
			break;
		}
		FAudio_assert(!FAILED(hr) && "Failed to get IAudioClient current padding!");

		hr = FAudio_FillAudioClientBuffer(args, render_client, frames, padding);
		FAudio_assert(!FAILED(hr) && "Failed to fill IAudioClient buffer!");
	}

	hr = IAudioClient_Stop(args->client);
	FAudio_assert(!FAILED(hr) && "Failed to stop IAudioClient!");

	IAudioRenderClient_Release(render_client);
	FAudio_free(args);
	return 0;
}

/* Sets `defaultDeviceIndex` to the default audio device index in
 * `deviceCollection`.
 * On failure, `defaultDeviceIndex` is not modified and the latest error is
 * returned. */
static HRESULT FAudio_DefaultDeviceIndex(
	IMMDeviceCollection *deviceCollection,
	uint32_t* defaultDeviceIndex
) {
	IMMDevice *device;
	HRESULT hr;
	uint32_t i, count;
	WCHAR *default_guid;
	WCHAR *device_guid; 

	/* Open the default device and get its GUID. */
	hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(
		device_enumerator,
		eRender,
		eConsole,
		&device
	);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = IMMDevice_GetId(device, &default_guid);
	if (FAILED(hr))
	{
		IMMDevice_Release(device);
		return hr;
	}

	/* Free the default device. */
	IMMDevice_Release(device);

	hr = IMMDeviceCollection_GetCount(deviceCollection, &count);
	if (FAILED(hr))
	{
		CoTaskMemFree(default_guid);
		return hr;
	}

	for (i = 0; i < count; i += 1)
	{
		/* Open the device and get its GUID. */
		hr = IMMDeviceCollection_Item(deviceCollection, i, &device);
		if (FAILED(hr)) {
			CoTaskMemFree(default_guid);
			return hr;
		}
		hr = IMMDevice_GetId(device, &device_guid);
		if (FAILED(hr))
		{
			CoTaskMemFree(default_guid);
			IMMDevice_Release(device);
			return hr;
		}

		if (lstrcmpW(default_guid, device_guid) == 0)
		{
			/* Device found. */
			CoTaskMemFree(default_guid);
			CoTaskMemFree(device_guid);
			IMMDevice_Release(device);
			*defaultDeviceIndex = i;
			return S_OK;
		}

		CoTaskMemFree(device_guid);
		IMMDevice_Release(device);
	}

	/* This should probably never happen. Just in case, set
	 * `defaultDeviceIndex` to 0 and return S_OK. */
	CoTaskMemFree(default_guid);
	*defaultDeviceIndex = 0;
	return S_OK;
}

/* Open `device`, corresponding to `deviceIndex`. `deviceIndex` 0 always
 * corresponds to the default device. XAudio reorders the devices so that the
 * default device is always at index 0, so we mimick this behavior here by
 * swapping the devices at indexes 0 and `defaultDeviceIndex`.
 */
static HRESULT FAudio_OpenDevice(uint32_t deviceIndex, IMMDevice **device)
{
	IMMDeviceCollection *deviceCollection;
	HRESULT hr;
	uint32_t defaultDeviceIndex;
	uint32_t actualIndex;

	*device = NULL;

	hr = IMMDeviceEnumerator_EnumAudioEndpoints(
		device_enumerator,
		eRender,
		DEVICE_STATE_ACTIVE,
		&deviceCollection
	);
	if (FAILED(hr))
	{
		return hr;
	}

	/* Get the default device index. */
	hr = FAudio_DefaultDeviceIndex(deviceCollection, &defaultDeviceIndex);
	if (FAILED(hr))
	{
		IMMDeviceCollection_Release(deviceCollection);
		return hr;
	}

	if (deviceIndex == 0) {
		/* Default device. */
		actualIndex = defaultDeviceIndex;
	} else if (deviceIndex == defaultDeviceIndex) {
		/* Open the device at index 0 instead of the "correct" one. */
		actualIndex = 0;
	} else {
		/* Otherwise, just open the device. */
		actualIndex = deviceIndex;
	
	}
	hr = IMMDeviceCollection_Item(deviceCollection, actualIndex, device);
	if (FAILED(hr))
	{
		IMMDeviceCollection_Release(deviceCollection);
		return hr;
	}

	IMMDeviceCollection_Release(deviceCollection);

	return hr;
}

void FAudio_PlatformInit(
	FAudio *audio,
	uint32_t flags,
	uint32_t deviceIndex,
	FAudioWaveFormatExtensible *mixFormat,
	uint32_t *updateSize,
	void** platformDevice
) {
	struct FAudioAudioClientThreadArgs *args;
	struct FAudioWin32PlatformData *data;
	REFERENCE_TIME duration;
	WAVEFORMATEX *closest;
	IMMDevice *device = NULL;
	HRESULT hr;
	HANDLE audioEvent = NULL;
	BOOL has_sse2 = IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64ec__) || defined(_M_ARM64EC)
	BOOL has_neon = TRUE;
#elif defined(__arm__) || defined(_M_ARM)
	BOOL has_neon = IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE);
#else
	BOOL has_neon = FALSE;
#endif
	FAudio_INTERNAL_InitSIMDFunctions(has_sse2, has_neon);
	FAudio_resolve_SetThreadDescription();

	FAudio_PlatformAddRef();

	*platformDevice = NULL;

	args = FAudio_malloc(sizeof(*args));
	FAudio_assert(!!args && "Failed to allocate FAudio thread args!");

	data = FAudio_malloc(sizeof(*data));
	FAudio_assert(!!data && "Failed to allocate FAudio platform data!");
	FAudio_zero(data, sizeof(*data));

	args->format.Format.wFormatTag = mixFormat->Format.wFormatTag;
	args->format.Format.nChannels = mixFormat->Format.nChannels;
	args->format.Format.nSamplesPerSec = mixFormat->Format.nSamplesPerSec;
	args->format.Format.nAvgBytesPerSec = mixFormat->Format.nAvgBytesPerSec;
	args->format.Format.nBlockAlign = mixFormat->Format.nBlockAlign;
	args->format.Format.wBitsPerSample = mixFormat->Format.wBitsPerSample;
	args->format.Format.cbSize = mixFormat->Format.cbSize;

	if (args->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		args->format.Samples.wValidBitsPerSample = mixFormat->Samples.wValidBitsPerSample;
		args->format.dwChannelMask = mixFormat->dwChannelMask;
		FAudio_memcpy(
			&args->format.SubFormat,
			&mixFormat->SubFormat,
			sizeof(GUID)
		);
	}

	audioEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	FAudio_assert(!!audioEvent && "Failed to create FAudio thread buffer event!");

	data->stopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	FAudio_assert(!!data->stopEvent && "Failed to create FAudio thread stop event!");

	hr = FAudio_OpenDevice(deviceIndex, &device);
	FAudio_assert(!FAILED(hr) && "Failed to get audio device!");

	hr = IMMDevice_Activate(
		device,
		&IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(void **)&data->client
	);
	FAudio_assert(!FAILED(hr) && "Failed to create audio client!");
	IMMDevice_Release(device);

	if (flags & FAUDIO_1024_QUANTUM) duration = 213333;
	else duration = 100000;

	hr = IAudioClient_IsFormatSupported(
		data->client,
		AUDCLNT_SHAREMODE_SHARED,
		&args->format.Format,
		&closest
	);
	FAudio_assert(!FAILED(hr) && "Failed to find supported audio format!");

	if (closest)
	{
		if (closest->wFormatTag != WAVE_FORMAT_EXTENSIBLE) args->format.Format = *closest;
		else args->format = *(WAVEFORMATEXTENSIBLE *)closest;
		CoTaskMemFree(closest);
	}

	hr = IAudioClient_Initialize(
		data->client,
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		duration * 3,
		0,
		&args->format.Format,
		&GUID_NULL
	);
	FAudio_assert(!FAILED(hr) && "Failed to initialize audio client!");

	hr = IAudioClient_SetEventHandle(data->client, audioEvent);
	FAudio_assert(!FAILED(hr) && "Failed to set audio client event!");

	mixFormat->Format.wFormatTag = args->format.Format.wFormatTag;
	mixFormat->Format.nChannels = args->format.Format.nChannels;
	mixFormat->Format.nSamplesPerSec = args->format.Format.nSamplesPerSec;
	mixFormat->Format.nAvgBytesPerSec = args->format.Format.nAvgBytesPerSec;
	mixFormat->Format.nBlockAlign = args->format.Format.nBlockAlign;
	mixFormat->Format.wBitsPerSample = args->format.Format.wBitsPerSample;

	if (args->format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		mixFormat->Format.cbSize = sizeof(FAudioWaveFormatExtensible) - sizeof(FAudioWaveFormatEx);
		mixFormat->Samples.wValidBitsPerSample = args->format.Samples.wValidBitsPerSample;
		mixFormat->dwChannelMask = args->format.dwChannelMask;
		FAudio_memcpy(
			&mixFormat->SubFormat,
			&args->format.SubFormat,
			sizeof(GUID)
		);
	}
	else
	{
		mixFormat->Format.cbSize = sizeof(FAudioWaveFormatEx);
	}

	args->client = data->client;
	args->events[0] = audioEvent;
	args->events[1] = data->stopEvent;
	args->audio = audio;
	if (flags & FAUDIO_1024_QUANTUM) args->updateSize = args->format.Format.nSamplesPerSec / (1000.0 / (64.0 / 3.0));
	else args->updateSize = args->format.Format.nSamplesPerSec / 100;

	data->audioThread = CreateThread(NULL, 0, &FAudio_AudioClientThread, args, 0, NULL);
	FAudio_assert(!!data->audioThread && "Failed to create audio client thread!");

	*updateSize = args->updateSize;
	*platformDevice = data;
	return;
}

void FAudio_PlatformQuit(void* platformDevice)
{
	struct FAudioWin32PlatformData *data = platformDevice;

	SetEvent(data->stopEvent);
	WaitForSingleObject(data->audioThread, INFINITE);
	if (data->client) IAudioClient_Release(data->client);
	if (kernelbase)
	{
		my_SetThreadDescription = NULL;
		FreeLibrary(kernelbase);
		kernelbase = NULL;
	}
	FAudio_PlatformRelease();
}

void FAudio_PlatformAddRef()
{
	HRESULT hr;
	EnterCriticalSection(&faudio_cs);
	if (!device_enumerator)
	{
		init_hr = CoInitialize(NULL);
		hr = CoCreateInstance(
			&CLSID_MMDeviceEnumerator,
			NULL,
			CLSCTX_INPROC_SERVER,
			&IID_IMMDeviceEnumerator,
			(void**)&device_enumerator
		);
		FAudio_assert(!FAILED(hr) && "CoCreateInstance failed!");
	}
	else IMMDeviceEnumerator_AddRef(device_enumerator);
	LeaveCriticalSection(&faudio_cs);
}

void FAudio_PlatformRelease()
{
	EnterCriticalSection(&faudio_cs);
	if (!IMMDeviceEnumerator_Release(device_enumerator))
	{
		device_enumerator = NULL;
		if (SUCCEEDED(init_hr)) CoUninitialize();
	}
	LeaveCriticalSection(&faudio_cs);
}

uint32_t FAudio_PlatformGetDeviceCount(void)
{
	IMMDeviceCollection *device_collection;
	uint32_t count;
	HRESULT hr;

	FAudio_PlatformAddRef();

	hr = IMMDeviceEnumerator_EnumAudioEndpoints(
		device_enumerator,
		eRender,
		DEVICE_STATE_ACTIVE,
		&device_collection
	);
	if (FAILED(hr)) {
		FAudio_PlatformRelease();
		return 0;
	}

	hr = IMMDeviceCollection_GetCount(device_collection, &count);
	if (FAILED(hr)) {
		IMMDeviceCollection_Release(device_collection);
		FAudio_PlatformRelease();
		return 0;
	}

	IMMDeviceCollection_Release(device_collection);

	FAudio_PlatformRelease();

	return count;
}

uint32_t FAudio_PlatformGetDeviceDetails(
	uint32_t index,
	FAudioDeviceDetails *details
) {
	WAVEFORMATEX *format, *obtained;
	WAVEFORMATEXTENSIBLE *ext;
	IAudioClient *client;
	IMMDevice *device;
	IPropertyStore* properties;
	PROPVARIANT deviceName;
	uint32_t count = 0;
	uint32_t ret = 0;
	HRESULT hr;
	WCHAR *str;
	GUID sub;

	FAudio_memset(details, 0, sizeof(FAudioDeviceDetails));

	FAudio_PlatformAddRef();

	count = FAudio_PlatformGetDeviceCount();
	if (index >= count)
	{
		FAudio_PlatformRelease();
		return FAUDIO_E_INVALID_CALL;
	}

	hr = FAudio_OpenDevice(index, &device);
	FAudio_assert(!FAILED(hr) && "Failed to get audio endpoint!");

	if (index == 0)
	{
		details->Role = FAudioGlobalDefaultDevice;
	}
	else
	{
		details->Role = FAudioNotDefaultDevice;
	}

	/* Set the Device Display Name */
	hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &properties);
	FAudio_assert(!FAILED(hr) && "Failed to open device property store!");
	hr = IPropertyStore_GetValue(properties, (PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &deviceName);
	FAudio_assert(!FAILED(hr) && "Failed to get audio device friendly name!");
	lstrcpynW((LPWSTR)details->DisplayName, deviceName.pwszVal, ARRAYSIZE(details->DisplayName) - 1);
	PropVariantClear(&deviceName);
	IPropertyStore_Release(properties);

	/* Set the Device ID */
	hr = IMMDevice_GetId(device, &str);
	FAudio_assert(!FAILED(hr) && "Failed to get audio endpoint id!");
	lstrcpynW((LPWSTR)details->DeviceID, str, ARRAYSIZE(details->DeviceID) - 1);
	CoTaskMemFree(str);

	hr = IMMDevice_Activate(
		device,
		&IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(void **)&client
	);
	FAudio_assert(!FAILED(hr) && "Failed to activate audio client!");

	hr = IAudioClient_GetMixFormat(client, &format);
	FAudio_assert(!FAILED(hr) && "Failed to get audio client mix format!");

	if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		ext = (WAVEFORMATEXTENSIBLE *)format;
		sub = ext->SubFormat;
		FAudio_memcpy(
			&ext->SubFormat,
			&DATAFORMAT_SUBTYPE_PCM,
			sizeof(GUID)
		);

		hr = IAudioClient_IsFormatSupported(client, AUDCLNT_SHAREMODE_SHARED, format, &obtained);
		if (FAILED(hr))
		{
			ext->SubFormat = sub;
		}
		else if (obtained)
		{
			CoTaskMemFree(format);
			format = obtained;
		}
	}

	details->OutputFormat.Format.wFormatTag = format->wFormatTag;
	details->OutputFormat.Format.nChannels = format->nChannels;
	details->OutputFormat.Format.nSamplesPerSec = format->nSamplesPerSec;
	details->OutputFormat.Format.nAvgBytesPerSec = format->nAvgBytesPerSec;
	details->OutputFormat.Format.nBlockAlign = format->nBlockAlign;
	details->OutputFormat.Format.wBitsPerSample = format->wBitsPerSample;
	details->OutputFormat.Format.cbSize = format->cbSize;

	if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		ext = (WAVEFORMATEXTENSIBLE *)format;
		details->OutputFormat.Samples.wValidBitsPerSample = ext->Samples.wValidBitsPerSample;
		details->OutputFormat.dwChannelMask = ext->dwChannelMask;
		FAudio_memcpy(
			&details->OutputFormat.SubFormat,
			&ext->SubFormat,
			sizeof(GUID)
		);
	}
	else
	{
		details->OutputFormat.dwChannelMask = GetMask(format->nChannels);
	}

	CoTaskMemFree(format);

	IAudioClient_Release(client);

	IMMDevice_Release(device);

	FAudio_PlatformRelease();

	return ret;
}

FAudioMutex FAudio_PlatformCreateMutex(void)
{
	CRITICAL_SECTION *cs;

	cs = FAudio_malloc(sizeof(CRITICAL_SECTION));
	if (!cs) return NULL;

	InitializeCriticalSection(cs);

	return cs;
}

void FAudio_PlatformLockMutex(FAudioMutex mutex)
{
	if (mutex) EnterCriticalSection(mutex);
}

void FAudio_PlatformUnlockMutex(FAudioMutex mutex)
{
	if (mutex) LeaveCriticalSection(mutex);
}

void FAudio_PlatformDestroyMutex(FAudioMutex mutex)
{
	if (mutex) DeleteCriticalSection(mutex);
	FAudio_free(mutex);
}

struct FAudioThreadArgs
{
	FAudioThreadFunc func;
	const char *name;
	void* data;
};

static DWORD WINAPI FaudioThreadWrapper(void *user)
{
	struct FAudioThreadArgs *args = user;
	DWORD ret;

	FAudio_set_thread_name(args->name);
	ret = args->func(args->data);

	FAudio_free(args);
	return ret;
}

FAudioThread FAudio_PlatformCreateThread(
	FAudioThreadFunc func,
	const char *name,
	void* data
) {
	struct FAudioThreadArgs *args;

	if (!(args = FAudio_malloc(sizeof(*args)))) return NULL;
	args->func = func;
	args->name = name;
	args->data = data;

	return CreateThread(NULL, 0, &FaudioThreadWrapper, args, 0, NULL);
}

void FAudio_PlatformWaitThread(FAudioThread thread, int32_t *retval)
{
	WaitForSingleObject(thread, INFINITE);
	if (retval != NULL) GetExitCodeThread(thread, (DWORD *)retval);
}

void FAudio_PlatformThreadPriority(FAudioThreadPriority priority)
{
	/* FIXME */
}

uint64_t FAudio_PlatformGetThreadID(void)
{
	return GetCurrentThreadId();
}

void FAudio_sleep(uint32_t ms)
{
	Sleep(ms);
}

uint32_t FAudio_timems()
{
	return GetTickCount();
}

/* FAudio I/O */

static size_t FAUDIOCALL FAudio_FILE_read(
	void *data,
	void *dst,
	size_t size,
	size_t count
) {
	if (!data) return 0;
	return fread(dst, size, count, data);
}

static int64_t FAUDIOCALL FAudio_FILE_seek(
	void *data,
	int64_t offset,
	int whence
) {
	if (!data) return -1;
	fseek(data, offset, whence);
	return ftell(data);
}

static int FAUDIOCALL FAudio_FILE_close(void *data)
{
	if (!data) return 0;
	fclose(data);
	return 0;
}

FAudioIOStream* FAudio_fopen(const char *path)
{
	FAudioIOStream *io;

	io = (FAudioIOStream*) FAudio_malloc(sizeof(FAudioIOStream));
	if (!io) return NULL;

	io->data = fopen(path, "rb");
	io->read = FAudio_FILE_read;
	io->seek = FAudio_FILE_seek;
	io->close = FAudio_FILE_close;
	io->lock = FAudio_PlatformCreateMutex();

	return io;
}

struct FAudio_mem
{
	char *mem;
	int64_t len;
	int64_t pos;
};

static size_t FAUDIOCALL FAudio_mem_read(
	void *data,
	void *dst,
	size_t size,
	size_t count
) {
	struct FAudio_mem *io = data;
	size_t len = size * count;

	if (!data) return 0;

	while (len && len > (io->len - io->pos)) len -= size;
	FAudio_memcpy(dst, io->mem + io->pos, len);
	io->pos += len;

	return len;
}

static int64_t FAUDIOCALL FAudio_mem_seek(
	void *data,
	int64_t offset,
	int whence
) {
	struct FAudio_mem *io = data;
	if (!data) return -1;

	if (whence == SEEK_SET)
	{
		if (io->len > offset) io->pos = offset;
		else io->pos = io->len;
	}
	if (whence == SEEK_CUR)
	{
		if (io->len > io->pos + offset) io->pos += offset;
		else io->pos = io->len;
	}
	if (whence == SEEK_END)
	{
		if (io->len > offset) io->pos = io->len - offset;
		else io->pos = 0;
	}

	return io->pos;
}

static int FAUDIOCALL FAudio_mem_close(void *data)
{
	if (!data) return 0;
	FAudio_free(data);
	return 0;
}

FAudioIOStream* FAudio_memopen(void *mem, int len)
{
	struct FAudio_mem *data;
	FAudioIOStream *io;

	io = (FAudioIOStream*) FAudio_malloc(sizeof(FAudioIOStream));
	if (!io) return NULL;

	data = FAudio_malloc(sizeof(struct FAudio_mem));
	if (!data)
	{
		FAudio_free(io);
		return NULL;
	}

	data->mem = mem;
	data->len = len;
	data->pos = 0;

	io->data = data;
	io->read = FAudio_mem_read;
	io->seek = FAudio_mem_seek;
	io->close = FAudio_mem_close;
	io->lock = FAudio_PlatformCreateMutex();
	return io;
}

uint8_t* FAudio_memptr(FAudioIOStream *io, size_t offset)
{
	struct FAudio_mem *memio = io->data;
	return memio->mem + offset;
}

void FAudio_close(FAudioIOStream *io)
{
	io->close(io->data);
	FAudio_PlatformDestroyMutex((FAudioMutex) io->lock);
	FAudio_free(io);
}

/* XNA Song implementation over Win32 MF */

static FAudioWaveFormatEx activeSongFormat;
IMFSourceReader *activeSong;
static uint8_t *songBuffer;
static SIZE_T songBufferSize;

static float songVolume = 1.0f;
static FAudio *songAudio = NULL;
static FAudioMasteringVoice *songMaster = NULL;

static FAudioSourceVoice *songVoice = NULL;
static FAudioVoiceCallback callbacks;

/* Internal Functions */

static void XNA_SongSubmitBuffer(FAudioVoiceCallback *callback, void *pBufferContext)
{
	IMFMediaBuffer *media_buffer;
	FAudioBuffer buffer;
	IMFSample *sample;
	HRESULT hr;
	DWORD flags, buffer_size = 0;
	BYTE *buffer_ptr;

	LOG_FUNC_ENTER(songAudio);

	FAudio_memset(&buffer, 0, sizeof(buffer));

	hr = IMFSourceReader_ReadSample(
		activeSong,
		MF_SOURCE_READER_FIRST_AUDIO_STREAM,
		0,
		NULL,
		&flags,
		NULL,
		&sample
	);
	FAudio_assert(!FAILED(hr) && "Failed to read audio sample!");

	if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
	{
		buffer.Flags = FAUDIO_END_OF_STREAM;
	}
	else
	{
		hr = IMFSample_ConvertToContiguousBuffer(
			sample,
			&media_buffer
		);
		FAudio_assert(!FAILED(hr) && "Failed to get sample buffer!");

		hr = IMFMediaBuffer_Lock(
			media_buffer,
			&buffer_ptr,
			NULL,
			&buffer_size
		);
		FAudio_assert(!FAILED(hr) && "Failed to lock buffer bytes!");

		if (songBufferSize < buffer_size)
		{
			songBufferSize = buffer_size;
			songBuffer = FAudio_realloc(songBuffer, songBufferSize);
			FAudio_assert(songBuffer != NULL && "Failed to allocate song buffer!");
		}
		FAudio_memcpy(songBuffer, buffer_ptr, buffer_size);

		hr = IMFMediaBuffer_Unlock(media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to unlock buffer bytes!");

		IMFMediaBuffer_Release(media_buffer);
		IMFSample_Release(sample);
	}

	if (buffer_size > 0)
	{
		buffer.AudioBytes = buffer_size;
		buffer.pAudioData = songBuffer;
		buffer.PlayBegin = 0;
		buffer.PlayLength = buffer_size / activeSongFormat.nBlockAlign;
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

	LOG_FUNC_EXIT(songAudio);
}

static void XNA_SongKill()
{
	if (songVoice != NULL)
	{
		FAudioSourceVoice_Stop(songVoice, 0, 0);
		FAudioVoice_DestroyVoice(songVoice);
		songVoice = NULL;
	}
	if (activeSong)
	{
		IMFSourceReader_Release(activeSong);
		activeSong = NULL;
	}
	FAudio_free(songBuffer);
	songBuffer = NULL;
	songBufferSize = 0;
}

/* "Public" API */

FAUDIOAPI void XNA_SongInit()
{
	HRESULT hr;

	hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
	FAudio_assert(!FAILED(hr) && "Failed to initialize Media Foundation!");

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
        MFShutdown();
}

FAUDIOAPI float XNA_PlaySong(const char *name)
{
	IMFAttributes *attributes = NULL;
	IMFMediaType *media_type = NULL;
	UINT32 channels, samplerate;
	UINT64 duration;
	PROPVARIANT var;
	HRESULT hr;
	WCHAR filename_w[MAX_PATH];

	LOG_FUNC_ENTER(songAudio);
	LOG_INFO(songAudio, "name %s\n", name);
	XNA_SongKill();

	MultiByteToWideChar(CP_UTF8, 0, name, -1, filename_w, MAX_PATH);

	hr = MFCreateAttributes(&attributes, 1);
	FAudio_assert(!FAILED(hr) && "Failed to create attributes!");
	hr = MFCreateSourceReaderFromURL(
		filename_w,
		attributes,
		&activeSong
	);
	FAudio_assert(!FAILED(hr) && "Failed to create source reader!");
	IMFAttributes_Release(attributes);

	hr = MFCreateMediaType(&media_type);
	FAudio_assert(!FAILED(hr) && "Failed to create media type!");
	hr = IMFMediaType_SetGUID(
		media_type,
		&MF_MT_MAJOR_TYPE,
		&MFMediaType_Audio
	);
	FAudio_assert(!FAILED(hr) && "Failed to set major type!");
	hr = IMFMediaType_SetGUID(
		media_type,
		&MF_MT_SUBTYPE,
		&MFAudioFormat_Float
	);
	FAudio_assert(!FAILED(hr) && "Failed to set sub type!");
	hr = IMFSourceReader_SetCurrentMediaType(
		activeSong,
		MF_SOURCE_READER_FIRST_AUDIO_STREAM,
		NULL,
		media_type
	);
	FAudio_assert(!FAILED(hr) && "Failed to set source media type!");
	hr = IMFSourceReader_SetStreamSelection(
		activeSong,
		MF_SOURCE_READER_FIRST_AUDIO_STREAM,
		TRUE
	);
	FAudio_assert(!FAILED(hr) && "Failed to select source stream!");
	IMFMediaType_Release(media_type);

	hr = IMFSourceReader_GetCurrentMediaType(
		activeSong,
		MF_SOURCE_READER_FIRST_AUDIO_STREAM,
		&media_type
	);
	FAudio_assert(!FAILED(hr) && "Failed to get current media type!");
	hr = IMFMediaType_GetUINT32(
		media_type,
		&MF_MT_AUDIO_NUM_CHANNELS,
		&channels
	);
	FAudio_assert(!FAILED(hr) && "Failed to get channel count!");
	hr = IMFMediaType_GetUINT32(
		media_type,
		&MF_MT_AUDIO_SAMPLES_PER_SECOND,
		&samplerate
	);
	FAudio_assert(!FAILED(hr) && "Failed to get sample rate!");
	IMFMediaType_Release(media_type);

	hr = IMFSourceReader_GetPresentationAttribute(
		activeSong,
		MF_SOURCE_READER_MEDIASOURCE,
		&MF_PD_DURATION,
		&var
	);
	FAudio_assert(!FAILED(hr) && "Failed to get song duration!");
        hr = PropVariantToInt64(&var, &duration);
	FAudio_assert(!FAILED(hr) && "Failed to get song duration!");
        PropVariantClear(&var);

	activeSongFormat.wFormatTag = FAUDIO_FORMAT_IEEE_FLOAT;
	activeSongFormat.nChannels = channels;
	activeSongFormat.nSamplesPerSec = samplerate;
	activeSongFormat.wBitsPerSample = sizeof(float) * 8;
	activeSongFormat.nBlockAlign = activeSongFormat.nChannels * activeSongFormat.wBitsPerSample / 8;
	activeSongFormat.nAvgBytesPerSec = activeSongFormat.nSamplesPerSec * activeSongFormat.nBlockAlign;
	activeSongFormat.cbSize = 0;

	/* Init voice */
	FAudio_zero(&callbacks, sizeof(FAudioVoiceCallback));
	callbacks.OnBufferEnd = XNA_SongSubmitBuffer;
	FAudio_CreateSourceVoice(
		songAudio,
		&songVoice,
		&activeSongFormat,
		0,
		1.0f, /* No pitch shifting here! */
		&callbacks,
		NULL,
		NULL
	);
	FAudioVoice_SetVolume(songVoice, songVolume, 0);
	XNA_SongSubmitBuffer(NULL, NULL);

	/* Finally. */
	FAudioSourceVoice_Start(songVoice, 0, 0);
	LOG_FUNC_EXIT(songAudio);
	return duration / 10000000.;
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
	if (songVoice == NULL || activeSong == NULL)
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

/* FAudio WMADEC implementation over Win32 MF */

struct FAudioWMADEC
{
	IMFTransform *decoder;
	IMFSample *output_sample;

	char *output_buf;
	size_t output_pos;
	size_t output_size;
	size_t input_pos;
	size_t input_size;
};

static HRESULT FAudio_WMAMF_ProcessInput(
	FAudioVoice *voice,
	FAudioBuffer *buffer
) {
	struct FAudioWMADEC *impl = voice->src.wmadec;
	IMFMediaBuffer *media_buffer;
	IMFSample *sample;
	DWORD copy_size;
	BYTE *copy_buf;
	HRESULT hr;

	copy_size = min(buffer->AudioBytes - impl->input_pos, impl->input_size);
	if (!copy_size) return S_FALSE;
	LOG_INFO(voice->audio, "pushing %x bytes at %x", copy_size, impl->input_pos);

	hr = MFCreateSample(&sample);
	FAudio_assert(!FAILED(hr) && "Failed to create sample!");
	hr = MFCreateMemoryBuffer(copy_size, &media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to create buffer!");
	hr = IMFMediaBuffer_SetCurrentLength(media_buffer, copy_size);
	FAudio_assert(!FAILED(hr) && "Failed to set buffer length!");
	hr = IMFMediaBuffer_Lock(
		media_buffer,
		&copy_buf,
		NULL,
		&copy_size
	);
	FAudio_assert(!FAILED(hr) && "Failed to lock buffer bytes!");
	FAudio_memcpy(copy_buf, buffer->pAudioData + impl->input_pos, copy_size);
	hr = IMFMediaBuffer_Unlock(media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to unlock buffer bytes!");

	hr = IMFSample_AddBuffer(sample, media_buffer);
	FAudio_assert(!FAILED(hr) && "Failed to buffer to sample!");
	IMFMediaBuffer_Release(media_buffer);

	hr = IMFTransform_ProcessInput(impl->decoder, 0, sample, 0);
	IMFSample_Release(sample);
	if (hr == MF_E_NOTACCEPTING) return S_OK;
	if (FAILED(hr))
	{
		LOG_ERROR(voice->audio, "IMFTransform_ProcessInput returned %#x", hr);
		return hr;
	}

	impl->input_pos += copy_size;
	return S_OK;
};

static HRESULT FAudio_WMAMF_ProcessOutput(
	FAudioVoice *voice,
	FAudioBuffer *buffer
) {
	struct FAudioWMADEC *impl = voice->src.wmadec;
	MFT_OUTPUT_DATA_BUFFER output;
	IMFMediaBuffer *media_buffer;
	DWORD status, copy_size;
	BYTE *copy_buf;
	HRESULT hr;

	while (1)
	{
		FAudio_memset(&output, 0, sizeof(output));
		output.pSample = impl->output_sample;
		hr = IMFTransform_ProcessOutput(impl->decoder, 0, 1, &output, &status);
		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) return S_FALSE;
		if (FAILED(hr))
		{
			LOG_ERROR(voice->audio, "IMFTransform_ProcessInput returned %#x", hr);
			return hr;
		}

		if (output.dwStatus & MFT_OUTPUT_DATA_BUFFER_NO_SAMPLE) continue;

		hr = IMFSample_ConvertToContiguousBuffer(
			output.pSample,
			&media_buffer
		);
		FAudio_assert(!FAILED(hr) && "Failed to get sample buffer!");
		hr = IMFMediaBuffer_Lock(
			media_buffer,
			&copy_buf,
			NULL,
			&copy_size
		);
		FAudio_assert(!FAILED(hr) && "Failed to lock buffer bytes!");
		if (impl->output_pos + copy_size > impl->output_size)
		{
			impl->output_size = max(
				impl->output_pos + copy_size,
				impl->output_size * 3 / 2
			);
			impl->output_buf = voice->audio->pRealloc(
				impl->output_buf,
				impl->output_size
			);
			FAudio_assert(impl->output_buf && "Failed to resize output buffer!");
		}
		FAudio_memcpy(impl->output_buf + impl->output_pos, copy_buf, copy_size);
		impl->output_pos += copy_size;
		LOG_INFO(voice->audio, "pulled %x bytes at %x", copy_size, impl->output_pos);
		hr = IMFMediaBuffer_Unlock(media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to unlock buffer bytes!");

		IMFMediaBuffer_Release(media_buffer);
		if (!impl->output_sample) IMFSample_Release(output.pSample);
	}

	return S_OK;
};

static void FAudio_INTERNAL_DecodeWMAMF(
	FAudioVoice *voice,
	FAudioBuffer *buffer,
	float *decodeCache,
	uint32_t samples
) {
	const FAudioWaveFormatExtensible *wfx = (FAudioWaveFormatExtensible *)voice->src.format;
	size_t samples_pos, samples_size, copy_size = 0;
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)

	if (!impl->output_pos)
	{
		if (wfx->Format.wFormatTag == FAUDIO_FORMAT_EXTENSIBLE)
		{
			const FAudioBufferWMA *wma = &voice->src.bufferList->bufferWMA;
			const UINT32 *output_sizes = wma->pDecodedPacketCumulativeBytes;

			impl->input_size = wfx->Format.nBlockAlign;
			impl->output_size = max(
				impl->output_size,
				output_sizes[wma->PacketCount - 1]
			);
		}
		else
		{
			const FAudioXMA2WaveFormat *xwf = (const FAudioXMA2WaveFormat *)wfx;

			impl->input_size = xwf->dwBytesPerBlock;
			impl->output_size = max(
				impl->output_size,
				(size_t) xwf->dwSamplesEncoded *
				voice->src.format->nChannels *
				(voice->src.format->wBitsPerSample / 8)
			);
		}

		impl->output_buf = voice->audio->pRealloc(
			impl->output_buf,
			impl->output_size
		);
		FAudio_assert(impl->output_buf && "Failed to allocate output buffer!");

		LOG_INFO(voice->audio, "sending BOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_START_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to notify decoder stream start!");
		FAudio_WMAMF_ProcessInput(voice, buffer);
	}

	samples_pos = voice->src.curBufferOffset * voice->src.format->nChannels * sizeof(float);
	samples_size = samples * voice->src.format->nChannels * sizeof(float);

	while (impl->output_pos < samples_pos + samples_size)
	{
		hr = FAudio_WMAMF_ProcessOutput(voice, buffer);
		if (FAILED(hr)) goto error;
		if (hr == S_OK) continue;

		hr  = FAudio_WMAMF_ProcessInput(voice, buffer);
		if (FAILED(hr)) goto error;
		if (hr == S_OK) continue;

		if (!impl->input_size) break;

		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}

	if (impl->output_pos > samples_pos)
	{
		copy_size = FAudio_min(impl->output_pos - samples_pos, samples_size);
		FAudio_memcpy(decodeCache, impl->output_buf + samples_pos, copy_size);
	}
	FAudio_zero(decodeCache + copy_size, samples_size - copy_size);
	LOG_INFO(
		voice->audio,
		"decoded %x / %x bytes, copied %x / %x bytes",
		impl->output_pos,
		impl->output_size,
		copy_size,
		samples_size
	);

	LOG_FUNC_EXIT(voice->audio)
	return;

error:
	FAudio_zero(decodeCache, samples * voice->src.format->nChannels * sizeof(float));
	LOG_FUNC_EXIT(voice->audio)
}

uint32_t FAudio_WMADEC_init(FAudioSourceVoice *voice, uint32_t type)
{
	static const uint8_t fake_codec_data[16] = {0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t fake_codec_data_wma3[18] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 224, 0, 0, 0};
	const FAudioWaveFormatExtensible *wfx = (FAudioWaveFormatExtensible *)voice->src.format;
	struct FAudioWMADEC *impl;
	MFT_OUTPUT_STREAM_INFO info = {0};
	IMFMediaBuffer *media_buffer;
	IMFMediaType *media_type;
	IMFTransform *decoder;
	HRESULT hr;
	UINT32 i, value;
	GUID guid;

	LOG_FUNC_ENTER(voice->audio)

	if (!(impl = voice->audio->pMalloc(sizeof(*impl)))) return -1;
	FAudio_memset(impl, 0, sizeof(*impl));

	hr = CoCreateInstance(
		&CLSID_CWMADecMediaObject,
		0,
		CLSCTX_INPROC_SERVER,
		&IID_IMFTransform,
		(void **)&decoder
	);
	if (FAILED(hr))
	{
		voice->audio->pFree(impl->output_buf);
		return -2;
	}

	hr = MFCreateMediaType(&media_type);
	FAudio_assert(!FAILED(hr) && "Failed create media type!");
	hr = IMFMediaType_SetGUID(
		media_type,
		&MF_MT_MAJOR_TYPE,
		&MFMediaType_Audio
	);
	FAudio_assert(!FAILED(hr) && "Failed set media major type!");

	switch (type)
	{
	case FAUDIO_FORMAT_WMAUDIO2:
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)fake_codec_data,
			sizeof(fake_codec_data)
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudioV8
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_WMAUDIO3:
                *(uint16_t *)fake_codec_data_wma3  = voice->src.format->wBitsPerSample;
                for (i = 0; i < voice->src.format->nChannels; i++)
                {
                    fake_codec_data_wma3[2] <<= 1;
                    fake_codec_data_wma3[2] |= 1;
                }
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)fake_codec_data_wma3,
			sizeof(fake_codec_data_wma3)
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudioV9
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_WMAUDIO_LOSSLESS:
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)&wfx->Samples,
			wfx->Format.cbSize
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_WMAudio_Lossless
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			wfx->Format.nBlockAlign
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	case FAUDIO_FORMAT_XMAUDIO2:
	{
		const FAudioXMA2WaveFormat *xwf = (const FAudioXMA2WaveFormat *)wfx;
		hr = IMFMediaType_SetBlob(
			media_type,
			&MF_MT_USER_DATA,
			(void *)&wfx->Samples,
			wfx->Format.cbSize
		);
		FAudio_assert(!FAILED(hr) && "Failed set codec private data!");
		hr = IMFMediaType_SetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&MFAudioFormat_XMAudio2
		);
		FAudio_assert(!FAILED(hr) && "Failed set media sub type!");
		hr = IMFMediaType_SetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			xwf->dwBytesPerBlock
		);
		FAudio_assert(!FAILED(hr) && "Failed set input block align!");
		break;
	}
	default:
		FAudio_assert(0 && "Unsupported type!");
		break;
	}

	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_BITS_PER_SAMPLE,
		wfx->Format.wBitsPerSample
	);
	FAudio_assert(!FAILED(hr) && "Failed set input bits per sample!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_AVG_BYTES_PER_SECOND,
		wfx->Format.nAvgBytesPerSec
	);
	FAudio_assert(!FAILED(hr) && "Failed set input bytes per sample!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_NUM_CHANNELS,
		wfx->Format.nChannels
	);
	FAudio_assert(!FAILED(hr) && "Failed set input channel count!");
	hr = IMFMediaType_SetUINT32(
		media_type,
		&MF_MT_AUDIO_SAMPLES_PER_SECOND,
		wfx->Format.nSamplesPerSec
	);
	FAudio_assert(!FAILED(hr) && "Failed set input sample rate!");

	hr = IMFTransform_SetInputType(
		decoder,
		0,
		media_type,
		0
	);
	FAudio_assert(!FAILED(hr) && "Failed set decoder input type!");
	IMFMediaType_Release(media_type);

	i = 0;
	while (SUCCEEDED(hr))
	{
		hr = IMFTransform_GetOutputAvailableType(
			decoder,
			0,
			i++,
			&media_type
		);
		FAudio_assert(!FAILED(hr) && "Failed get output media type!");

		hr = IMFMediaType_GetGUID(
			media_type,
			&MF_MT_MAJOR_TYPE,
			&guid
		);
		FAudio_assert(!FAILED(hr) && "Failed get media major type!");
		if (!IsEqualGUID(&MFMediaType_Audio, &guid)) goto next;

		hr = IMFMediaType_GetGUID(
			media_type,
			&MF_MT_SUBTYPE,
			&guid
		);
		FAudio_assert(!FAILED(hr) && "Failed get media major type!");
		if (!IsEqualGUID(&MFAudioFormat_Float, &guid)) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_BITS_PER_SAMPLE,
			&value
		);
		if (FAILED(hr))
		{
			value = 32;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_BITS_PER_SAMPLE,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get bits per sample!");
		if (value != 32) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_NUM_CHANNELS,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nChannels;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_NUM_CHANNELS,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get channel count!");
		if (value != wfx->Format.nChannels) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_SAMPLES_PER_SECOND,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nSamplesPerSec;
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_SAMPLES_PER_SECOND,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get sample rate!");
		if (value != wfx->Format.nSamplesPerSec) goto next;

		hr = IMFMediaType_GetUINT32(
			media_type,
			&MF_MT_AUDIO_BLOCK_ALIGNMENT,
			&value
		);
		if (FAILED(hr))
		{
			value = wfx->Format.nChannels * sizeof(float);
			hr = IMFMediaType_SetUINT32(
				media_type,
				&MF_MT_AUDIO_BLOCK_ALIGNMENT,
				value
			);
		}
		FAudio_assert(!FAILED(hr) && "Failed get block align!");
		if (value == wfx->Format.nChannels * sizeof(float)) break;

next:
		IMFMediaType_Release(media_type);
	}
	FAudio_assert(!FAILED(hr) && "Failed to find output media type!");
	hr = IMFTransform_SetOutputType(decoder, 0, media_type, 0);
	FAudio_assert(!FAILED(hr) && "Failed set decoder output type!");
	IMFMediaType_Release(media_type);

	hr = IMFTransform_GetOutputStreamInfo(decoder, 0, &info);
	FAudio_assert(!FAILED(hr) && "Failed to get output stream info!");

	impl->decoder = decoder;
	if (!(info.dwFlags & MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
	{
		hr = MFCreateSample(&impl->output_sample);
		FAudio_assert(!FAILED(hr) && "Failed to create sample!");
		hr = MFCreateMemoryBuffer(info.cbSize, &media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to create buffer!");
		hr = IMFSample_AddBuffer(impl->output_sample, media_buffer);
		FAudio_assert(!FAILED(hr) && "Failed to buffer to sample!");
		IMFMediaBuffer_Release(media_buffer);
	}

	hr = IMFTransform_ProcessMessage(
		decoder,
		MFT_MESSAGE_NOTIFY_BEGIN_STREAMING,
		0
	);
	FAudio_assert(!FAILED(hr) && "Failed to start decoder stream!");

	voice->src.wmadec = impl;
	voice->src.decode = FAudio_INTERNAL_DecodeWMAMF;

	LOG_FUNC_EXIT(voice->audio);
	return 0;
}

void FAudio_WMADEC_free(FAudioSourceVoice *voice)
{
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)
	FAudio_PlatformLockMutex(voice->audio->sourceLock);
	LOG_MUTEX_LOCK(voice->audio, voice->audio->sourceLock)

	if (impl->input_size)
	{
		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}
	if (impl->output_pos)
	{
		LOG_INFO(voice->audio, "sending DRAIN to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_COMMAND_DRAIN,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send DRAIN!");
		impl->output_pos = 0;
	}

	if (impl->output_sample) IMFSample_Release(impl->output_sample);
	IMFTransform_Release(impl->decoder);
	voice->audio->pFree(impl->output_buf);
	voice->audio->pFree(voice->src.wmadec);
	voice->src.wmadec = NULL;
	voice->src.decode = NULL;

	FAudio_PlatformUnlockMutex(voice->audio->sourceLock);
	LOG_MUTEX_UNLOCK(voice->audio, voice->audio->sourceLock)
	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_WMADEC_end_buffer(FAudioSourceVoice *voice)
{
	struct FAudioWMADEC *impl = voice->src.wmadec;
	HRESULT hr;

	LOG_FUNC_ENTER(voice->audio)

	if (impl->input_size)
	{
		LOG_INFO(voice->audio, "sending EOS to %p", impl->decoder);
		hr = IMFTransform_ProcessMessage(
			impl->decoder,
			MFT_MESSAGE_NOTIFY_END_OF_STREAM,
			0
		);
		FAudio_assert(!FAILED(hr) && "Failed to send EOS!");
		impl->input_size = 0;
	}
	impl->output_pos = 0;
	impl->input_pos = 0;

	LOG_FUNC_EXIT(voice->audio)
}

#else

extern int this_tu_is_empty;

#endif /* FAUDIO_WIN32_PLATFORM */
