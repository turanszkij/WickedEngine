/* XAudio2 tests
 *
 * This tests behavior of Microsoft's XAudio2. Tests in this file should be
 * written to the XAudio2 API. FAudio_compat.h provides conversion from XAudio2
 * to FAudio to verify FAudio's behavior.
 *
 * Copyright (c) 2015-2018 Andrew Eikum for CodeWeavers
 * Copyright (c) 2018 Masanori Kakura
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
 */

#ifdef _WIN32
/* cross-compile for win32 executable */
#define COBJMACROS
#include "initguid.h"
#include "xaudio2.h"
#include "xaudio2fx.h"
#include "xapo.h"
#include "xapofx.h"
#else
/* native build against FAudio */
#include "FAudio.h"
#include "FAudioFX.h"
#include "FAPO.h"

#include "FAudio_compat.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

static void *FAtest_malloc(size_t len)
{
#ifdef _WIN32
    return HeapAlloc(GetProcessHeap(), 0, len);
#else
    return malloc(len);
#endif
}

static void FAtest_free(void *p)
{
#ifdef _WIN32
    HeapFree(GetProcessHeap(), 0, p);
#else
    free(p);
#endif
}

static void FAtest_sleep(uint64_t millis)
{
#ifdef _WIN32
    Sleep(millis);
#else
    usleep(millis * 1000);
#endif
}

#define XA2CALL_0(method) if(xaudio27) hr = IXAudio27_##method((IXAudio27*)xa); else hr = IXAudio2_##method(xa);
#define XA2CALL_0V(method) if(xaudio27) IXAudio27_##method((IXAudio27*)xa); else IXAudio2_##method(xa);
#define XA2CALL_V(method, ...) if(xaudio27) IXAudio27_##method((IXAudio27*)xa, __VA_ARGS__); else IXAudio2_##method(xa, __VA_ARGS__);
#define XA2CALL(method, ...) if(xaudio27) hr = IXAudio27_##method((IXAudio27*)xa, __VA_ARGS__); else hr = IXAudio2_##method(xa, __VA_ARGS__);

#ifdef _WIN32
static HRESULT (WINAPI *pXAudio2Create)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR) = NULL;
static HRESULT (WINAPI *pCreateAudioVolumeMeter)(IUnknown**) = NULL;
#endif

static int failure_count = 0;
static int success_count = 0;

