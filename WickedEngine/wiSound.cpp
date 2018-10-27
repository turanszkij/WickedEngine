#include "wiSound.h"
#include "wiBackLog.h"

using namespace std;

wiAudioEngine* wiSoundEffect::audioEngine = nullptr;
wiAudioEngine* wiMusic::audioEngine = nullptr;


wiAudioEngine::wiAudioEngine()
{
	INITIALIZED = false;
	pXAudio2 = nullptr;
	pMasterVoice = nullptr;
	Initialize();
}
wiAudioEngine::~wiAudioEngine()
{
	if (pMasterVoice != nullptr) pMasterVoice->DestroyVoice();
	if (pXAudio2 != nullptr) pXAudio2->Release();
	SAFE_DELETE(pMasterVoice);
	SAFE_DELETE(pXAudio2);
	INITIALIZED = false;
}
HRESULT wiAudioEngine::Initialize()
{
	if (INITIALIZED)
		return S_OK;

	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	HRESULT hr;
	pXAudio2 = nullptr;
	if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
		return hr;
	pMasterVoice = nullptr;
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)))
		return hr;

	INITIALIZED = true;
	return EXIT_SUCCESS;
}
void wiAudioEngine::SetVolume(float vol) {
	if (INITIALIZED && pMasterVoice != nullptr)
		pMasterVoice->SetVolume(vol);
}
float wiAudioEngine::GetVolume() {
	float vol;
	if (pMasterVoice != nullptr)
		pMasterVoice->GetVolume(&vol);
	return vol;
}







wiSound::wiSound()
{
	pSourceVoice = nullptr;
}
wiSound::~wiSound()
{
	//if (pSourceVoice != nullptr) pSourceVoice->DestroyVoice();
	//SAFE_DELETE(pSourceVoice);
}

HRESULT wiSound::FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
	HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, LARGE_INTEGER(), NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if( 0 == ReadFile( hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        if( 0 == ReadFile( hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL ) )
            hr = HRESULT_FROM_WIN32( GetLastError() );

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if( 0 == ReadFile( hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL ) )
                hr = HRESULT_FROM_WIN32( GetLastError() );
            break;

		default:
			LARGE_INTEGER li = LARGE_INTEGER();
			li.QuadPart = dwChunkDataSize;
            if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, li, NULL, FILE_CURRENT ) )
            return HRESULT_FROM_WIN32( GetLastError() ); 
        }

        dwOffset += sizeof(DWORD) * 2;
        
        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;
        
        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;
    
}
HRESULT wiSound::ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;

	LARGE_INTEGER li = LARGE_INTEGER();
	li.QuadPart=bufferoffset;
	if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, li, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}
HRESULT wiSound::OpenFile(const TCHAR* strFileName)
{
	HRESULT hr;

	wfx = WAVEFORMATEX();
	buffer = XAUDIO2_BUFFER();

	// Open the file
	HANDLE hFile;

	hFile = CreateFile2(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		OPEN_EXISTING,
		nullptr
		);

	if (INVALID_HANDLE_VALUE == hFile)
		return HRESULT_FROM_WIN32(GetLastError());

	if (INVALID_SET_FILE_POINTER == SetFilePointerEx(hFile, LARGE_INTEGER(), NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile,fourccRIFF,dwChunkSize, dwChunkPosition );
	DWORD filetype;
	if(FAILED( hr = ReadChunkData(hFile,&filetype,sizeof(DWORD),dwChunkPosition) ))
		return hr;
	if (filetype != fourccWAVE)
		return S_FALSE;



	if(FAILED( hr = FindChunk(hFile,fourccFMT, dwChunkSize, dwChunkPosition ) ))
		return hr;
	if(FAILED( hr = ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition ) ))
		return hr;
	wfx.wFormatTag=WAVE_FORMAT_PCM;


	//fill out the audio data buffer with the contents of the fourccDATA chunk
	if(FAILED( hr = FindChunk(hFile,fourccDATA,dwChunkSize, dwChunkPosition ) ))
		return hr;
	BYTE * pDataBuffer = new BYTE[dwChunkSize];
	if(FAILED( hr = ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition) ))
		return hr;


	buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
	buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer


	return EXIT_SUCCESS;
}
HRESULT wiSound::Load(wstring filename)
{
	return OpenFile(filename.data());
}
HRESULT wiSound::Load(string filename)
{
	return OpenFile(wstring(filename.begin(),filename.end()).data());
}
void wiSound::Stop(){
	StopSound();
}
void wiSound::Initialize()
{
	pSourceVoice = nullptr;
}
HRESULT wiSound::PlaySound(wiAudioEngine* engine)
{
	if (engine == nullptr)
		return E_FAIL;

	if (!engine->INITIALIZED)
		return E_FAIL;

	HRESULT hr;
	pSourceVoice = nullptr;

	if (FAILED(hr = engine->pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx)))
		return hr;

	if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer)))
		return hr;

	if (FAILED(hr = pSourceVoice->Start(0)))
		return hr;

	return EXIT_SUCCESS;
}
void wiSound::StopSound()
{
	if (pSourceVoice != nullptr) {
		pSourceVoice->Stop();
	}
}


