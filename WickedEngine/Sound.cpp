#include "Sound.h"

IXAudio2* wiSoundEffect::pXAudio2;
IXAudio2MasteringVoice* wiSoundEffect::pMasterVoice;
bool wiSoundEffect::INITIALIZED = false;

IXAudio2* wiMusic::pXAudio2;
IXAudio2MasteringVoice* wiMusic::pMasterVoice;
bool wiMusic::INITIALIZED = false;

using namespace std;




HRESULT wiSound::FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
	HRESULT hr = S_OK;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, LARGE_INTEGER(), NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
#else
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
#endif

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
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
			LARGE_INTEGER li = LARGE_INTEGER();
			li.QuadPart = dwChunkDataSize;
            if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, li, NULL, FILE_CURRENT ) )
            return HRESULT_FROM_WIN32( GetLastError() ); 
#else
            if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
            return HRESULT_FROM_WIN32( GetLastError() ); 
#endif
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

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	LARGE_INTEGER li = LARGE_INTEGER();
	li.QuadPart=bufferoffset;
	if( INVALID_SET_FILE_POINTER == SetFilePointerEx( hFile, li, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
#else
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
#endif
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}
HRESULT wiSound::OpenFile(TCHAR* strFileName)
{
	HRESULT hr;

	wfx = WAVEFORMATEX();
	buffer = XAUDIO2_BUFFER();

	// Open the file
	HANDLE hFile;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
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
#else
	hFile = CreateFile(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL );

	if( INVALID_HANDLE_VALUE == hFile )
		return HRESULT_FROM_WIN32( GetLastError() );

	if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
		return HRESULT_FROM_WIN32( GetLastError() );
#endif

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
	return OpenFile(filename._Myptr());
}
HRESULT wiSound::Load(string filename)
{
	return OpenFile(wstring(filename.begin(),filename.end())._Myptr());
}
void wiSound::DelayHelper(wiSound* sound, DWORD delay){
	Sleep(delay);
	sound->Play();
}
HRESULT wiSound::Play(DWORD delay)
{
	if(delay>0){
		thread(DelayHelper,this,delay).detach();
		return EXIT_SUCCESS;
	}
	return PlaySoundEffect();
}
void wiSound::Stop(){
	StopSoundEffect();
}



void wiSoundEffect::SetVolume(float vol){
	if(INITIALIZED && pMasterVoice != nullptr)
		pMasterVoice->SetVolume(vol);
}
float wiSoundEffect::GetVolume(){
	float vol;
	if(pMasterVoice != nullptr)
		pMasterVoice->GetVolume(&vol);
	return vol;
}
HRESULT wiSoundEffect::Initialize()
{
	HRESULT hr;
	pXAudio2 = nullptr;
	if(FAILED( hr = XAudio2Create( &pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR ) ))
		return hr;
	pMasterVoice = nullptr;
	if(FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasterVoice ) ))
		return hr;

	INITIALIZED=true;
	return EXIT_SUCCESS;
}
void wiSoundEffect::CleanUp()
{
	if(pMasterVoice!=nullptr) pMasterVoice->DestroyVoice();
	if(pXAudio2!=nullptr) pXAudio2->Release();
	pMasterVoice=nullptr;
	pXAudio2=nullptr;
	INITIALIZED=false;
}
wiSoundEffect::wiSoundEffect()
{
	pSourceVoice=nullptr;
	if(!INITIALIZED) Initialize();
}
wiSoundEffect::wiSoundEffect(wstring filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
wiSoundEffect::wiSoundEffect(string filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
wiSoundEffect::~wiSoundEffect()
{
}
HRESULT wiSoundEffect::PlaySoundEffect()
{

	HRESULT hr;
	pSourceVoice = nullptr;

	if( FAILED(hr = pXAudio2->CreateSourceVoice( &pSourceVoice, (WAVEFORMATEX*)&wfx ) ) ) 
		return hr;

	if( FAILED(hr = pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
		return hr;

	if ( FAILED(hr = pSourceVoice->Start( 0 ) ) )
		return hr;

	return EXIT_SUCCESS;
}
void wiSoundEffect::StopSoundEffect()
{
	if(pSourceVoice!=nullptr){
		pSourceVoice->Stop();
	}
}


void wiMusic::SetVolume(float vol){
	if(INITIALIZED && pMasterVoice != nullptr)
		pMasterVoice->SetVolume(vol);
}
float wiMusic::GetVolume(){
	float vol;
	if(pMasterVoice != nullptr)
		pMasterVoice->GetVolume(&vol);
	return vol;
}
HRESULT wiMusic::Initialize()
{
	HRESULT hr;
	pXAudio2 = nullptr;
	if(FAILED( hr = XAudio2Create( &pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR ) ))
		return hr;
	pMasterVoice = nullptr;
	if(FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasterVoice ) ))
		return hr;

	INITIALIZED=true;
	return EXIT_SUCCESS;
}
void wiMusic::CleanUp()
{
	if(pMasterVoice!=nullptr) pMasterVoice->DestroyVoice();
	if(pXAudio2!=nullptr) pXAudio2->Release();
	pMasterVoice=nullptr;
	pXAudio2=nullptr;
	INITIALIZED=false;
}
wiMusic::wiMusic()
{
	pSourceVoice=nullptr;
	if(!INITIALIZED) Initialize();
}
wiMusic::wiMusic(wstring filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
wiMusic::wiMusic(string filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
wiMusic::~wiMusic()
{
}
HRESULT wiMusic::PlaySoundEffect()
{

	HRESULT hr;
	pSourceVoice = nullptr;

	if( FAILED(hr = pXAudio2->CreateSourceVoice( &pSourceVoice, (WAVEFORMATEX*)&wfx ) ) ) 
		return hr;

	if( FAILED(hr = pSourceVoice->SubmitSourceBuffer( &buffer ) ) )
		return hr;

	if ( FAILED(hr = pSourceVoice->Start( 0 ) ) )
		return hr;

	return EXIT_SUCCESS;
}
void wiMusic::StopSoundEffect()
{
	if(pSourceVoice!=nullptr){
		pSourceVoice->Stop();
	}
}