static void ok_(const char *file, int line, BOOL success, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
#define ok(success, fmt, ...) ok_(__FILE__, __LINE__, success, fmt, ##__VA_ARGS__)
static void ok_(const char *file, int line, BOOL success, const char *fmt, ...)
{
    if(!success){
        va_list va;
        va_start(va, fmt);
        fprintf(stdout, "test failed (%s:%u): ", file, line);
        vfprintf(stdout, fmt, va);
        va_end(va);
        ++failure_count;
    }else
        ++success_count;
}

static int xaudio27;

#ifdef _WIN32
DWORD main_thread_id;
#else
pthread_t main_thread_id;
#endif

static int is_main_thread(void)
{
#ifdef _WIN32
    return GetCurrentThreadId() == main_thread_id;
#else
    return pthread_equal(pthread_self(), main_thread_id);
#endif
}

static void fill_buf(float *buf, WAVEFORMATEX *fmt, DWORD hz, DWORD len_frames)
{
#if 0
    /* make sound */
    DWORD offs, c;
    for(offs = 0; offs < len_frames; ++offs)
        for(c = 0; c < fmt->nChannels; ++c)
            buf[offs * fmt->nChannels + c] = sinf(offs * hz * 2 * M_PI / fmt->nSamplesPerSec);
#else
    /* silence */
    memset(buf, 0, sizeof(float) * len_frames * fmt->nChannels);
#endif
}

static struct _cb_state {
    BOOL start_called, end_called;
} ecb_state, src1_state, src2_state;

static int pass_state = 0;
static BOOL buffers_called = FALSE;

static void WINAPI ECB_OnProcessingPassStart(IXAudio2EngineCallback *This)
{
    ok(!xaudio27 || pass_state == 0, "Callbacks called out of order: %u\n", pass_state);
    ++pass_state;
}

static void WINAPI ECB_OnProcessingPassEnd(IXAudio2EngineCallback *This)
{
    ok(!xaudio27 || pass_state == (buffers_called ? 7 : 5), "Callbacks called out of order: %u\n", pass_state);
    pass_state = 0;
    buffers_called = FALSE;
}

static void WINAPI ECB_OnCriticalError(IXAudio2EngineCallback *This, HRESULT Error)
{
    ok(0, "Unexpected OnCriticalError\n");
}

#if _WIN32
static IXAudio2EngineCallbackVtbl ecb_vtbl = {
    ECB_OnProcessingPassStart,
    ECB_OnProcessingPassEnd,
    ECB_OnCriticalError
};

static IXAudio2EngineCallback ecb = { &ecb_vtbl };
#else
static FAudioEngineCallback ecb = {
    ECB_OnCriticalError,
    ECB_OnProcessingPassEnd,
    ECB_OnProcessingPassStart
};
#endif

struct simple_streaming_callback {
    IXAudio2VoiceCallback IXAudio2VoiceCallback_iface;
    IXAudio27SourceVoice *xa27src;
    IXAudio2SourceVoice *xa2src;
};

static struct simple_streaming_callback vcb1, vcb2;

static void WINAPI VCB_OnVoiceProcessingPassStart(IXAudio2VoiceCallback *iface,
        UINT32 BytesRequired)
{
    struct simple_streaming_callback *This = (struct simple_streaming_callback *)iface;
    XAUDIO2_VOICE_STATE state;

    if(xaudio27)
        IXAudio27SourceVoice_GetState(This->xa27src, &state);
    else
        IXAudio2SourceVoice_GetState(This->xa2src, &state, 0);

    /* Underrun allowance may vary, but 0.03 seconds buffered should be enough
     * to not require more bytes. */
    if (state.BuffersQueued == 0){
        ok(BytesRequired > 0, "No buffers queued, but no more bytes requested.\n");
    }else if(state.SamplesPlayed < 22050 - 3 * 441){
        ok(BytesRequired == 0, "Plenty of data buffered, but more bytes requested. Buffered: %"PRIu64" samples, requested: %u bytes\n",
                22050 - state.SamplesPlayed, BytesRequired);
    }else if(state.SamplesPlayed == 22050){
        ok(BytesRequired > 0, "End of buffer reached, but no more bytes requested.\n");
    }

    if(iface == &vcb1.IXAudio2VoiceCallback_iface){
        ok(!xaudio27 || pass_state == (buffers_called ? 4 : 3), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 1, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }
}

static void WINAPI VCB_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback *iface)
{
    if(iface == &vcb1.IXAudio2VoiceCallback_iface){
        ok(!xaudio27 || pass_state == (buffers_called ? 6 : 4), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == (buffers_called ? 3 : 2), "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }
}

static void WINAPI VCB_OnStreamEnd(IXAudio2VoiceCallback *iface)
{
    if(iface == &vcb1.IXAudio2VoiceCallback_iface){
        ok(0, "Unexpected OnStreamEnd\n");
    }else{
        ok(!xaudio27 || pass_state == 3, "Callbacks called out of order: %u\n", pass_state);
    }
}

static void WINAPI VCB_OnBufferStart(IXAudio2VoiceCallback *iface,
        void *pBufferContext)
{
    if(iface == &vcb1.IXAudio2VoiceCallback_iface){
        ok(!xaudio27 || pass_state == 5, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 2, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
        buffers_called = TRUE;
    }
}

static void WINAPI VCB_OnBufferEnd(IXAudio2VoiceCallback *iface,
        void *pBufferContext)
{
    if(iface == &vcb1.IXAudio2VoiceCallback_iface){
        ok(!xaudio27 || pass_state == 5, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
    }else{
        ok(!xaudio27 || pass_state == 2, "Callbacks called out of order: %u\n", pass_state);
        ++pass_state;
        buffers_called = TRUE;
    }
}

static void WINAPI VCB_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ok(0, "Unexpected OnLoopEnd\n");
}

static void WINAPI VCB_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
    ok(0, "Unexpected OnVoiceError\n");
}

#ifdef _WIN32
static IXAudio2VoiceCallbackVtbl vcb_vtbl = {
    VCB_OnVoiceProcessingPassStart,
    VCB_OnVoiceProcessingPassEnd,
    VCB_OnStreamEnd,
    VCB_OnBufferStart,
    VCB_OnBufferEnd,
    VCB_OnLoopEnd,
    VCB_OnVoiceError
};

static struct simple_streaming_callback vcb1 = { {&vcb_vtbl} };
static struct simple_streaming_callback vcb2 = { {&vcb_vtbl} };
#else
static struct simple_streaming_callback vcb1 = {
    {
        VCB_OnBufferEnd,
        VCB_OnBufferStart,
        VCB_OnLoopEnd,
        VCB_OnStreamEnd,
        VCB_OnVoiceError,
        VCB_OnVoiceProcessingPassEnd,
        VCB_OnVoiceProcessingPassStart
    }
};

static struct simple_streaming_callback vcb2 = {
    {
        VCB_OnBufferEnd,
        VCB_OnBufferStart,
        VCB_OnLoopEnd,
        VCB_OnStreamEnd,
        VCB_OnVoiceError,
        VCB_OnVoiceProcessingPassEnd,
        VCB_OnVoiceProcessingPassStart
    }
};
#endif

static void test_simple_streaming(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src, *src2;
#ifdef _WIN32
    IUnknown *vumeter;
#else
    FAPO *vumeter;
#endif
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf, buf2;
    XAUDIO2_VOICE_STATE state;
    XAUDIO2_EFFECT_DESCRIPTOR effect;
    XAUDIO2_EFFECT_CHAIN chain;
    XAUDIO2_PERFORMANCE_DATA perfdata;
    DWORD chmask;

    memset(&perfdata, 0, sizeof(perfdata));
    XA2CALL_V(GetPerformanceData, &perfdata);
    ok(perfdata.ActiveSourceVoiceCount == 0, "Got wrong ActiveSourceVoiceCount: %u\n",
            perfdata.ActiveSourceVoiceCount);
    ok(perfdata.TotalSourceVoiceCount == 0, "Got wrong TotalSourceVoiceCount: %u\n",
            perfdata.TotalSourceVoiceCount);
    ok(perfdata.CurrentLatencyInSamples == 0, "Expected zero latency before mastering voice creation, got %u\n", perfdata.CurrentLatencyInSamples);

    memset(&ecb_state, 0, sizeof(ecb_state));
    memset(&src1_state, 0, sizeof(src1_state));
    memset(&src2_state, 0, sizeof(src2_state));

    XA2CALL_0V(StopEngine);

    /* Tests show in native XA2.8, ECB is called from a mixer thread, but VCBs
     * may be called from other threads in any order. So we can't rely on any
     * sequencing between VCB calls.
     *
     * XA2.7 does all mixing from a single thread, so call sequence can be
     * tested. */
    XA2CALL(RegisterForCallbacks, &ecb);
    ok(hr == S_OK, "RegisterForCallbacks failed: %08x\n", hr);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    if(!xaudio27){
        chmask = 0xdeadbeef;
        IXAudio2MasteringVoice_GetChannelMask(master, &chmask);
        ok(chmask == (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT), "Got unexpected channel mask: 0x%x\n", chmask);
    }

    /* create first source voice */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb1.IXAudio2VoiceCallback_iface, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        vcb1.xa27src = (IXAudio27SourceVoice*)src;
        IXAudio27SourceVoice_GetVoiceDetails((IXAudio27SourceVoice*)src, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        vcb1.xa2src = src;
        IXAudio2SourceVoice_GetVoiceDetails(src, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = FAtest_malloc(buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    /* create second source voice */
    XA2CALL(CreateSourceVoice, &src2, &fmt, 0, 1.f, &vcb2.IXAudio2VoiceCallback_iface, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        vcb2.xa27src = (IXAudio27SourceVoice*)src2;
        IXAudio27SourceVoice_GetVoiceDetails((IXAudio27SourceVoice*)src2, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        vcb2.xa2src = src2;
        IXAudio2SourceVoice_GetVoiceDetails(src2, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }

    memset(&buf2, 0, sizeof(buf2));
    buf2.Flags = XAUDIO2_END_OF_STREAM;
    buf2.AudioBytes = 22050 * fmt.nBlockAlign;
    buf2.pAudioData = FAtest_malloc(buf2.AudioBytes);
    fill_buf((float*)buf2.pAudioData, &fmt, 220, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src2, &buf2, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src2, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    /* hook up volume meter */
#ifdef _WIN32
    if(xaudio27){
        hr = CoCreateInstance(&CLSID_AudioVolumeMeter27, NULL,
                CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&vumeter);
        ok(hr == S_OK, "CoCreateInstance(AudioVolumeMeter) failed: %08x\n", hr);
    }else{
        hr = pCreateAudioVolumeMeter(&vumeter);
        ok(hr == S_OK, "CreateAudioVolumeMeter failed: %08x\n", hr);
    }
#else
    hr = FAudioCreateVolumeMeter(&vumeter, 0);
    ok(hr == S_OK, "FAudioCreateVolumeMeter failed: %08x\n", hr);
#endif

    effect.InitialState = TRUE;
    effect.OutputChannels = 2;
    effect.pEffect = vumeter;

    chain.EffectCount = 1;
    chain.pEffectDescriptors = &effect;

    hr = IXAudio2MasteringVoice_SetEffectChain(master, &chain);
    ok(hr == S_OK, "SetEffectchain failed: %08x\n", hr);

#ifdef _WIN32
    IUnknown_Release(vumeter);
#else
    vumeter->Release(vumeter);
#endif

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 22050)
            break;
        FAtest_sleep(100);
    }

    ok(state.SamplesPlayed == 22050, "Got wrong samples played\n");

    memset(&perfdata, 0, sizeof(perfdata));
    XA2CALL_V(GetPerformanceData, &perfdata);
    ok(perfdata.ActiveSourceVoiceCount == 2, "Got wrong ActiveSourceVoiceCount: %u\n",
            perfdata.ActiveSourceVoiceCount);
    ok(perfdata.TotalSourceVoiceCount == 2, "Got wrong TotalSourceVoiceCount: %u\n",
            perfdata.TotalSourceVoiceCount);
    ok(perfdata.CurrentLatencyInSamples > 0, "Got zero latency?\n");

    IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 22050, "Got wrong samples played after stop\n");
    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 22050, "Got wrong samples played after stop and start\n");
    IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    IXAudio2SourceVoice_FlushSourceBuffers(src);
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 22050, "Got wrong samples played after stop start and flush\n");
    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 22050, "Got wrong samples played after stop start flush and start\n");

    FAtest_free((void*)buf.pAudioData);
    FAtest_free((void*)buf2.pAudioData);

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src2);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src);
        IXAudio2SourceVoice_DestroyVoice(src2);
    }
    IXAudio2MasteringVoice_DestroyVoice(master);

    XA2CALL_V(UnregisterForCallbacks, &ecb);
}

