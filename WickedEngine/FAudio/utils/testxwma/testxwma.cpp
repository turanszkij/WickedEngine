#include <stdio.h>
#include <SDL.h>

#include <FAudio.h>

float argPlayBegin = 0.0f;
float argPlayLength = 0.0f;
float argLoopBegin = 0.0f;
float argLoopLength = 0.0f;
uint32_t argLoopCount = 0;

FAudio *faudio = NULL;
FAudioMasteringVoice *mastering_voice = NULL;
FAudioSourceVoice *source_voice = NULL;

FAudioWaveFormatExtensible *wfx = NULL;
FAudioBuffer buffer = {0};
FAudioBufferWMA buffer_wma = {0};

/* based on https://docs.microsoft.com/en-us/windows/desktop/xaudio2/how-to--load-audio-data-files-in-xaudio2 */
#define fourccRIFF *((uint32_t *) "RIFF")
#define fourccDATA *((uint32_t *) "data")
#define fourccFMT  *((uint32_t *) "fmt ")
#define fourccWAVE *((uint32_t *) "WAVE")
#define fourccXWMA *((uint32_t *) "XWMA")
#define fourccDPDS *((uint32_t *) "dpds")

uint32_t FindChunk(FILE *hFile, uint32_t fourcc, uint32_t *dwChunkSize, uint32_t *dwChunkDataPosition)
{
	uint32_t hr = 0;

	if (fseek(hFile, 0, SEEK_SET) != 0)
	{ 
		return -1;
	}

	uint32_t dwChunkType;
	uint32_t dwChunkDataSize;
	uint32_t dwRIFFDataSize = 0;
	uint32_t dwFileType;
	uint32_t bytesRead = 0;
	uint32_t dwOffset = 0;

	while (hr == 0)
	{
		if (fread(&dwChunkType, sizeof(uint32_t), 1, hFile) < 1)
			hr = 1;

		if (fread(&dwChunkDataSize, sizeof(uint32_t), 1, hFile) < 1)
			hr = 1;

		if (dwChunkType == fourccRIFF)
		{
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;

			if (fread(&dwFileType, sizeof(uint32_t), 1, hFile) < 1)
				hr = 1;
		}
		else
		{
			if (fseek(hFile, dwChunkDataSize, SEEK_CUR) != 0)
				return 1;
		}

		dwOffset += sizeof(uint32_t) * 2;

		if (dwChunkType == fourcc)
		{
			*dwChunkSize = dwChunkDataSize;
			*dwChunkDataPosition = dwOffset;
			return 0;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) 
			return 1;

	}

	return 1;
}

uint32_t ReadChunkData(FILE *hFile, void * buffer, uint32_t buffersize, uint32_t bufferoffset)
{
	uint32_t hr = 0;

	if (fseek(hFile, bufferoffset, SEEK_SET) != 0)
		return 1;

	if (fread(buffer, buffersize, 1, hFile) < 1)
		hr = 1;

	return hr;
}

uint32_t load_data(const char *filename)
{
	/* open the audio file */
	FILE *hFile = fopen(filename, "rb");
	if (!hFile)
		return 1;

	fseek(hFile, 0, SEEK_SET);

	/* Locate the 'RIFF' chunk in the audio file, and check the file type. */
	uint32_t dwChunkSize;
	uint32_t dwChunkPosition;
	uint32_t filetype;

	FindChunk(hFile,fourccRIFF, &dwChunkSize, &dwChunkPosition);
	ReadChunkData(hFile, &filetype, sizeof(uint32_t), dwChunkPosition);
	
	if (filetype != fourccWAVE && filetype != fourccXWMA)
		return 1;

	/* Locate the 'fmt ' chunk, and copy its contents into a WAVEFORMATEXTENSIBLE structure. */
	FindChunk(hFile,fourccFMT, &dwChunkSize, &dwChunkPosition );
	if (dwChunkSize > sizeof(FAudioWaveFormatExtensible))
	{
		wfx = (FAudioWaveFormatExtensible *) malloc(dwChunkSize);
		printf("chunk-size exceeds wfx size, allocating more: %u > %u\n", dwChunkSize, sizeof(FAudioWaveFormatExtensible));
	}
	else
	{
		wfx = (FAudioWaveFormatExtensible *) malloc(sizeof(FAudioWaveFormatExtensible));
		printf("chunk-size equal or less than wfx size, capping: %u <= %u\n", dwChunkSize, sizeof(FAudioWaveFormatExtensible));
	}
	ReadChunkData(hFile, wfx, dwChunkSize, dwChunkPosition );

	/* Locate the 'data' chunk, and read its contents into a buffer. */
	FindChunk(hFile, fourccDATA, &dwChunkSize, &dwChunkPosition);
	uint8_t *pDataBuffer = (uint8_t *) malloc(dwChunkSize);
	ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	printf("data chunk size: %u\n", dwChunkSize);
	buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
	buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
	buffer.Flags = FAUDIO_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	/* Locate the 'dpds' chunk, and read its contents into a buffer. */
	if (FindChunk(hFile, fourccDPDS, &dwChunkSize, &dwChunkPosition) == 0) 
	{
		uint32_t *cumulBytes = (uint32_t *) malloc(dwChunkSize);
		ReadChunkData(hFile, cumulBytes, dwChunkSize, dwChunkPosition);

		buffer_wma.pDecodedPacketCumulativeBytes = cumulBytes;
		buffer_wma.PacketCount = dwChunkSize / sizeof(uint32_t);
	}

	fclose(hFile);

	return 0;
}

