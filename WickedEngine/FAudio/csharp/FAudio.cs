/* FAudio# - C# Wrapper for FAudio
 *
 * Copyright (c) 2018-2021 Ethan Lee.
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

#region Using Statements
using System;
using System.Runtime.InteropServices;
using System.Text;
#endregion

public static class FAudio
{
	#region Native Library Name

	const string nativeLibName = "FAudio";

	#endregion

	#region UTF8 Marshaling

	/* Used for stack allocated string marshaling. */
	private static int Utf8Size(string str)
	{
		return (str.Length * 4) + 1;
	}
	private static unsafe byte* Utf8Encode(string str, byte* buffer, int bufferSize)
	{
		fixed (char* strPtr = str)
		{
			Encoding.UTF8.GetBytes(strPtr, str.Length + 1, buffer, bufferSize);
		}

		return buffer;
	}

	/* Used for heap allocated string marshaling
	 * Returned byte* must be free'd with FreeHGlobal.
	 */
	private static unsafe byte* Utf8Encode(string str)
	{
		int bufferSize = (str.Length * 4) + 1;
		byte* buffer = (byte*)Marshal.AllocHGlobal(bufferSize);
		fixed (char* strPtr = str)
		{
			Encoding.UTF8.GetBytes(strPtr, str.Length + 1, buffer, bufferSize);
		}
		return buffer;
	}
	
	#endregion

	#region FAudio API

	/* Version */

	public const uint FAUDIO_TARGET_VERSION = 8;

	public const uint FAUDIO_ABI_VERSION =		 0;
	public const uint FAUDIO_MAJOR_VERSION =	21;
	public const uint FAUDIO_MINOR_VERSION =	 9;
	public const uint FAUDIO_PATCH_VERSION =	 0;

	public const uint FAUDIO_COMPILED_VERSION = (
		(FAUDIO_ABI_VERSION * 100 * 100 * 100) +
		(FAUDIO_MAJOR_VERSION * 100 * 100) +
		(FAUDIO_MINOR_VERSION * 100) +
		(FAUDIO_PATCH_VERSION)
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioLinkedVersion();

	/* Enumerations */

	[Flags]
	public enum FAudioDeviceRole
	{
		FAudioNotDefaultDevice =		0x0,
		FAudioDefaultConsoleDevice =		0x1,
		FAudioDefaultMultimediaDevice =		0x2,
		FAudioDefaultCommunicationsDevice =	0x4,
		FAudioDefaultGameDevice =		0x8,
		FAudioGlobalDefaultDevice =		0xF,
		FAudioInvalidDeviceRole = ~FAudioGlobalDefaultDevice
	}

	public enum FAudioFilterType
	{
		FAudioLowPassFilter,
		FAudioBandPassFilter,
		FAudioHighPassFilter,
		FAudioNotchFilter
	}

	/* FIXME: The original enum violates ISO C and is platform specific anyway... */
	public const uint FAUDIO_DEFAULT_PROCESSOR = 0xFFFFFFFF;

	/* Structures */

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public unsafe struct FAudioGUID
	{
		public uint Data1;
		public ushort Data2;
		public ushort Data3;
		public fixed byte Data4[8];
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioWaveFormatEx
	{
		public ushort wFormatTag;
		public ushort nChannels;
		public uint nSamplesPerSec;
		public uint nAvgBytesPerSec;
		public ushort nBlockAlign;
		public ushort wBitsPerSample;
		public ushort cbSize;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioWaveFormatExtensible
	{
		public FAudioWaveFormatEx Format;
		public ushort Samples; /* FIXME: union! */
		public uint dwChannelMask;
		public FAudioGUID SubFormat;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioADPCMCoefSet
	{
		public short iCoef1;
		public short iCoef2;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioADPCMWaveFormat
	{
		public FAudioWaveFormatEx wfx;
		public ushort wSamplesPerBlock;
		public ushort wNumCoef;
		public IntPtr aCoef; /* FAudioADPCMCoefSet[] */
		/* MSADPCM has 7 coefficient pairs:
		 * {
		 *	{ 256,    0 },
		 *	{ 512, -256 },
		 *	{   0,    0 },
		 *	{ 192,   64 },
		 *	{ 240,    0 },
		 *	{ 460, -208 },
		 *	{ 392, -232 }
		 * }
		 */
	}

	public struct FAudioXMA2WaveFormatEx
	{
		public FAudioWaveFormatEx wfx;
		public ushort wNumStreams;
		public uint dwChannelMask;
		public uint dwSamplesEncoded;
		public uint dwBytesPerBlock;
		public uint dwPlayBegin;
		public uint dwPlayLength;
		public uint dwLoopBegin;
		public uint dwLoopLength;
		public byte bLoopCount;
		public byte bEncoderVersion;
		public ushort wBlockCount;
	};

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public unsafe struct FAudioDeviceDetails
	{
		public fixed short DeviceID[256]; /* Win32 wchar_t */
		public fixed short DisplayName[256]; /* Win32 wchar_t */
		public FAudioDeviceRole Role;
		public FAudioWaveFormatExtensible OutputFormat;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioVoiceDetails
	{
		public uint CreationFlags;
		public uint ActiveFlags;
		public uint InputChannels;
		public uint InputSampleRate;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioSendDescriptor
	{
		public uint Flags;
		public IntPtr pOutputVoice; /* FAudioVoice* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioVoiceSends
	{
		public uint SendCount;
		public IntPtr pSends; /* FAudioSendDescriptor* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioEffectDescriptor
	{
		public IntPtr pEffect; /* void* */
		public int InitialState;
		public uint OutputChannels;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioEffectChain
	{
		public uint EffectCount;
		public IntPtr pEffectDescriptors; /* FAudioEffectDescriptor* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioFilterParameters
	{
		public FAudioFilterType Type;
		public float Frequency;
		public float OneOverQ;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioBuffer
	{
		public uint Flags;
		public uint AudioBytes;
		public IntPtr pAudioData; /* const uint8_t* */
		public uint PlayBegin;
		public uint PlayLength;
		public uint LoopBegin;
		public uint LoopLength;
		public uint LoopCount;
		public IntPtr pContext; /* void* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioBufferWMA
	{
		public IntPtr pDecodedPacketCumulativeBytes; /* const uint32_t* */
		public uint PacketCount;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioVoiceState
	{
		public IntPtr pCurrentBufferContext; /* void* */
		public uint BuffersQueued;
		public ulong SamplesPlayed;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioPerformanceData
	{
		public ulong AudioCyclesSinceLastQuery;
		public ulong TotalCyclesSinceLastQuery;
		public uint MinimumCyclesPerQuantum;
		public uint MaximumCyclesPerQuantum;
		public uint MemoryUsageInBytes;
		public uint CurrentLatencyInSamples;
		public uint GlitchesSinceEngineStarted;
		public uint ActiveSourceVoiceCount;
		public uint TotalSourceVoiceCount;
		public uint ActiveSubmixVoiceCount;
		public uint ActiveResamplerCount;
		public uint ActiveMatrixMixCount;
		public uint ActiveXmaSourceVoices;
		public uint ActiveXmaStreams;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FAudioDebugConfiguration
	{
		public uint TraceMask;
		public uint BreakMask;
		public int LogThreadID;
		public int LogFileline;
		public int LogFunctionName;
		public int LogTiming;
	}

	/* Constants */

	public const uint FAUDIO_MAX_BUFFER_BYTES =		0x80000000;
	public const uint FAUDIO_MAX_QUEUED_BUFFERS =		64;
	public const uint FAUDIO_MAX_AUDIO_CHANNELS =		64;
	public const uint FAUDIO_MIN_SAMPLE_RATE =		1000;
	public const uint FAUDIO_MAX_SAMPLE_RATE =		200000;
	public const float FAUDIO_MAX_VOLUME_LEVEL =		16777216.0f;
	public const float FAUDIO_MIN_FREQ_RATIO =		(1.0f / 1024.0f);
	public const float FAUDIO_MAX_FREQ_RATIO =		1024.0f;
	public const float FAUDIO_DEFAULT_FREQ_RATIO =		2.0f;
	public const float FAUDIO_MAX_FILTER_ONEOVERQ =		1.5f;
	public const float FAUDIO_MAX_FILTER_FREQUENCY =	1.0f;
	public const uint FAUDIO_MAX_LOOP_COUNT =		254;

	public const uint FAUDIO_COMMIT_NOW =		0;
	public const uint FAUDIO_COMMIT_ALL =		0;
	public const uint FAUDIO_INVALID_OPSET =	unchecked((uint) -1);
	public const uint FAUDIO_NO_LOOP_REGION =	0;
	public const uint FAUDIO_LOOP_INFINITE =	255;
	public const uint FAUDIO_DEFAULT_CHANNELS =	0;
	public const uint FAUDIO_DEFAULT_SAMPLERATE =	0;

	public const uint FAUDIO_DEBUG_ENGINE =			0x0001;
	public const uint FAUDIO_VOICE_NOPITCH =		0x0002;
	public const uint FAUDIO_VOICE_NOSRC =			0x0004;
	public const uint FAUDIO_VOICE_USEFILTER =		0x0008;
	public const uint FAUDIO_VOICE_MUSIC =			0x0010;
	public const uint FAUDIO_PLAY_TAILS =			0x0020;
	public const uint FAUDIO_END_OF_STREAM =		0x0040;
	public const uint FAUDIO_SEND_USEFILTER =		0x0080;
	public const uint FAUDIO_VOICE_NOSAMPLESPLAYED =	0x0100;
	public const uint FAUDIO_1024_QUANTUM =			0x8000;

	public const FAudioFilterType FAUDIO_DEFAULT_FILTER_TYPE =	FAudioFilterType.FAudioLowPassFilter;
	public const float FAUDIO_DEFAULT_FILTER_FREQUENCY =		FAUDIO_MAX_FILTER_FREQUENCY;
	public const float FAUDIO_DEFAULT_FILTER_ONEOVERQ =		1.0f;

	public const ushort FAUDIO_LOG_ERRORS =		0x0001;
	public const ushort FAUDIO_LOG_WARNINGS =	0x0002;
	public const ushort FAUDIO_LOG_INFO =		0x0004;
	public const ushort FAUDIO_LOG_DETAIL =		0x0008;
	public const ushort FAUDIO_LOG_API_CALLS =	0x0010;
	public const ushort FAUDIO_LOG_FUNC_CALLS =	0x0020;
	public const ushort FAUDIO_LOG_TIMING =		0x0040;
	public const ushort FAUDIO_LOG_LOCKS =		0x0080;
	public const ushort FAUDIO_LOG_MEMORY =		0x0100;
	public const ushort FAUDIO_LOG_STREAMING =	0x1000;

	/* FAudio Interface */

	/* FIXME: Do we want to actually reproduce the COM stuff or what...? -flibit */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioCreate(
		out IntPtr ppFAudio, /* FAudio** */
		uint Flags,
		uint XAudio2Processor /* FAudioProcessor */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_AddRef(
		IntPtr audio /* FAudio */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_Release(
		IntPtr audio /* FAudio* */
	);

	/* FIXME: QueryInterface? Or just ignore COM garbage... -flibit */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_GetDeviceCount(
		IntPtr audio, /* FAudio* */
		out uint pCount
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_GetDeviceDetails(
		IntPtr audio, /* FAudio* */
		uint Index,
		out FAudioDeviceDetails pDeviceDetails
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_Initialize(
		IntPtr audio, /* FAudio* */
		uint Flags,
		uint XAudio2Processor /* FAudioProcessor */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_RegisterForCallbacks(
		IntPtr audio, /* FAudio* */
		IntPtr pCallback /* FAudioEngineCallback* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_UnregisterForCallbacks(
		IntPtr audio, /* FAudio* */
		IntPtr pCallback /* FAudioEngineCallback* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_CreateSourceVoice(
		IntPtr audio, /* FAudio* */
		out IntPtr ppSourceVoice, /* FAudioSourceVoice** */
		ref FAudioWaveFormatEx pSourceFormat,
		uint Flags,
		float MaxFrequencyRatio,
		IntPtr pCallback, /* FAudioVoiceCallback* */
		IntPtr pSendList, /* FAudioVoiceSends* */
		IntPtr pEffectChain /* FAudioEffectChain* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_CreateSourceVoice(
		IntPtr audio, /* FAudio* */
		out IntPtr ppSourceVoice, /* FAudioSourceVoice** */
		IntPtr pSourceFormat, /* FAudioWaveFormatEx* */
		uint Flags,
		float MaxFrequencyRatio,
		IntPtr pCallback, /* FAudioVoiceCallback* */
		IntPtr pSendList, /* FAudioVoiceSends* */
		IntPtr pEffectChain /* FAudioEffectChain* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_CreateSubmixVoice(
		IntPtr audio, /* FAudio* */
		out IntPtr ppSubmixVoice, /* FAudioSubmixVoice** */
		uint InputChannels,
		uint InputSampleRate,
		uint Flags,
		uint ProcessingStage,
		IntPtr pSendList, /* FAudioVoiceSends* */
		IntPtr pEffectChain /* FAudioEffectChain* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_CreateMasteringVoice(
		IntPtr audio, /* FAudio* */
		out IntPtr ppMasteringVoice, /* FAudioMasteringVoice** */
		uint InputChannels,
		uint InputSampleRate,
		uint Flags,
		uint DeviceIndex,
		IntPtr pEffectChain /* FAudioEffectChain* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_StartEngine(
		IntPtr audio /* FAudio* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_StopEngine(
		IntPtr audio /* FAudio* */
	);

	[DllImport(nativeLibName, EntryPoint = "FAudio_CommitOperationSet", CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudio_CommitChanges(
		IntPtr audio /* FAudio* */,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_GetPerformanceData(
		IntPtr audio, /* FAudio* */
		out FAudioPerformanceData pPerfData
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_SetDebugConfiguration(
		IntPtr audio, /* FAudio* */
		ref FAudioDebugConfiguration pDebugConfiguration,
		IntPtr pReserved /* void* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_GetProcessingQuantum(
		IntPtr audio, /* FAudio */
		out uint quantumNumerator,
		out uint quantumDenominator
	);

	/* FAudioVoice Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetVoiceDetails(
		IntPtr voice, /* FAudioVoice* */
		out FAudioVoiceDetails pVoiceDetails
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetOutputVoices(
		IntPtr voice, /* FAudioVoice* */
		ref FAudioVoiceSends pSendList
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetEffectChain(
		IntPtr voice, /* FAudioVoice* */
		ref FAudioEffectChain pEffectChain
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_EnableEffect(
		IntPtr voice, /* FAudioVoice* */
		uint EffectIndex,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_DisableEffect(
		IntPtr voice, /* FAudioVoice* */
		uint EffectIndex,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetEffectState(
		IntPtr voice, /* FAudioVoice* */
		uint EffectIndex,
		out int pEnabled
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetEffectParameters(
		IntPtr voice, /* FAudioVoice* */
		uint EffectIndex,
		IntPtr pParameters, /* const void* */
		uint ParametersByteSize,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_GetEffectParameters(
		IntPtr voice, /* FAudioVoice* */
		uint EffectIndex,
		IntPtr pParameters, /* void* */
		uint ParametersByteSize
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetFilterParameters(
		IntPtr voice, /* FAudioVoice* */
		ref FAudioFilterParameters pParameters,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetFilterParameters(
		IntPtr voice, /* FAudioVoice* */
		out FAudioFilterParameters pParameters
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetOutputFilterParameters(
		IntPtr voice, /* FAudioVoice* */
		IntPtr pDestinationVoice, /* FAudioVoice */
		ref FAudioFilterParameters pParameters,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetOutputFilterParameters(
		IntPtr voice, /* FAudioVoice* */
		IntPtr pDestinationVoice, /* FAudioVoice */
		out FAudioFilterParameters pParameters
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetVolume(
		IntPtr voice, /* FAudioVoice* */
		float Volume,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetVolume(
		IntPtr voice, /* FAudioVoice* */
		out float pVolume
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetChannelVolumes(
		IntPtr voice, /* FAudioVoice* */
		uint Channels,
		float[] pVolumes,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetChannelVolumes(
		IntPtr voice, /* FAudioVoice* */
		uint Channels,
		float[] pVolumes
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioVoice_SetOutputMatrix(
		IntPtr voice, /* FAudioVoice* */
		IntPtr pDestinationVoice, /* FAudioVoice* */
		uint SourceChannels,
		uint DestinationChannels,
		IntPtr pLevelMatrix, /* float* */
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_GetOutputMatrix(
		IntPtr voice, /* FAudioVoice* */
		IntPtr pDestinationVoice, /* FAudioVoice* */
		uint SourceChannels,
		uint DestinationChannels,
		float[] pLevelMatrix
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioVoice_DestroyVoice(
		IntPtr voice /* FAudioVoice* */
	);

	/* FAudioSourceVoice Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_Start(
		IntPtr voice, /* FAudioSourceVoice* */
		uint Flags,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_Stop(
		IntPtr voice, /* FAudioSourceVoice* */
		uint Flags,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_SubmitSourceBuffer(
		IntPtr voice, /* FAudioSourceVoice* */
		ref FAudioBuffer pBuffer,
		IntPtr pBufferWMA /* const FAudioBufferWMA* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_SubmitSourceBuffer(
		IntPtr voice, /* FAudioSourceVoice* */
		ref FAudioBuffer pBuffer,
		ref FAudioBufferWMA pBufferWMA
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_FlushSourceBuffers(
		IntPtr voice /* FAudioSourceVoice* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_Discontinuity(
		IntPtr voice /* FAudioSourceVoice* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_ExitLoop(
		IntPtr voice, /* FAudioSourceVoice* */
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioSourceVoice_GetState(
		IntPtr voice, /* FAudioSourceVoice* */
		out FAudioVoiceState pVoiceState,
		uint Flags
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_SetFrequencyRatio(
		IntPtr voice, /* FAudioSourceVoice* */
		float Ratio,
		uint OperationSet
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudioSourceVoice_GetFrequencyRatio(
		IntPtr voice, /* FAudioSourceVoice* */
		out float pRatio
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioSourceVoice_SetSourceSampleRate(
		IntPtr voice, /* FAudioSourceVoice* */
		uint NewSourceSampleRate
	);

	/* FAudioEngineCallback Interface */

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnCriticalErrorFunc(
		IntPtr engineCallback, /* FAudioEngineCallback* */
		uint Error
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnProcessingPassEndFunc(
		IntPtr engineCallback /* FAudioEngineCallback* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnProcessingPassStartFunc(
		IntPtr engineCallback /* FAudioEngineCallback* */
	);

	[StructLayout(LayoutKind.Sequential)]
	public struct FAudioEngineCallback
	{
		public IntPtr OnCriticalError; /* OnCriticalErrorFunc */
		public IntPtr OnProcessingPassEnd; /* OnProcessingPassEndFunc */
		public IntPtr OnProcessingPassStart; /* OnProcessingPassStartFunc */
	}

	/* FAudioVoiceCallback Interface */

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnBufferEndFunc(
		IntPtr voiceCallback, /* FAudioVoiceCallback* */
		IntPtr pBufferContext /* void* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnBufferStartFunc(
		IntPtr voiceCallback, /* FAudioVoiceCallback* */
		IntPtr pBufferContext /* void* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnLoopEndFunc(
		IntPtr voiceCallback, /* FAudioVoiceCallback* */
		IntPtr pBufferContext /* void* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnStreamEndFunc(
		IntPtr voiceCallback /* FAudioVoiceCallback* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnVoiceErrorFunc(
		IntPtr voiceCallback, /* FAudioVoiceCallback* */
		IntPtr pBufferContext, /* void* */
		uint Error
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnVoiceProcessingPassEndFunc(
		IntPtr voiceCallback /* FAudioVoiceCallback* */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate void OnVoiceProcessingPassStartFunc(
		IntPtr voiceCallback, /* FAudioVoiceCallback* */
		uint BytesRequired
	);

	[StructLayout(LayoutKind.Sequential)]
	public struct FAudioVoiceCallback
	{
		public IntPtr OnBufferEnd; /* OnBufferEndFunc */
		public IntPtr OnBufferStart; /* OnBufferStartFunc */
		public IntPtr OnLoopEnd; /* OnLoopEndFunc */
		public IntPtr OnStreamEnd; /* OnStreamEndFunc */
		public IntPtr OnVoiceError; /* OnVoiceErrorFunc */
		public IntPtr OnVoiceProcessingPassEnd; /* OnVoiceProcessingPassEndFunc */
		public IntPtr OnVoiceProcessingPassStart; /* OnVoiceProcessingPassStartFunc */
	}

	#endregion

	#region FAudioFX API

	/* TODO */

	/* Structures */

	[StructLayout(LayoutKind.Sequential)]
	public struct FAudioFXReverbParameters
	{
		public float WetDryMix;
		public uint ReflectionsDelay;
		public byte ReverbDelay;
		public byte RearDelay;
		public byte PositionLeft;
		public byte PositionRight;
		public byte PositionMatrixLeft;
		public byte PositionMatrixRight;
		public byte EarlyDiffusion;
		public byte LateDiffusion;
		public byte LowEQGain;
		public byte LowEQCutoff;
		public byte HighEQGain;
		public byte HighEQCutoff;
		public float RoomFilterFreq;
		public float RoomFilterMain;
		public float RoomFilterHF;
		public float ReflectionsGain;
		public float ReverbGain;
		public float DecayTime;
		public float Density;
		public float RoomSize;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FAudioFXReverbParameters9
	{
		public float WetDryMix;
		public uint ReflectionsDelay;
		public byte ReverbDelay;
		public byte RearDelay;
		public byte SideDelay;
		public byte PositionLeft;
		public byte PositionRight;
		public byte PositionMatrixLeft;
		public byte PositionMatrixRight;
		public byte EarlyDiffusion;
		public byte LateDiffusion;
		public byte LowEQGain;
		public byte LowEQCutoff;
		public byte HighEQGain;
		public byte HighEQCutoff;
		public float RoomFilterFreq;
		public float RoomFilterMain;
		public float RoomFilterHF;
		public float ReflectionsGain;
		public float ReverbGain;
		public float DecayTime;
		public float Density;
		public float RoomSize;
	}

	/* Constants */

	public const float FAUDIOFX_REVERB_DEFAULT_WET_DRY_MIX =	100.0f;
	public const uint FAUDIOFX_REVERB_DEFAULT_REFLECTIONS_DELAY =	5;
	public const byte FAUDIOFX_REVERB_DEFAULT_REVERB_DELAY =	5;
	public const byte FAUDIOFX_REVERB_DEFAULT_REAR_DELAY =		5;
	public const byte FAUDIOFX_REVERB_DEFAULT_7POINT1_SIDE_DELAY =	5;
	public const byte FAUDIOFX_REVERB_DEFAULT_7POINT1_REAR_DELAY =	20;
	public const byte FAUDIOFX_REVERB_DEFAULT_POSITION =		6;
	public const byte FAUDIOFX_REVERB_DEFAULT_POSITION_MATRIX =	27;
	public const byte FAUDIOFX_REVERB_DEFAULT_EARLY_DIFFUSION =	8;
	public const byte FAUDIOFX_REVERB_DEFAULT_LATE_DIFFUSION =	8;
	public const byte FAUDIOFX_REVERB_DEFAULT_LOW_EQ_GAIN =		8;
	public const byte FAUDIOFX_REVERB_DEFAULT_LOW_EQ_CUTOFF =	4;
	public const byte FAUDIOFX_REVERB_DEFAULT_HIGH_EQ_GAIN =	8;
	public const byte FAUDIOFX_REVERB_DEFAULT_HIGH_EQ_CUTOFF =	4;
	public const float FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_FREQ =	5000.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_MAIN =	0.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_ROOM_FILTER_HF =	0.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_REFLECTIONS_GAIN =	0.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_REVERB_GAIN =	0.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_DECAY_TIME =		1.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_DENSITY =		100.0f;
	public const float FAUDIOFX_REVERB_DEFAULT_ROOM_SIZE =		100.0f;

	/* Functions */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioCreateReverb(out IntPtr ppApo, uint Flags);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAudioCreateReverb9(out IntPtr ppApo, uint Flags);

	#endregion

	#region FAPO API

	/* TODO */

	#endregion

	#region FAPOBase API

	/* TODO */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FAPOBase_Release(IntPtr fapo);

	#endregion

	#region FACT API

	/* Delegates */

	[UnmanagedFunctionPointer(CallingConvention.StdCall)]
	public delegate int FACTReadFileCallback(
		IntPtr hFile,
		IntPtr buffer,
		uint nNumberOfBytesToRead,
		IntPtr lpOverlapped /* FACTOverlapped* */
	);

	[UnmanagedFunctionPointer(CallingConvention.StdCall)]
	public delegate int FACTGetOverlappedResultCallback(
		IntPtr hFile,
		IntPtr lpOverlapped, /* FACTOverlapped* */
		out uint lpNumberOfBytesTransferred,
		int bWait
	);

	[UnmanagedFunctionPointer(CallingConvention.StdCall)]
	public delegate void FACTNotificationCallback(
		IntPtr pNotification /* const FACTNotification* */
	);

	/* Enumerations */

	public enum FACTWaveBankSegIdx
	{
		FACT_WAVEBANK_SEGIDX_BANKDATA = 0,
		FACT_WAVEBANK_SEGIDX_ENTRYMETADATA,
		FACT_WAVEBANK_SEGIDX_SEEKTABLES,
		FACT_WAVEBANK_SEGIDX_ENTRYNAMES,
		FACT_WAVEBANK_SEGIDX_ENTRYWAVEDATA,
		FACT_WAVEBANK_SEGIDX_COUNT
	}

	/* Structures */

	[StructLayout(LayoutKind.Sequential)]
	public unsafe struct FACTRendererDetails
	{
		public fixed short rendererID[0xFF]; // Win32 wchar_t
		public fixed short displayName[0xFF]; // Win32 wchar_t
		public int defaultDevice;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTOverlapped
	{
		public IntPtr Internal; /* ULONG_PTR */
		public IntPtr InternalHigh; /* ULONG_PTR */
		public uint Offset; /* FIXME: union! */
		public uint OffsetHigh; /* FIXME: union! */
		public IntPtr hEvent;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTFileIOCallbacks
	{
		public IntPtr readFileCallback; /* FACTReadCallback */
		public IntPtr getOverlappedResultCallback; /* FACTGetOverlappedResultCallback */
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTRuntimeParameters
	{
		public uint lookAheadTime;
		public IntPtr pGlobalSettingsBuffer;
		public uint globalSettingsBufferSize;
		public uint globalSettingsFlags;
		public uint globalSettingsAllocAttributes;
		public FACTFileIOCallbacks fileIOCallbacks;
		public IntPtr fnNotificationCallback; /* FACTNotificationCallback */
		public IntPtr pRendererID; /* Win32 wchar_t* */
		public IntPtr pXAudio2;
		public IntPtr pMasteringVoice;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTStreamingParameters
	{
		public IntPtr file;
		public uint offset;
		public uint flags;
		public ushort packetSize;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTWaveBankRegion
	{
		public uint dwOffset;
		public uint dwLength;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTWaveBankSampleRegion
	{
		public uint dwStartSample;
		public uint dwTotalSamples;
	}

	/* TODO
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTWaveBankHeader
	{
		public uint dwSignature;
		public uint dwVersion;
		public uint dwHeaderVersion;
		public fixed FACTWaveBankRegion Segments[FACT_WAVEBANK_SEGIDX_COUNT];
	}
	*/

	[StructLayout(LayoutKind.Sequential, Pack = 1)] /* FIXME: union! */
	public struct FACTWaveBankMiniWaveFormat
	{
		/*struct
		{
			public uint wFormatTag : 2;
			public uint nChannels : 3;
			public uint nSamplesPerSec : 18;
			public uint wBlockAlign : 8;
			public uint wBitsPerSample : 1;
		};*/
		public uint dwValue;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTWaveBankEntry
	{
		public uint dwFlagsAndDuration; /* FIXME: union! */
		public FACTWaveBankMiniWaveFormat Format;
		public FACTWaveBankRegion PlayRegion;
		public FACTWaveBankSampleRegion LoopRegion;
	}

	/* TODO
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTWaveBankEntryCompact
	{
		public uint dwOffset : 21;
		public uint dwLengthDeviation : 11;
	}
	*/

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public unsafe struct FACTWaveBankData
	{
		public uint dwFlags;
		public uint dwEntryCount;
		public fixed char szBankName[64];
		public uint dwEntryMetaDataElementSize;
		public uint dwEntryNameElementSize;
		public uint dwAlignment;
		public FACTWaveBankMiniWaveFormat CompactFormat;
		public ulong BuildTime;
	}

	[StructLayout(LayoutKind.Sequential)]
	public unsafe struct FACTWaveProperties
	{
		public fixed byte friendlyName[64];
		public FACTWaveBankMiniWaveFormat format;
		public uint durationInSamples;
		public FACTWaveBankSampleRegion loopRegion;
		public int streaming;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTWaveInstanceProperties
	{
		public FACTWaveProperties properties;
		public int backgroundMusic;
	}

	[StructLayout(LayoutKind.Sequential)]
	public unsafe struct FACTCueProperties
	{
		public fixed char friendlyName[0xFF];
		public int interactive;
		public ushort iaVariableIndex;
		public ushort numVariations;
		public byte maxInstances;
		public byte currentInstances;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTTrackProperties
	{
		public uint duration;
		public ushort numVariations;
		public byte numChannels;
		public ushort waveVariation;
		public byte loopCount;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTVariationProperties
	{
		public ushort index;
		public byte weight;
		public float iaVariableMin;
		public float iaVariableMax;
		public int linger;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTSoundProperties
	{
		public ushort category;
		public byte priority;
		public short pitch;
		public float volume;
		public ushort numTracks;
		public FACTTrackProperties arrTrackProperties; /* FIXME: [1] */
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTSoundVariationProperties
	{
		public FACTVariationProperties variationProperties;
		public FACTSoundProperties soundProperties;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct FACTCueInstanceProperties
	{
		public uint allocAttributes;
		public FACTCueProperties cueProperties;
		public FACTSoundVariationProperties activeVariationProperties;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationDescription
	{
		public byte type;
		public byte flags;
		public IntPtr pSoundBank; /* FACTSoundBank* */
		public IntPtr pWaveBank; /* FACTWaveBank* */
		public IntPtr pCue; /* FACTCue* */
		public IntPtr pWave; /* FACTWave* */
		public ushort cueIndex;
		public ushort waveIndex;
		public IntPtr pvContext;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationCue
	{
		public ushort cueIndex;
		public IntPtr pSoundBank; /* FACTSoundBank* */
		public IntPtr pCue; /* FACTCue* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationMarker
	{
		public ushort cueIndex;
		public IntPtr pSoundBank; /* FACTSoundBank* */
		public IntPtr pCue; /* FACTCue* */
		public uint marker;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationSoundBank
	{
		public IntPtr pSoundBank; /* FACTSoundBank* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationWaveBank
	{
		public IntPtr pWaveBank; /* FACTWaveBank* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationVariable
	{
		public ushort cueIndex;
		public IntPtr pSoundBank; /* FACTSoundBank* */
		public IntPtr pCue; /* FACTCue* */
		public ushort variableIndex;
		public float variableValue;
		public int local;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationGUI
	{
		public uint reserved;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotificationWave
	{
		public IntPtr pWaveBank; /* FACTWaveBank* */
		public ushort waveIndex;
		public ushort cueIndex;
		public IntPtr pSoundBank; /* FACTSoundBank* */
		public IntPtr pCue; /* FACTCue* */
		public IntPtr pWave; /* FACTWave* */
	}

	[StructLayout(LayoutKind.Explicit)]
	public struct FACTNotification_union
	{
		[FieldOffset(0)]
		public FACTNotificationCue cue;
		[FieldOffset(0)]
		public FACTNotificationMarker marker;
		[FieldOffset(0)]
		public FACTNotificationSoundBank soundBank;
		[FieldOffset(0)]
		public FACTNotificationWaveBank waveBank;
		[FieldOffset(0)]
		public FACTNotificationVariable variable;
		[FieldOffset(0)]
		public FACTNotificationGUI gui;
		[FieldOffset(0)]
		public FACTNotificationWave wave;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct FACTNotification
	{
		public byte type;
		public int timeStamp;
		public IntPtr pvContext;
		public FACTNotification_union anon;
	}

	/* Constants */

	public const int FACT_CONTENT_VERSION = 46;

	public const uint FACT_FLAG_MANAGEDATA =	0x00000001;

	public const uint FACT_FLAG_STOP_RELEASE =	0x00000000;
	public const uint FACT_FLAG_STOP_IMMEDIATE =	0x00000001;

	public const uint FACT_FLAG_BACKGROUND_MUSIC =	0x00000002;
	public const uint FACT_FLAG_UNITS_MS =		0x00000004;
	public const uint FACT_FLAG_UNITS_SAMPLES =	0x00000008;

	public const uint FACT_STATE_CREATED =		0x00000001;
	public const uint FACT_STATE_PREPARING =	0x00000002;
	public const uint FACT_STATE_PREPARED =		0x00000004;
	public const uint FACT_STATE_PLAYING =		0x00000008;
	public const uint FACT_STATE_STOPPING =		0x00000010;
	public const uint FACT_STATE_STOPPED =		0x00000020;
	public const uint FACT_STATE_PAUSED =		0x00000040;
	public const uint FACT_STATE_INUSE =		0x00000080;
	public const uint FACT_STATE_PREPAREFAILED =	0x80000000;

	public const short FACTPITCH_MIN =		-1200;
	public const short FACTPITCH_MAX =		 1200;
	public const short FACTPITCH_MIN_TOTAL =	-2400;
	public const short FACTPITCH_MAX_TOTAL =	 2400;

	public const float FACTVOLUME_MIN = 0.0f;
	public const float FACTVOLUME_MAX = 16777216.0f;

	public const ushort FACTINDEX_INVALID =		0xFFFF;
	public const ushort FACTVARIABLEINDEX_INVALID =	0xFFFF;
	public const ushort FACTCATEGORY_INVALID =	0xFFFF;

	public const uint FACT_ENGINE_LOOKAHEAD_DEFAULT = 250;

	public const byte FACTNOTIFICATIONTYPE_CUEPREPARED =				1;
	public const byte FACTNOTIFICATIONTYPE_CUEPLAY =				2;
	public const byte FACTNOTIFICATIONTYPE_CUESTOP =				3;
	public const byte FACTNOTIFICATIONTYPE_CUEDESTROYED =				4;
	public const byte FACTNOTIFICATIONTYPE_MARKER =					5;
	public const byte FACTNOTIFICATIONTYPE_SOUNDBANKDESTROYED =			6;
	public const byte FACTNOTIFICATIONTYPE_WAVEBANKDESTROYED =			7;
	public const byte FACTNOTIFICATIONTYPE_LOCALVARIABLECHANGED =			8;
	public const byte FACTNOTIFICATIONTYPE_GLOBALVARIABLECHANGED =			9;
	public const byte FACTNOTIFICATIONTYPE_GUICONNECTED =				10;
	public const byte FACTNOTIFICATIONTYPE_GUIDISCONNECTED =			11;
	public const byte FACTNOTIFICATIONTYPE_WAVEPREPARED =				12;
	public const byte FACTNOTIFICATIONTYPE_WAVEPLAY =				13;
	public const byte FACTNOTIFICATIONTYPE_WAVESTOP =				14;
	public const byte FACTNOTIFICATIONTYPE_WAVELOOPED =				15;
	public const byte FACTNOTIFICATIONTYPE_WAVEDESTROYED =				16;
	public const byte FACTNOTIFICATIONTYPE_WAVEBANKPREPARED =			17;
	public const byte FACTNOTIFICATIONTYPE_WAVEBANKSTREAMING_INVALIDCONTENT =	18;

	public const byte FACT_FLAG_NOTIFICATION_PERSIST = 0x01;

	public const uint FACT_WAVEBANK_TYPE_BUFFER =		0x00000000;
	public const uint FACT_WAVEBANK_TYPE_STREAMING =	0x00000001;
	public const uint FACT_WAVEBANK_TYPE_MASK =		0x00000001;

	public const uint FACT_WAVEBANK_FLAGS_ENTRYNAMES =	0x00010000;
	public const uint FACT_WAVEBANK_FLAGS_COMPACT =		0x00020000;
	public const uint FACT_WAVEBANK_FLAGS_SYNC_DISABLED =	0x00040000;
	public const uint FACT_WAVEBANK_FLAGS_SEEKTABLES =	0x00080000;
	public const uint FACT_WAVEBANK_FLAGS_MASK =		0x000F0000;

	/* AudioEngine Interface */

	/* FIXME: Do we want to actually reproduce the COM stuff or what...? -flibit */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCreateEngine(
		uint dwCreationFlags,
		out IntPtr ppEngine /* FACTAudioEngine** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_AddRef(
		IntPtr pEngine /* FACTAudioEngine* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_Release(
		IntPtr pEngine /* FACTAudioEngine* */
	);

	/* FIXME: QueryInterface? Or just ignore COM garbage... -flibit */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_GetRendererCount(
		IntPtr pEngine, /* FACTAudioEngine* */
		out ushort pnRendererCount
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_GetRendererDetails(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nRendererIndex,
		out FACTRendererDetails pRendererDetails
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_GetFinalMixFormat(
		IntPtr pEngine, /* FACTAudioEngine* */
		out FAudioWaveFormatExtensible pFinalMixFormat
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_Initialize(
		IntPtr pEngine, /* FACTAudioEngine* */
		ref FACTRuntimeParameters pParams
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_ShutDown(
		IntPtr pEngine /* FACTAudioEngine* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_DoWork(
		IntPtr pEngine /* FACTAudioEngine* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_CreateSoundBank(
		IntPtr pEngine, /* FACTAudioEngine* */
		IntPtr pvBuffer,
		uint dwSize,
		uint dwFlags,
		uint dwAllocAttributes,
		out IntPtr ppSoundBank /* FACTSoundBank** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_CreateInMemoryWaveBank(
		IntPtr pEngine, /* FACTAudioEngine* */
		IntPtr pvBuffer,
		uint dwSize,
		uint dwFlags,
		uint dwAllocAttributes,
		out IntPtr ppWaveBank /* FACTWaveBank** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_CreateStreamingWaveBank(
		IntPtr pEngine, /* FACTAudioEngine* */
		ref FACTStreamingParameters pParms,
		out IntPtr ppWaveBank /* FACTWaveBank** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe uint FACTAudioEngine_PrepareWave(
		IntPtr pEngine, /* FACTAudioEngine* */
		uint dwFlags,
		byte* szWavePath,
		uint wStreamingPacketSize,
		uint dwAlignment,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	);
	public static unsafe uint FACTAudioEngine_PrepareWave(
		IntPtr pEngine, /* FACTAudioEngine* */
		uint dwFlags,
		string szWavePath,
		uint wStreamingPacketSize,
		uint dwAlignment,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	) {
		byte* utf8WavePath = Utf8Encode(szWavePath);
		uint result = FACTAudioEngine_PrepareWave(
			pEngine,
			dwFlags,
			utf8WavePath,
			wStreamingPacketSize,
			dwAlignment,
			dwPlayOffset,
			nLoopCount,
			out ppWave
		);
		Marshal.FreeHGlobal((IntPtr) utf8WavePath);
		return result;
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_PrepareInMemoryWave(
		IntPtr pEngine, /* FACTAudioEngine* */
		uint dwFlags,
		FACTWaveBankEntry entry,
		uint[] pdwSeekTable, /* Optional! */
		byte[] pbWaveData,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_PrepareStreamingWave(
		IntPtr pEngine, /* FACTAudioEngine* */
		uint dwFlags,
		FACTWaveBankEntry entry,
		FACTStreamingParameters streamingParams,
		uint dwAlignment,
		uint[] pdwSeekTable, /* Optional! */
		byte[] pbWaveData,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_RegisterNotification(
		IntPtr pEngine, /* FACTAudioEngine* */
		ref FACTNotificationDescription pNotificationDescription
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_UnRegisterNotification(
		IntPtr pEngine, /* FACTAudioEngine* */
		ref FACTNotificationDescription pNotificationDescription
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe ushort FACTAudioEngine_GetCategory(
		IntPtr pEngine, /* FACTAudioEngine* */
		byte* szFriendlyName
	);
	public static unsafe ushort FACTAudioEngine_GetCategory(
		IntPtr pEngine, /* FACTAudioEngine* */
		string szFriendlyName
	) {
		int utf8BufSize = Utf8Size(szFriendlyName);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FACTAudioEngine_GetCategory(
			pEngine,
			Utf8Encode(szFriendlyName, utf8Buf, utf8BufSize)
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_Stop(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nCategory,
		uint dwFlags
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_SetVolume(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nCategory,
		float volume
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_Pause(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nCategory,
		int fPause
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe ushort FACTAudioEngine_GetGlobalVariableIndex(
		IntPtr pEngine, /* FACTAudioEngine* */
		byte* szFriendlyName
	);
	public static unsafe ushort FACTAudioEngine_GetGlobalVariableIndex(
		IntPtr pEngine, /* FACTAudioEngine* */
		string szFriendlyName
	) {
		int utf8BufSize = Utf8Size(szFriendlyName);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FACTAudioEngine_GetGlobalVariableIndex(
			pEngine,
			Utf8Encode(szFriendlyName, utf8Buf, utf8BufSize)
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_SetGlobalVariable(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nIndex,
		float nValue
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTAudioEngine_GetGlobalVariable(
		IntPtr pEngine, /* FACTAudioEngine* */
		ushort nIndex,
		out float pnValue
	);

	/* SoundBank Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe ushort FACTSoundBank_GetCueIndex(
		IntPtr pSoundBank, /* FACTSoundBank* */
		byte* szFriendlyName
	);
	public static unsafe ushort FACTSoundBank_GetCueIndex(
		IntPtr pSoundBank, /* FACTSoundBank* */
		string szFriendlyName
	)
	{
		int utf8BufSize = Utf8Size(szFriendlyName);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FACTSoundBank_GetCueIndex(
			pSoundBank,
			Utf8Encode(szFriendlyName, utf8Buf, utf8BufSize)
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_GetNumCues(
		IntPtr pSoundBank, /* FACTSoundBank* */
		out ushort pnNumCues
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_GetCueProperties(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		out FACTCueProperties pProperties
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Prepare(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		uint dwFlags,
		int timeOffset,
		out IntPtr ppCue /* FACTCue** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Play(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		uint dwFlags,
		int timeOffset,
		out IntPtr ppCue /* FACTCue** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Play(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		uint dwFlags,
		int timeOffset,
		IntPtr ppCue /* Pass IntPtr.Zero! */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Play3D(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		uint dwFlags,
		int timeOffset,
		ref F3DAUDIO_DSP_SETTINGS pDSPSettings,
		IntPtr ppCue /* Pass IntPtr.Zero! */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Stop(
		IntPtr pSoundBank, /* FACTSoundBank* */
		ushort nCueIndex,
		uint dwFlags
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_Destroy(
		IntPtr pSoundBank /* FACTSoundBank* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTSoundBank_GetState(
		IntPtr pSoundBank, /* FACTSoundBank* */
		out uint pdwState
	);

	/* WaveBank Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_Destroy(
		IntPtr pWaveBank /* FACTWaveBank* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_GetState(
		IntPtr pWaveBank, /* FACTWaveBank* */
		out uint pdwState
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_GetNumWaves(
		IntPtr pWaveBank, /* FACTWaveBank* */
		out ushort pnNumWaves
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe ushort FACTWaveBank_GetWaveIndex(
		IntPtr pWaveBank, /* FACTWaveBank* */
		byte* szFriendlyName
	);
	public static unsafe ushort FACTWaveBank_GetWaveIndex(
		IntPtr pWaveBank, /* FACTWaveBank* */
		string szFriendlyName
	) {
		int utf8BufSize = Utf8Size(szFriendlyName);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FACTWaveBank_GetWaveIndex(
			pWaveBank,
			Utf8Encode(szFriendlyName, utf8Buf, utf8BufSize)
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_GetWaveProperties(
		IntPtr pWaveBank, /* FACTWaveBank* */
		ushort nWaveIndex,
		out FACTWaveProperties pWaveProperties
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_Prepare(
		IntPtr pWaveBank, /* FACTWaveBank* */
		ushort nWaveIndex,
		uint dwFlags,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_Play(
		IntPtr pWaveBank, /* FACTWaveBank* */
		ushort nWaveIndex,
		uint dwFlags,
		uint dwPlayOffset,
		byte nLoopCount,
		out IntPtr ppWave /* FACTWave** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWaveBank_Stop(
		IntPtr pWaveBank, /* FACTWaveBank* */
		ushort nWaveIndex,
		uint dwFlags
	);

	/* Wave Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_Destroy(
		IntPtr pWave /* FACTWave* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_Play(
		IntPtr pWave /* FACTWave* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_Stop(
		IntPtr pWave, /* FACTWave* */
		uint dwFlags
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_Pause(
		IntPtr pWave, /* FACTWave* */
		int fPause
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_GetState(
		IntPtr pWave, /* FACTWave* */
		out uint pdwState
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_SetPitch(
		IntPtr pWave, /* FACTWave* */
		short pitch
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_SetVolume(
		IntPtr pWave, /* FACTWave* */
		float volume
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_SetMatrixCoefficients(
		IntPtr pWave, /* FACTWave* */
		uint uSrcChannelCount,
		uint uDstChannelCount,
		float[] pMatrixCoefficients
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTWave_GetProperties(
		IntPtr pWave, /* FACTWave* */
		out FACTWaveInstanceProperties pProperties
	);

	/* Cue Interface */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_Destroy(
		IntPtr pCue /* FACTCue* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_Play(
		IntPtr pCue /* FACTCue* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_Stop(
		IntPtr pCue, /* FACTCue* */
		uint dwFlags
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_GetState(
		IntPtr pCue, /* FACTCue* */
		out uint pdwState
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_SetMatrixCoefficients(
		IntPtr pCue, /* FACTCue* */
		uint uSrcChannelCount,
		uint uDstChannelCount,
		float[] pMatrixCoefficients
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe ushort FACTCue_GetVariableIndex(
		IntPtr pCue, /* FACTCue* */
		byte* szFriendlyName
	);
	public static unsafe ushort FACTCue_GetVariableIndex(
		IntPtr pCue, /* FACTCue* */
		string szFriendlyName
	) {
		int utf8BufSize = Utf8Size(szFriendlyName);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FACTCue_GetVariableIndex(
			pCue,
			Utf8Encode(szFriendlyName, utf8Buf, utf8BufSize)
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_SetVariable(
		IntPtr pCue, /* FACTCue* */
		ushort nIndex,
		float nValue
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_GetVariable(
		IntPtr pCue, /* FACTCue* */
		ushort nIndex,
		out float nValue
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_Pause(
		IntPtr pCue, /* FACTCue* */
		int fPause
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_GetProperties(
		IntPtr pCue, /* FACTCue* */
		out IntPtr ppProperties /* FACTCueInstanceProperties** */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_SetOutputVoices(
		IntPtr pCue, /* FACTCue* */
		IntPtr pSendList /* Optional FAudioVoiceSends* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACTCue_SetOutputVoiceMatrix(
		IntPtr pCue, /* FACTCue* */
		IntPtr pDestinationVoice, /* Optional FAudioVoice* */
		uint SourceChannels,
		uint DestinationChannels,
		float[] pLevelMatrix /* SourceChannels * DestinationChannels */
	);

	#endregion

	#region F3DAudio API

	/* Constants */

	public const uint SPEAKER_FRONT_LEFT =			0x00000001;
	public const uint SPEAKER_FRONT_RIGHT =			0x00000002;
	public const uint SPEAKER_FRONT_CENTER =		0x00000004;
	public const uint SPEAKER_LOW_FREQUENCY =		0x00000008;
	public const uint SPEAKER_BACK_LEFT =			0x00000010;
	public const uint SPEAKER_BACK_RIGHT =			0x00000020;
	public const uint SPEAKER_FRONT_LEFT_OF_CENTER =	0x00000040;
	public const uint SPEAKER_FRONT_RIGHT_OF_CENTER =	0x00000080;
	public const uint SPEAKER_BACK_CENTER =			0x00000100;
	public const uint SPEAKER_SIDE_LEFT =			0x00000200;
	public const uint SPEAKER_SIDE_RIGHT =			0x00000400;
	public const uint SPEAKER_TOP_CENTER =			0x00000800;
	public const uint SPEAKER_TOP_FRONT_LEFT =		0x00001000;
	public const uint SPEAKER_TOP_FRONT_CENTER =		0x00002000;
	public const uint SPEAKER_TOP_FRONT_RIGHT =		0x00004000;
	public const uint SPEAKER_TOP_BACK_LEFT =		0x00008000;
	public const uint SPEAKER_TOP_BACK_CENTER =		0x00010000;
	public const uint SPEAKER_TOP_BACK_RIGHT =		0x00020000;

	public const uint SPEAKER_MONO =	SPEAKER_FRONT_CENTER;
	public const uint SPEAKER_STEREO =	(SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
	public const uint SPEAKER_2POINT1 =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_LOW_FREQUENCY	);
	public const uint SPEAKER_SURROUND =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_FRONT_CENTER	|
			SPEAKER_BACK_CENTER	);
	public const uint SPEAKER_QUAD =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_BACK_LEFT	|
			SPEAKER_BACK_RIGHT	);
	public const uint SPEAKER_4POINT1 =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_LOW_FREQUENCY	|
			SPEAKER_BACK_LEFT	|
			SPEAKER_BACK_RIGHT	);
	public const uint SPEAKER_5POINT1 =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_FRONT_CENTER	|
			SPEAKER_LOW_FREQUENCY	|
			SPEAKER_BACK_LEFT	|
			SPEAKER_BACK_RIGHT	);
	public const uint SPEAKER_7POINT1 =
		(	SPEAKER_FRONT_LEFT		|
			SPEAKER_FRONT_RIGHT		|
			SPEAKER_FRONT_CENTER		|
			SPEAKER_LOW_FREQUENCY		|
			SPEAKER_BACK_LEFT		|
			SPEAKER_BACK_RIGHT		|
			SPEAKER_FRONT_LEFT_OF_CENTER	|
			SPEAKER_FRONT_RIGHT_OF_CENTER	);
	public const uint SPEAKER_5POINT1_SURROUND =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_FRONT_CENTER	|
			SPEAKER_LOW_FREQUENCY	|
			SPEAKER_SIDE_LEFT	|
			SPEAKER_SIDE_RIGHT	);
	public const uint SPEAKER_7POINT1_SURROUND =
		(	SPEAKER_FRONT_LEFT	|
			SPEAKER_FRONT_RIGHT	|
			SPEAKER_FRONT_CENTER	|
			SPEAKER_LOW_FREQUENCY	|
			SPEAKER_BACK_LEFT	|
			SPEAKER_BACK_RIGHT	|
			SPEAKER_SIDE_LEFT	|
			SPEAKER_SIDE_RIGHT	);

	/* FIXME: Hmmmm */
	public const uint SPEAKER_XBOX = SPEAKER_5POINT1;

	public const float F3DAUDIO_PI =	3.141592654f;
	public const float F3DAUDIO_2PI =	6.283185307f;

	public const uint F3DAUDIO_CALCULATE_MATRIX =		0x00000001;
	public const uint F3DAUDIO_CALCULATE_DELAY =		0x00000002;
	public const uint F3DAUDIO_CALCULATE_LPF_DIRECT =	0x00000004;
	public const uint F3DAUDIO_CALCULATE_LPF_REVERB =	0x00000008;
	public const uint F3DAUDIO_CALCULATE_REVERB =		0x00000010;
	public const uint F3DAUDIO_CALCULATE_DOPPLER =		0x00000020;
	public const uint F3DAUDIO_CALCULATE_EMITTER_ANGLE =	0x00000040;
	public const uint F3DAUDIO_CALCULATE_ZEROCENTER =	0x00010000;
	public const uint F3DAUDIO_CALCULATE_REDIRECT_TO_LFE =	0x00020000;

	/* Type Declarations */

	/* FIXME: Everything about this type blows */
	public const int F3DAUDIO_HANDLE_BYTESIZE = 20;
	// Alloc a byte[] of size F3DAUDIO_HANDLE_BYTESIZE!

	/* Structures */

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_VECTOR
	{
		public float x;
		public float y;
		public float z;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_DISTANCE_CURVE_POINT
	{
		public float Distance;
		public float DSPSetting;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_DISTANCE_CURVE
	{
		IntPtr pPoints; /* F3DAUDIO_DISTANCE_CURVE_POINT* */
		public uint PointCount;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_CONE
	{
		public float InnerAngle;
		public float OuterAngle;
		public float InnerVolume;
		public float OuterVolume;
		public float InnerLPF;
		public float OuterLPF;
		public float InnerReverb;
		public float OuterReverb;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_LISTENER
	{
		public F3DAUDIO_VECTOR OrientFront;
		public F3DAUDIO_VECTOR OrientTop;
		public F3DAUDIO_VECTOR Position;
		public F3DAUDIO_VECTOR Velocity;
		public IntPtr pCone; /* F3DAUDIO_CONE* */
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_EMITTER
	{
		public IntPtr pCone; /* F3DAUDIO_CONE* */
		public F3DAUDIO_VECTOR OrientFront;
		public F3DAUDIO_VECTOR OrientTop;
		public F3DAUDIO_VECTOR Position;
		public F3DAUDIO_VECTOR Velocity;
		public float InnerRadius;
		public float InnerRadiusAngle;
		public uint ChannelCount;
		public float ChannelRadius;
		public IntPtr pChannelAzimuths; /* float */
		public IntPtr pVolumeCurve;
		public IntPtr pLFECurve; /* F3DAUDIO_DISTANCE_CURVE* */
		public IntPtr pLPFDirectCurve; /* F3DAUDIO_DISTANCE_CURVE* */
		public IntPtr pLPFReverbCurve; /* F3DAUDIO_DISTANCE_CURVE* */
		public IntPtr pReverbCurve; /* F3DAUDIO_DISTANCE_CURVE* */
		public float CurveDistanceScaler;
		public float DopplerScaler;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct F3DAUDIO_DSP_SETTINGS
	{
		public IntPtr pMatrixCoefficients; /* float* */
		public IntPtr pDelayTimes; /* float* */
		public uint SrcChannelCount;
		public uint DstChannelCount;
		public float LPFDirectCoefficient;
		public float LPFReverbCoefficient;
		public float ReverbLevel;
		public float DopplerFactor;
		public float EmitterToListenerAngle;
		public float EmitterToListenerDistance;
		public float EmitterVelocityComponent;
		public float ListenerVelocityComponent;
	}

	/* Functions */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void F3DAudioInitialize(
		uint SpeakerChannelMask,
		float SpeedOfSound,
		byte[] Instance // F3DAUDIO_HANDLE
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint F3DAudioInitialize8(
		uint SpeakerChannelMask,
		float SpeedOfSound,
		byte[] Instance // F3DAUDIO_HANDLE
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void F3DAudioCalculate(
		byte[] Instance, // F3DAUDIO_HANDLE
		ref F3DAUDIO_LISTENER pListener,
		ref F3DAUDIO_EMITTER pEmitter,
		uint Flags,
		ref F3DAUDIO_DSP_SETTINGS pDSPSettings
	);

	#endregion

	#region FACT3D API

	/* Constants */

	public const float LEFT_AZIMUTH =			(3.0f * F3DAUDIO_PI / 2.0f);
	public const float RIGHT_AZIMUTH =			(F3DAUDIO_PI / 2.0f);
	public const float FRONT_LEFT_AZIMUTH =			(7.0f * F3DAUDIO_PI / 4.0f);
	public const float FRONT_RIGHT_AZIMUTH =		(F3DAUDIO_PI / 4.0f);
	public const float FRONT_CENTER_AZIMUTH =		0.0f;
	public const float LOW_FREQUENCY_AZIMUTH =		F3DAUDIO_2PI;
	public const float BACK_LEFT_AZIMUTH =			(5.0f * F3DAUDIO_PI / 4.0f);
	public const float BACK_RIGHT_AZIMUTH =			(3.0f * F3DAUDIO_PI / 4.0f);
	public const float BACK_CENTER_AZIMUTH =		F3DAUDIO_PI;
	public const float FRONT_LEFT_OF_CENTER_AZIMUTH =	(15.0f * F3DAUDIO_PI / 8.0f);
	public const float FRONT_RIGHT_OF_CENTER_AZIMUTH =	(F3DAUDIO_PI / 8.0f);

	public static readonly float[] aStereoLayout = new float[]
	{
		LEFT_AZIMUTH,
		RIGHT_AZIMUTH
	};
	public static readonly float[] a2Point1Layout = new float[]
	{
		LEFT_AZIMUTH,
		RIGHT_AZIMUTH,
		LOW_FREQUENCY_AZIMUTH
	};
	public static readonly float[] aQuadLayout = new float[]
	{
		FRONT_LEFT_AZIMUTH,
		FRONT_RIGHT_AZIMUTH,
		BACK_LEFT_AZIMUTH,
		BACK_RIGHT_AZIMUTH
	};
	public static readonly float[] a4Point1Layout = new float[]
	{
		FRONT_LEFT_AZIMUTH,
		FRONT_RIGHT_AZIMUTH,
		LOW_FREQUENCY_AZIMUTH,
		BACK_LEFT_AZIMUTH,
		BACK_RIGHT_AZIMUTH
	};
	public static readonly float[] a5Point1Layout = new float[]
	{
		FRONT_LEFT_AZIMUTH,
		FRONT_RIGHT_AZIMUTH,
		FRONT_CENTER_AZIMUTH,
		LOW_FREQUENCY_AZIMUTH,
		BACK_LEFT_AZIMUTH,
		BACK_RIGHT_AZIMUTH
	};
	public static readonly float[] a7Point1Layout = new float[]
	{
		FRONT_LEFT_AZIMUTH,
		FRONT_RIGHT_AZIMUTH,
		FRONT_CENTER_AZIMUTH,
		LOW_FREQUENCY_AZIMUTH,
		BACK_LEFT_AZIMUTH,
		BACK_RIGHT_AZIMUTH,
		LEFT_AZIMUTH,
		RIGHT_AZIMUTH
	};

	/* Functions */

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACT3DInitialize(
		IntPtr pEngine, /* FACTAudioEngine* */
		byte[] D3FInstance // F3DAUDIO_HANDLE
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACT3DCalculate(
		byte[] F3DInstance, // F3DAUDIO_HANDLE
		ref F3DAUDIO_LISTENER pListener,
		ref F3DAUDIO_EMITTER pEmitter,
		ref F3DAUDIO_DSP_SETTINGS pDSPSettings
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint FACT3DApply(
		ref F3DAUDIO_DSP_SETTINGS pDSPSettings,
		IntPtr pCue /* FACTCue* */
	);

	#endregion

	#region XNA Song API

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_SongInit();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_SongQuit();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe float XNA_PlaySong(byte* name);
	public static unsafe float XNA_PlaySong(string name)
	{
		int utf8BufSize = Utf8Size(name);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return XNA_PlaySong(Utf8Encode(name, utf8Buf, utf8BufSize));
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_PauseSong();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_ResumeSong();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_StopSong();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_SetSongVolume(float volume);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint XNA_GetSongEnded();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_EnableVisualization(uint enable);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint XNA_VisualizationEnabled();

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void XNA_GetSongVisualizationData(
		float[] frequencies,
		float[] samples,
		uint count
	);

	#endregion

	#region FAudio I/O API

	/* Delegates */

	/* IntPtr refers to a size_t */
	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate IntPtr FAudio_readfunc(
		IntPtr data, /* void* */
		IntPtr dst, /* void* */
		IntPtr size, /* size_t */
		IntPtr count /* size_t */
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate long FAudio_seekfunc(
		IntPtr data, /* void* */
		long offset,
		int whence
	);

	[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
	public delegate int FAudio_closefunc(IntPtr data);

	/* Structures */

	[StructLayout(LayoutKind.Sequential)]
	public struct FAudioIOStream
	{
		public IntPtr data;
		public IntPtr read; /* FAudio_readfunc */
		public IntPtr seek; /* FAudio_seekfunc */
		public IntPtr close; /* FAudio_closefunc */
		public IntPtr ioLock; /* lock, lolC# */
	}

	/* Functions */

	/* IntPtr refers to an FAudioIOStream* */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe IntPtr FAudio_fopen(byte* path);
	public static unsafe IntPtr FAudio_fopen(string path)
	{
		int utf8BufSize = Utf8Size(path);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return FAudio_fopen(Utf8Encode(path, utf8Buf, utf8BufSize));
	}

	/* IntPtr refers to an FAudioIOStream* */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern IntPtr FAudio_memopen(IntPtr mem, int len);

	/* IntPtr refers to a uint8_t* */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern IntPtr FAudio_memptr(IntPtr io, IntPtr offset);

	/* io refers to an FAudioIOStream* */
	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void FAudio_close(IntPtr io);

	#endregion

	#region stb_vorbis

	/* Because why not? */

	[StructLayout(LayoutKind.Sequential)]
	public struct stb_vorbis_alloc
	{
		public IntPtr alloc_buffer;
		public int alloc_buffer_length_in_bytes;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct stb_vorbis_info
	{
		public uint sample_rate;
		public int channels;

		public uint setup_memory_required;
		public uint setup_temp_memory_required;
		public uint temp_memory_required;

		public int max_frame_size;
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct stb_vorbis_comment
	{
		public IntPtr vendor;

		public int comment_list_length;
		public IntPtr comment_list;
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern stb_vorbis_info stb_vorbis_get_info(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern stb_vorbis_comment stb_vorbis_get_comment(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_error(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern void stb_vorbis_close(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_sample_offset(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint stb_vorbis_get_file_offset(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern IntPtr stb_vorbis_open_memory(
		IntPtr data,
		int len,
		out int error,
		IntPtr alloc_buffer /* stb_vorbis_alloc* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	private static extern unsafe IntPtr stb_vorbis_open_filename(
		byte* filename,
		out int error,
		IntPtr alloc_buffer /* stb_vorbis_alloc* */
	);
	public static unsafe IntPtr stb_vorbis_open_filename(
		string filename,
		out int error,
		IntPtr alloc_buffer /* stb_vorbis_alloc* */
	) {
		int utf8BufSize = Utf8Size(filename);
		byte* utf8Buf = stackalloc byte[utf8BufSize];
		return stb_vorbis_open_filename(
			Utf8Encode(filename, utf8Buf, utf8BufSize),
			out error,
			alloc_buffer
		);
	}

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern IntPtr stb_vorbis_open_file(
		IntPtr f, /* FAudioIOStream* */
		int close_handle_on_close,
		out int error,
		IntPtr alloc_buffer /* stb_vorbis_alloc* */
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern IntPtr stb_vorbis_open_file_section(
		IntPtr f, /* FAudioIOStream* */
		int close_handle_on_close,
		out int error,
		IntPtr alloc_buffer, /* stb_vorbis_alloc* */
		uint len
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_seek_frame(IntPtr f, uint sample_number);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_seek(IntPtr f, uint sample_number);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_seek_start(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern uint stb_vorbis_stream_length_in_samples(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern float stb_vorbis_stream_length_in_seconds(IntPtr f);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_frame_float(
		IntPtr f,
		out int channels,
		ref float[][] output
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_frame_float(
		IntPtr f,
		IntPtr channels, /* IntPtr.Zero */
		ref float[][] output
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_samples_float_interleaved(
		IntPtr f,
		int channels,
		float[] buffer,
		int num_floats
	);

	[DllImport(nativeLibName, CallingConvention = CallingConvention.Cdecl)]
	public static extern int stb_vorbis_get_samples_float(
		IntPtr f,
		int channels,
		float[][] buffer,
		int num_samples
	);

	#endregion
}
