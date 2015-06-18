#include "Sound.h"

IXAudio2* SoundEffect::pXAudio2;
IXAudio2MasteringVoice* SoundEffect::pMasterVoice;
bool SoundEffect::INITIALIZED=false;

IXAudio2* Music::pXAudio2;
IXAudio2MasteringVoice* Music::pMasterVoice;
bool Music::INITIALIZED=false;

using namespace std;




HRESULT Sound::FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) )
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
            if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, dwChunkDataSize, NULL, FILE_CURRENT ) )
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
HRESULT Sound::ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if( INVALID_SET_FILE_POINTER == SetFilePointer( hFile, bufferoffset, NULL, FILE_BEGIN ) )
        return HRESULT_FROM_WIN32( GetLastError() );
    DWORD dwRead;
    if( 0 == ReadFile( hFile, buffer, buffersize, &dwRead, NULL ) )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    return hr;
}
HRESULT Sound::OpenFile(TCHAR* strFileName)
{
	HRESULT hr;

	wfx = WAVEFORMATEX();
	buffer = XAUDIO2_BUFFER();

	// Open the file
	HANDLE hFile = CreateFile(
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



HRESULT Sound::Load(wstring filename)
{
	return OpenFile(filename._Myptr());
}
HRESULT Sound::Load(string filename)
{
	return OpenFile(wstring(filename.begin(),filename.end())._Myptr());
}
void Sound::DelayHelper(Sound* sound, DWORD delay){
	Sleep(delay);
	sound->Play();
}
HRESULT Sound::Play(DWORD delay)
{
	if(delay>0){
		thread(DelayHelper,this,delay).detach();
		return EXIT_SUCCESS;
	}
	return PlaySoundEffect();
}
void Sound::Stop(){
	StopSoundEffect();
}



void SoundEffect::SetVolume(float vol){
	if(INITIALIZED && pMasterVoice != nullptr)
		pMasterVoice->SetVolume(vol);
}
float SoundEffect::GetVolume(){
	float vol;
	if(pMasterVoice != nullptr)
		pMasterVoice->GetVolume(&vol);
	return vol;
}
HRESULT SoundEffect::Initialize()
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
void SoundEffect::CleanUp()
{
	if(pMasterVoice!=nullptr) pMasterVoice->DestroyVoice();
	if(pXAudio2!=nullptr) pXAudio2->Release();
	pMasterVoice=nullptr;
	pXAudio2=nullptr;
	INITIALIZED=false;
}
SoundEffect::SoundEffect()
{
	pSourceVoice=nullptr;
	if(!INITIALIZED) Initialize();
}
SoundEffect::SoundEffect(wstring filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
SoundEffect::SoundEffect(string filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
SoundEffect::~SoundEffect()
{
}
HRESULT SoundEffect::PlaySoundEffect()
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
void SoundEffect::StopSoundEffect()
{
	if(pSourceVoice!=nullptr){
		pSourceVoice->Stop();
	}
}


void Music::SetVolume(float vol){
	if(INITIALIZED && pMasterVoice != nullptr)
		pMasterVoice->SetVolume(vol);
}
float Music::GetVolume(){
	float vol;
	if(pMasterVoice != nullptr)
		pMasterVoice->GetVolume(&vol);
	return vol;
}
HRESULT Music::Initialize()
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
void Music::CleanUp()
{
	if(pMasterVoice!=nullptr) pMasterVoice->DestroyVoice();
	if(pXAudio2!=nullptr) pXAudio2->Release();
	pMasterVoice=nullptr;
	pXAudio2=nullptr;
	INITIALIZED=false;
}
Music::Music()
{
	pSourceVoice=nullptr;
	if(!INITIALIZED) Initialize();
}
Music::Music(wstring filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
Music::Music(string filename)
{
	if(!INITIALIZED) Initialize();
	Load(filename);
}
Music::~Music()
{
}
HRESULT Music::PlaySoundEffect()
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
void Music::StopSoundEffect()
{
	if(pSourceVoice!=nullptr){
		pSourceVoice->Stop();
	}
}