void wiSoundEffect::SetVolume(float vol) {
	if (audioEngine == nullptr)
		return;
	audioEngine->SetVolume(vol);
}
float wiSoundEffect::GetVolume() {
	if (audioEngine == nullptr)
		return 0;
	return audioEngine->GetVolume();
}
wiSoundEffect::wiSoundEffect()
{
	wiSound::Initialize();
}
wiSoundEffect::wiSoundEffect(wstring filename)
{
	wiSound::Initialize();
	Load(filename);
}
wiSoundEffect::wiSoundEffect(string filename)
{
	wiSound::Initialize();
	Load(filename);
}
wiSoundEffect::~wiSoundEffect()
{
}
HRESULT wiSoundEffect::Initialize()
{
	if (audioEngine == nullptr)
	{
		audioEngine = new wiAudioEngine;
	}

	if (audioEngine->INITIALIZED)
	{
		wiBackLog::post("wiSoundEffect Initialized");
		return S_OK;
	}
	else
	{
		wiBackLog::post("wiSoundEffect Initialization FAILED!");
		return E_FAIL;
	}
}
HRESULT wiSoundEffect::Play(DWORD delay)
{
	if (delay > 0) {
		thread([=] {
			Sleep(delay);
			PlaySound(audioEngine);
		}).detach();
		return S_OK;
	}
	return PlaySound(audioEngine);
}


void wiMusic::SetVolume(float vol){
	if (audioEngine == nullptr)
		return;
	audioEngine->SetVolume(vol);
}
float wiMusic::GetVolume(){
	if (audioEngine == nullptr)
		return 0;
	return audioEngine->GetVolume();
}
wiMusic::wiMusic()
{
	wiSound::Initialize();
	pSourceVoice = nullptr;
}
wiMusic::wiMusic(wstring filename)
{
	wiSound::Initialize();
	Load(filename);
}
wiMusic::wiMusic(string filename)
{
	wiSound::Initialize();
	Load(filename);
}
wiMusic::~wiMusic()
{
}
HRESULT wiMusic::Initialize()
{
	if (audioEngine == nullptr)
	{
		audioEngine = new wiAudioEngine;
	}

	if (audioEngine->INITIALIZED)
	{
		wiBackLog::post("wiMusic Initialized");
		return S_OK;
	}
	else
	{
		wiBackLog::post("wiMusic Initialization FAILED!");
		return E_FAIL;
	}
}
HRESULT wiMusic::Play(DWORD delay)
{
	if (delay > 0) {
		thread([=] {
			Sleep(delay);
			PlaySound(audioEngine);
		}).detach();
		return S_OK;
	}
	return PlaySound(audioEngine);
}