FAudioGUID DATAFORMAT_SUBTYPE_PCM = {
    0x01,
    0x00,
    0x10,
    {
        0x80,
        0x00,
        0x00,
        0xAA,
        0x00,
        0x38,
        0x9B,
        0x71
    }
};

static UINT32 test_DeviceDetails(IXAudio27 *xa)
{
    HRESULT hr;
    XAUDIO2_DEVICE_DETAILS dd;
    UINT32 count, i;

    hr = IXAudio27_GetDeviceCount(xa, &count);
    ok(hr == S_OK, "GetDeviceCount failed: %08x\n", hr);

    if(count == 0)
        return 0;

    for(i = 0; i < count; ++i){
        hr = IXAudio27_GetDeviceDetails(xa, i, &dd);
        ok(hr == S_OK, "GetDeviceDetails failed: %08x\n", hr);

        ok(!memcmp(&dd.OutputFormat.SubFormat, &DATAFORMAT_SUBTYPE_PCM, sizeof(FAudioGUID)),
                "Expected SubFormat of device at index %u to be MFAudioFormat_PCM. "
                "Got {%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}.\n", i,
                dd.OutputFormat.SubFormat.Data1,
                dd.OutputFormat.SubFormat.Data2,
                dd.OutputFormat.SubFormat.Data3,
                dd.OutputFormat.SubFormat.Data4[0],
                dd.OutputFormat.SubFormat.Data4[1],
                dd.OutputFormat.SubFormat.Data4[2],
                dd.OutputFormat.SubFormat.Data4[3],
                dd.OutputFormat.SubFormat.Data4[4],
                dd.OutputFormat.SubFormat.Data4[5],
                dd.OutputFormat.SubFormat.Data4[6],
                dd.OutputFormat.SubFormat.Data4[7]);

        if(i == 0)
            ok(dd.Role == GlobalDefaultDevice, "Got wrong role for index 0: 0x%x\n", dd.Role);
        else
            ok(dd.Role == NotDefaultDevice, "Got wrong role for index %u: 0x%x\n", i, dd.Role);
    }

    return count;
}