void faudio_setup() {
	uint32_t hr = FAudioCreate(&faudio, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (hr != 0) {
		return;
	}

	hr = FAudio_CreateMasteringVoice(faudio, &mastering_voice, 2, 44100, 0, 0, NULL);
	if (hr != 0) {
		return;
	}

	hr = FAudio_CreateSourceVoice(
		faudio, 
		&source_voice, 
		(FAudioWaveFormatEx *) wfx,
		FAUDIO_VOICE_USEFILTER, 
		FAUDIO_MAX_FREQ_RATIO, 
		NULL, NULL, NULL
	);
}

void play(void) {

	buffer.PlayBegin = argPlayBegin * wfx->Format.nSamplesPerSec;
	buffer.PlayLength = argPlayLength * wfx->Format.nSamplesPerSec;

	buffer.LoopBegin = argLoopBegin * wfx->Format.nSamplesPerSec;
	buffer.LoopLength = argLoopLength * wfx->Format.nSamplesPerSec;
	buffer.LoopCount = argLoopCount;

	if (buffer_wma.pDecodedPacketCumulativeBytes != NULL)
		FAudioSourceVoice_SubmitSourceBuffer(source_voice, &buffer, &buffer_wma);
	else
		FAudioSourceVoice_SubmitSourceBuffer(source_voice, &buffer, NULL);

	uint32_t hr = FAudioSourceVoice_Start(source_voice, 0, FAUDIO_COMMIT_NOW);

	int is_running = 1;
	while (hr == 0 && is_running) {
		FAudioVoiceState state;
		FAudioSourceVoice_GetState(source_voice, &state, 0);
		is_running = (state.BuffersQueued > 0) != 0;
		SDL_Delay(10);
	}

	FAudioVoice_DestroyVoice(source_voice);

	/* free allocated space for FAudioWafeFormatExtensible */
	free(wfx);
}

int main(int argc, char *argv[]) {
	
	/* process command line arguments. This is just a test program, didn't go too nuts with validation. */
	if (argc < 2 || (argc > 4 && argc != 7)) {
		printf("Usage: %s filename [PlayBegin] [PlayLength] [LoopBegin LoopLength LoopCount]\n", argv[0]);
		printf(" - filename (required): can be either a WAV or xWMA audio file.\n");
		printf(" - PlayBegin (optional): start playing at this offset. (in seconds)\n");
		printf(" - PlayLength (optional): duration of the region to be played. (in seconds)\n");
		printf(" - LoopBegin (optional): start looping at this offset. (in seconds)\n");
		printf(" - LoopLength (optional): duration of the loop (in seconds)\n");
		printf(" - LoopCount (optional): number of times to loop\n");
		return -1;
	}

	switch (argc) 
	{
		case 7:
			sscanf(argv[6], "%d", &argLoopCount);
			sscanf(argv[5], "%f", &argLoopLength);
			sscanf(argv[4], "%f", &argLoopBegin);
		
		case 4:
			sscanf(argv[3], "%f", &argPlayLength);

		case 3:
			sscanf(argv[2], "%f", &argPlayBegin);
	}

	if (load_data(argv[1]) != 0)
	{
		printf("Error loading data\n");
		return -1;
	}

	faudio_setup();
	play();

	return 0;
}
