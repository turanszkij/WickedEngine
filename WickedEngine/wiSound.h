#ifndef XAUDIO2
#define XAUDIO2

//#include <xaudio2.h>
////#include <xaudio2fx.h>



#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#else
#include "Xaudio2_7/comdecl.h"
#include "Xaudio2_7/xaudio2.h"
#endif

#include <string>
#include <thread>


#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

class wiSound
{
protected:
	WAVEFORMATEX wfx;
	XAUDIO2_BUFFER buffer;
	IXAudio2SourceVoice* pSourceVoice;

	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset);
	HRESULT OpenFile(TCHAR*);
	virtual HRESULT PlaySoundEffect()=0;
	virtual void StopSoundEffect()=0;
public:
	
	HRESULT Load(std::wstring);
	HRESULT Load(std::string);
	HRESULT Play(DWORD delay = 0);
	void Stop();

private:
	static void DelayHelper(wiSound* sound, DWORD delay);
};

class wiSoundEffect : public wiSound
{
private:
	static IXAudio2MasteringVoice* pMasterVoice;
	static IXAudio2* pXAudio2;
	static bool INITIALIZED;
	HRESULT PlaySoundEffect();
	void StopSoundEffect();

public:
	wiSoundEffect();
	wiSoundEffect(std::wstring);
	wiSoundEffect(std::string);
	~wiSoundEffect();

	static HRESULT Initialize();
	static void CleanUp();

	static void SetVolume(float);
	static float GetVolume();
};

class wiMusic : public wiSound
{
private:
	static IXAudio2MasteringVoice* pMasterVoice;
	static IXAudio2* pXAudio2;
	static bool INITIALIZED;
	HRESULT PlaySoundEffect();
	void StopSoundEffect();

public:
	wiMusic();
	wiMusic(std::wstring);
	wiMusic(std::string);
	~wiMusic();
	
	static HRESULT Initialize();
	static void CleanUp();

	static void SetVolume(float);
	static float GetVolume();
};

#endif