static void WINAPI vcb_buf_OnVoiceProcessingPassStart(IXAudio2VoiceCallback *This,
        UINT32 BytesRequired)
{
}

static void WINAPI vcb_buf_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback *This)
{
}

static void WINAPI vcb_buf_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ok(0, "Unexpected OnStreamEnd\n");
}

struct vcb_buf_testdata {
    int idx;
    IXAudio2SourceVoice *src;
};

static int obs_calls = 0;
static int obe_calls = 0;

static void WINAPI vcb_buf_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    struct vcb_buf_testdata *data = pBufferContext;
    XAUDIO2_VOICE_STATE state;

    ok(data->idx == obs_calls, "Buffer callback out of order: %u\n", data->idx);

    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
    else
        IXAudio2SourceVoice_GetState(data->src, &state, 0);

    ok(state.BuffersQueued == 5 - obs_calls, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
    ok(state.pCurrentBufferContext == pBufferContext, "Got wrong buffer from GetState\n");

    ++obs_calls;
}

static void WINAPI vcb_buf_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    struct vcb_buf_testdata *data = pBufferContext;
    XAUDIO2_VOICE_STATE state;

    ok(data->idx == obe_calls, "Buffer callback out of order: %u\n", data->idx);

    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
    else
        IXAudio2SourceVoice_GetState(data->src, &state, 0);

    ok(state.BuffersQueued == 5 - obe_calls - 1, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
    if(state.BuffersQueued == 0)
        ok(state.pCurrentBufferContext == NULL, "Got wrong buffer from GetState: %p\n", state.pCurrentBufferContext);
    else
        ok(state.pCurrentBufferContext == data + 1, "Got wrong buffer from GetState: %p\n", state.pCurrentBufferContext);

    ++obe_calls;
}

static void WINAPI vcb_buf_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ok(0, "Unexpected OnLoopEnd\n");
}

static void WINAPI vcb_buf_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
    ok(0, "Unexpected OnVoiceError\n");
}

#ifdef _WIN32
static IXAudio2VoiceCallbackVtbl vcb_buf_vtbl = {
    vcb_buf_OnVoiceProcessingPassStart,
    vcb_buf_OnVoiceProcessingPassEnd,
    vcb_buf_OnStreamEnd,
    vcb_buf_OnBufferStart,
    vcb_buf_OnBufferEnd,
    vcb_buf_OnLoopEnd,
    vcb_buf_OnVoiceError
};

static IXAudio2VoiceCallback vcb_buf = { &vcb_buf_vtbl };
#else
static FAudioVoiceCallback vcb_buf = {
    vcb_buf_OnBufferEnd,
    vcb_buf_OnBufferStart,
    vcb_buf_OnLoopEnd,
    vcb_buf_OnStreamEnd,
    vcb_buf_OnVoiceError,
    vcb_buf_OnVoiceProcessingPassEnd,
    vcb_buf_OnVoiceProcessingPassStart
};
#endif

static int nloopends = 0;
static int nstreamends = 0;

static void WINAPI loop_buf_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ++nstreamends;
}

static void WINAPI loop_buf_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
}

static void WINAPI loop_buf_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
}

static void WINAPI loop_buf_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ++nloopends;
}

static void WINAPI loop_buf_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
}

#ifdef _WIN32
static IXAudio2VoiceCallbackVtbl loop_buf_vtbl = {
    vcb_buf_OnVoiceProcessingPassStart,
    vcb_buf_OnVoiceProcessingPassEnd,
    loop_buf_OnStreamEnd,
    loop_buf_OnBufferStart,
    loop_buf_OnBufferEnd,
    loop_buf_OnLoopEnd,
    loop_buf_OnVoiceError
};

static IXAudio2VoiceCallback loop_buf = { &loop_buf_vtbl };
#else
static FAudioVoiceCallback loop_buf = {
    loop_buf_OnBufferEnd,
    loop_buf_OnBufferStart,
    loop_buf_OnLoopEnd,
    loop_buf_OnStreamEnd,
    loop_buf_OnVoiceError,
    vcb_buf_OnVoiceProcessingPassEnd,
    vcb_buf_OnVoiceProcessingPassStart
};
#endif

static void test_buffer_callbacks(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    XAUDIO2_VOICE_STATE state;
    struct vcb_buf_testdata testdata[5];
    int i, timeout;

    obs_calls = 0;
    obe_calls = 0;

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    /* test OnBufferStart/End callbacks */
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &vcb_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 4410 * fmt.nBlockAlign;
    buf.pAudioData = FAtest_malloc(buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 4410);

    /* submit same buffer fragment 5 times */
    for(i = 0; i < 5; ++i){
        testdata[i].idx = i;
        testdata[i].src = src;
        buf.pContext = &testdata[i];

        hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);
    }

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);


    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    if(xaudio27){
        hr = IXAudio27SourceVoice_SetSourceSampleRate((IXAudio27SourceVoice*)src, 48000);
        ok(hr == S_OK, "SetSourceSampleRate failed: %08x\n", hr);
    }else{
        hr = IXAudio2SourceVoice_SetSourceSampleRate(src, 48000);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetSourceSampleRate should have failed: %08x\n", hr);
    }

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 4410 * 5)
            break;
        FAtest_sleep(100);
    }

    ok(state.SamplesPlayed == 4410 * 5, "Got wrong samples played\n");

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
        IXAudio2SourceVoice_DestroyVoice(src);


    /* test OnStreamEnd callback */
    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    buf.Flags = XAUDIO2_END_OF_STREAM;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    timeout = 0;
    while(nstreamends == 0 && timeout < 1000){
        FAtest_sleep(100);
        timeout += 100;
    }

    ok(nstreamends == 1, "Got wrong number of OnStreamEnd calls: %u\n", nstreamends);

    /* xaudio resets SamplesPlayed after processing an end-of-stream buffer */
    if(xaudio27)
        IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
    else
        IXAudio2SourceVoice_GetState(src, &state, 0);
    ok(state.SamplesPlayed == 0, "Got wrong samples played\n");

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
        IXAudio2SourceVoice_DestroyVoice(src);


    IXAudio2MasteringVoice_DestroyVoice(master);

    FAtest_free((void*)buf.pAudioData);
}

static UINT32 play_to_completion(IXAudio2SourceVoice *src, UINT32 max_samples)
{
    XAUDIO2_VOICE_STATE state;
    HRESULT hr;

    nloopends = 0;

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.BuffersQueued == 0)
            break;
        if(state.SamplesPlayed >= max_samples){
            if(xaudio27)
                IXAudio27SourceVoice_ExitLoop((IXAudio27SourceVoice*)src, XAUDIO2_COMMIT_NOW);
            else
                IXAudio2SourceVoice_ExitLoop(src, XAUDIO2_COMMIT_NOW);
        }
        FAtest_sleep(100);
    }

    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    return state.SamplesPlayed;
}

static void test_looping(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    UINT32 played, running_total = 0;

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);


    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &loop_buf, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));

    buf.AudioBytes = 44100 * fmt.nBlockAlign;
    buf.pAudioData = FAtest_malloc(buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 44100);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    /* play from middle to end */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 0;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 22050, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play 4410 samples from middle */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 0;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 4410, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* loop 4410 samples in middle */
    buf.PlayBegin = 0;
    buf.PlayLength = 0;
    buf.LoopBegin = 22050;
    buf.LoopLength = 4410;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 44100 + 4410, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play last half, then loop the whole buffer */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 22050 + 44100, "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* play short segment from middle, loop to the beginning, and end at PlayEnd */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, -1);
    ok(played - running_total == 4410 + (22050 + 4410), "Got wrong samples played: %u\n", played - running_total);
    running_total = played;
    ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);

    /* invalid: LoopEnd must be <= PlayEnd
     * xaudio27: play until LoopEnd, loop to beginning, play until PlayEnd */
    buf.PlayBegin = 22050;
    buf.PlayLength = 4410;
    buf.LoopBegin = 0;
    buf.LoopLength = 22050 + 4410 * 2;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    if(xaudio27){
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 4410 + (22050 + 4410 * 2), "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);

    /* invalid: LoopEnd must be within play range
     * xaudio27: plays only play range */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0; /* == until end of buffer */
    buf.LoopBegin = 0;
    buf.LoopLength = 22050;
    buf.LoopCount = 1;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    if(xaudio27){
        ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

        played = play_to_completion(src, -1);
        ok(played - running_total == 22050, "Got wrong samples played: %u\n", played - running_total);
        running_total = played;
        ok(nloopends == 0, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    }else
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);

    /* invalid: LoopBegin must be before PlayEnd
     * xaudio27: crashes */
    if(!xaudio27){
        buf.PlayBegin = 0;
        buf.PlayLength = 4410;
        buf.LoopBegin = 22050;
        buf.LoopLength = 4410;
        buf.LoopCount = 1;

        hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SubmitSourceBuffer should have failed: %08x\n", hr);
    }

    /* infinite looping buffer */
    buf.PlayBegin = 22050;
    buf.PlayLength = 0;
    buf.LoopBegin = 0;
    buf.LoopLength = 0;
    buf.LoopCount = 255;

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    played = play_to_completion(src, running_total + 88200);
    ok(played - running_total == 22050 + 44100 * 2, "Got wrong samples played: %u\n", played - running_total);
    ok(nloopends == (played - running_total) / 88200 + 1, "Got wrong OnLoopEnd calls: %u\n", nloopends);
    running_total = played;

    if(xaudio27)
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    else
        IXAudio2SourceVoice_DestroyVoice(src);
    IXAudio2MasteringVoice_DestroyVoice(master);
    FAtest_free((void*)buf.pAudioData);
}

static void test_submix(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SubmixVoice *sub;

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    XA2CALL(CreateSubmixVoice, &sub, 2, 44100, 0, 0, NULL, NULL);
    ok(hr == S_OK, "CreateSubmixVoice failed: %08x\n", hr);

    if(xaudio27){
        XAUDIO27_VOICE_DETAILS details;
        IXAudio27SubmixVoice_GetVoiceDetails((IXAudio27SubmixVoice*)sub, &details);
        ok(details.CreationFlags == 0, "Got wrong flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }else{
        XAUDIO2_VOICE_DETAILS details;
        IXAudio2SubmixVoice_GetVoiceDetails(sub, &details);
        ok(details.CreationFlags == 0, "Got wrong creation flags: 0x%x\n", details.CreationFlags);
        ok(details.ActiveFlags == 0, "Got wrong active flags: 0x%x\n", details.CreationFlags);
        ok(details.InputChannels == 2, "Got wrong channel count: 0x%x\n", details.InputChannels);
        ok(details.InputSampleRate == 44100, "Got wrong sample rate: 0x%x\n", details.InputSampleRate);
    }

    IXAudio2SubmixVoice_DestroyVoice(sub);
    IXAudio2MasteringVoice_DestroyVoice(master);
}

static void WINAPI flush_buf_OnVoiceProcessingPassStart(IXAudio2VoiceCallback *This,
        UINT32 BytesRequired)
{
}

static void WINAPI flush_buf_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback *This)
{
}

static void WINAPI flush_buf_OnStreamEnd(IXAudio2VoiceCallback *This)
{
    ok(0, "Unexpected OnStreamEnd\n");
}

static void WINAPI flush_buf_OnBufferStart(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
}

struct flush_buf_testdata {
    IXAudio2SourceVoice *src;
    int idx;
    int test;
};

static int flush_buf_tested;

static void WINAPI flush_buf_OnBufferEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    struct flush_buf_testdata *data = pBufferContext;
    XAUDIO2_VOICE_STATE state;
    XAUDIO2_BUFFER buf;
    HRESULT hr;

    ok(!is_main_thread(), "Buffer callback called from main thread!\n");
    if(!data)
        return;

    IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
    switch(data->test){
        case 0:
            ok(state.BuffersQueued == data->idx ? 0 : 1,
                "Got wrong number of buffers remaining for index %u: %u\n", data->idx, state.BuffersQueued);
            ok(data->idx == flush_buf_tested, "Wrong index %u\n", data->idx);
            break;

        case 1:
            ok(state.BuffersQueued == 1, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
            ok(data->idx == 1, "Wrong index %u\n", data->idx);
            break;

        case 2:
            ok(state.BuffersQueued == 1, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);

            /* Avoid it when first buffer is flushed */
            if(data->idx == 0)
                return;

            /* FlushSourceBuffers is not executed immediately even when called from a callback */
            memset(&buf, 0, sizeof(buf));
            buf.AudioBytes = 22050 * 2 * 4;
            buf.pAudioData = FAtest_malloc(buf.AudioBytes);
            memset((float*)buf.pAudioData, 0, buf.AudioBytes);

            hr = IXAudio2SourceVoice_SubmitSourceBuffer(data->src, &buf, NULL);
            ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
            ok(state.BuffersQueued == 2, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);

            hr = IXAudio2SourceVoice_FlushSourceBuffers(data->src);
            ok(hr == S_OK, "FlushSourceBuffers failed: %08x\n", hr);

            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)data->src, &state);
            ok(state.BuffersQueued == 2, "Got wrong number of buffers remaining: %u\n", state.BuffersQueued);
            break;
    }

    flush_buf_tested++;
}

static void WINAPI flush_buf_OnLoopEnd(IXAudio2VoiceCallback *This,
        void *pBufferContext)
{
    ok(0, "Unexpected OnLoopEnd\n");
}

static void WINAPI flush_buf_OnVoiceError(IXAudio2VoiceCallback *This,
        void *pBuffercontext, HRESULT Error)
{
    ok(0, "Unexpected OnVoiceError\n");
}

#ifdef _WIN32
static IXAudio2VoiceCallbackVtbl flush_buf_vtbl = {
    flush_buf_OnVoiceProcessingPassStart,
    flush_buf_OnVoiceProcessingPassEnd,
    flush_buf_OnStreamEnd,
    flush_buf_OnBufferStart,
    flush_buf_OnBufferEnd,
    flush_buf_OnLoopEnd,
    flush_buf_OnVoiceError
};

static IXAudio2VoiceCallback flush_buf = { &flush_buf_vtbl };
#else
static FAudioVoiceCallback flush_buf = {
    flush_buf_OnBufferEnd,
    flush_buf_OnBufferStart,
    flush_buf_OnLoopEnd,
    flush_buf_OnStreamEnd,
    flush_buf_OnVoiceError,
    flush_buf_OnVoiceProcessingPassEnd,
    flush_buf_OnVoiceProcessingPassStart
};
#endif

static void test_flush(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src;
    WAVEFORMATEX fmt;
    XAUDIO2_BUFFER buf;
    XAUDIO2_VOICE_STATE state;
    struct flush_buf_testdata testdata[2];
    int i, j;

    XA2CALL_0V(StopEngine);

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 2, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 2, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = 44100;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    fmt.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = 22050 * fmt.nBlockAlign;
    buf.pAudioData = FAtest_malloc(buf.AudioBytes);
    fill_buf((float*)buf.pAudioData, &fmt, 440, 22050);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    XA2CALL_0(StartEngine);
    ok(hr == S_OK, "StartEngine failed: %08x\n", hr);

    while(1){
        if(xaudio27)
            IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
        else
            IXAudio2SourceVoice_GetState(src, &state, 0);
        if(state.SamplesPlayed >= 2205)
            break;
        FAtest_sleep(10);
    }

    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Stop failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_FlushSourceBuffers(src);
    ok(hr == S_OK, "FlushSourceBuffers failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "Start failed: %08x\n", hr);

    FAtest_sleep(100);

    hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
    ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src);
    }

    /* In XA2.7 FlushSourceBuffers is always asynchronous. We also test Stop both
     * before and after FlushSourceBuffers, with two buffers submitted. During the
     * FlushSourceBuffers callbacks, even if Stop was called first (and thus both
     * buffers get OnBufferEnd events), the BuffersQueued is still 1 for the first
     * event. The game Legend of Heroes: Trails of Cold Steel 2 relies on this. */
    if(xaudio27){
        for(i = 0; i < 3; ++i){
            flush_buf_tested = 0;

            XA2CALL(CreateSourceVoice, &src, &fmt, 0, 1.f, &flush_buf, NULL, NULL);
            ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

            for(j = 0; j < 2; j++){
                testdata[j].idx = j;
                testdata[j].test = i;
                testdata[j].src = src;
                buf.pContext = &testdata[j];

                hr = IXAudio2SourceVoice_SubmitSourceBuffer(src, &buf, NULL);
                ok(hr == S_OK, "SubmitSourceBuffer failed: %08x\n", hr);
            }

            hr = IXAudio2SourceVoice_Start(src, 0, XAUDIO2_COMMIT_NOW);
            ok(hr == S_OK, "Start failed: %08x\n", hr);

            while(1){
                IXAudio27SourceVoice_GetState((IXAudio27SourceVoice*)src, &state);
                if(state.SamplesPlayed >= 2205)
                    break;
                FAtest_sleep(10);
            }

            switch(i){
                case 0:
                    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
                    ok(hr == S_OK, "Stop failed: %08x\n", hr);

                    hr = IXAudio2SourceVoice_FlushSourceBuffers(src);
                    ok(hr == S_OK, "FlushSourceBuffers failed: %08x\n", hr);

                    while(1){
                        if(flush_buf_tested >= 2)
                            break;
                        FAtest_sleep(10);
                    }
                    ok(flush_buf_tested == 2, "Wrong number of OnBufferEnd callbacks tested: %u\n", flush_buf_tested);
                    break;

                case 1:
                case 2:
                    hr = IXAudio2SourceVoice_FlushSourceBuffers(src);
                    ok(hr == S_OK, "FlushSourceBuffers failed: %08x\n", hr);

                    hr = IXAudio2SourceVoice_Stop(src, 0, XAUDIO2_COMMIT_NOW);
                    ok(hr == S_OK, "Stop failed: %08x\n", hr);

                    while(1){
                        if(flush_buf_tested >= 1)
                            break;
                        FAtest_sleep(10);
                    }
                    ok(flush_buf_tested == 1, "Wrong number of OnBufferEnd callbacks tested: %u\n", flush_buf_tested);
                    break;
            }
            IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src);
        }
    }
    IXAudio2MasteringVoice_DestroyVoice(master);

    FAtest_free((void*)buf.pAudioData);
}

static void test_setchannelvolumes(IXAudio2 *xa)
{
    HRESULT hr;
    IXAudio2MasteringVoice *master;
    IXAudio2SourceVoice *src_2ch, *src_8ch;
    WAVEFORMATEX fmt_2ch, fmt_8ch;
    float volumes[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};

    if(xaudio27)
        hr = IXAudio27_CreateMasteringVoice((IXAudio27*)xa, &master, 8, 44100, 0, 0, NULL);
    else
        hr = IXAudio2_CreateMasteringVoice(xa, &master, 8, 44100, 0,
#ifdef _WIN32
                NULL /*WCHAR *deviceID*/, NULL, AudioCategory_GameEffects);
#else
                0 /*int deviceIndex*/, NULL);
#endif
    ok(hr == S_OK, "CreateMasteringVoice failed: %08x\n", hr);

    fmt_2ch.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt_2ch.nChannels = 2;
    fmt_2ch.nSamplesPerSec = 44100;
    fmt_2ch.wBitsPerSample = 32;
    fmt_2ch.nBlockAlign = fmt_2ch.nChannels * fmt_2ch.wBitsPerSample / 8;
    fmt_2ch.nAvgBytesPerSec = fmt_2ch.nSamplesPerSec * fmt_2ch.nBlockAlign;
    fmt_2ch.cbSize = 0;

    fmt_8ch.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt_8ch.nChannels = 8;
    fmt_8ch.nSamplesPerSec = 44100;
    fmt_8ch.wBitsPerSample = 32;
    fmt_8ch.nBlockAlign = fmt_8ch.nChannels * fmt_8ch.wBitsPerSample / 8;
    fmt_8ch.nAvgBytesPerSec = fmt_8ch.nSamplesPerSec * fmt_8ch.nBlockAlign;
    fmt_8ch.cbSize = 0;

    XA2CALL(CreateSourceVoice, &src_2ch, &fmt_2ch, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    XA2CALL(CreateSourceVoice, &src_8ch, &fmt_8ch, 0, 1.f, NULL, NULL, NULL);
    ok(hr == S_OK, "CreateSourceVoice failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 2, volumes, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "SetChannelVolumes failed: %08x\n", hr);

    hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 8, volumes, XAUDIO2_COMMIT_NOW);
    ok(hr == S_OK, "SetChannelVolumes failed: %08x\n", hr);

    if(xaudio27){
        /* XAudio 2.7 doesn't check the number of channels */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 2, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == S_OK, "SetChannelVolumes failed: %08x\n", hr);
    }else{
        /* the number of channels must be the same as the number of channels on the source voice */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_8ch, 2, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08x\n", hr);

        hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 8, volumes, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08x\n", hr);

        /* volumes must not be NULL, XAudio 2.7 doesn't check this */
        hr = IXAudio2SourceVoice_SetChannelVolumes(src_2ch, 2, NULL, XAUDIO2_COMMIT_NOW);
        ok(hr == XAUDIO2_E_INVALID_CALL, "SetChannelVolumes should have failed: %08x\n", hr);
    }

    if(xaudio27){
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src_2ch);
        IXAudio27SourceVoice_DestroyVoice((IXAudio27SourceVoice*)src_8ch);
    }else{
        IXAudio2SourceVoice_DestroyVoice(src_2ch);
        IXAudio2SourceVoice_DestroyVoice(src_8ch);
    }

    IXAudio2MasteringVoice_DestroyVoice(master);
}

int main(int argc, char **argv)
{
    HRESULT hr;
    IXAudio2 *xa;
    IXAudio27 *xa27 = NULL;
    UINT32 has_devices;
#ifdef _WIN32
    HANDLE xa28dll;

    main_thread_id = GetCurrentThreadId();

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_XAudio27, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio27, (void**)&xa27);
#else
    main_thread_id = pthread_self();
    hr = FAudioCOMConstructEXT(&xa27, 7);
    ok(hr == S_OK, "Failed to create FAudio object\n");
#endif

    if(hr == S_OK){
        xaudio27 = TRUE;

        hr = IXAudio27_Initialize(xa27, 0, XAUDIO2_ANY_PROCESSOR);
        ok(hr == S_OK, "Initialize failed: %08x\n", hr);

        has_devices = test_DeviceDetails(xa27);
        if(has_devices){
            test_simple_streaming((IXAudio2*)xa27);
            test_buffer_callbacks((IXAudio2*)xa27);
            test_looping((IXAudio2*)xa27);
            test_submix((IXAudio2*)xa27);
            test_flush((IXAudio2*)xa27);
            test_setchannelvolumes((IXAudio2*)xa27);
        }else
            fprintf(stdout, "No audio devices available\n");

        IXAudio27_Release(xa27);
    }else
        fprintf(stdout, "XAudio2.7 not available, tests skipped\n");

#ifdef _WIN32
    hr = E_FAIL;
    xa28dll = LoadLibraryA("xaudio2_8.dll");
    if(xa28dll){
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");
        pCreateAudioVolumeMeter = (void*)GetProcAddress(xa28dll, "CreateAudioVolumeMeter");
        ok(pXAudio2Create != NULL && pCreateAudioVolumeMeter != NULL,
                "xaudio2_8 doesn't have expected exports?\n");

        if(pXAudio2Create)
            hr = pXAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
    }
#else
    hr = FAudioCreate(&xa, 0, FAUDIO_DEFAULT_PROCESSOR);
    ok(hr == S_OK, "Failed to create FAudio object\n");
#endif

    if(hr == S_OK){
        xaudio27 = FALSE;
        has_devices = test_DeviceDetails((IXAudio27*)xa);
        if(has_devices){
            test_simple_streaming(xa);
            test_buffer_callbacks(xa);
            test_looping(xa);
            test_submix(xa);
            test_flush(xa);
            test_setchannelvolumes(xa);
        }else
            fprintf(stdout, "No audio devices available\n");

        IXAudio2_Release(xa);
    }else
        fprintf(stdout, "XAudio2.8 not available, tests skipped\n");

    fprintf(stdout, "Finished with %u successful tests and %u failed tests.\n",
            success_count, failure_count);

    return failure_count > 0;
}
