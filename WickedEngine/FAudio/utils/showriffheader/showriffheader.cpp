#include <stdio.h>
#include <SDL.h>

#include <FAudio.h>

#include <iostream>
#include <iomanip>
#include <sstream>

char printable_char (char c)
{
	return (std::isprint(c) ? c : ' ');
}
std::string uint8_to_charstr(const uint8_t x)
{
	std::stringstream ss;
	ss << "|"
	   << printable_char(static_cast<char>(x))
	   << "   |"
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned int>(x) << "         |"
	   << std::setfill(' ') << std::setw(8)
	   << "0x"
	   << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned int>(x)
	   << "|"
	   << std::setfill(' ') << std::setw(10)
	   << std::dec << static_cast<unsigned int>(x)
	   << "|";
	return ss.str();
}
std::string uint16_to_charstr(const uint16_t x)
{
	uint8_t buf[2];
	buf[0] = static_cast<uint8_t> (x >>  8);
	buf[1] = static_cast<uint8_t> (x >>  0);
	std::stringstream ss;
	ss << "|"
	   << printable_char(static_cast<char>(buf[1]))
	   << printable_char(static_cast<char>(buf[0]))
	   << "  |"
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[1]) << "."
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[0]) << "      |"
	   << std::setfill(' ') << std::setw(6)
	   << "0x"
	   << std::setfill('0') << std::setw(4)
	   << x
	   << "|"
	   << std::setfill(' ') << std::setw(10)
	   << std::dec << x
	   << "|";
	return ss.str();
}
std::string uint32_to_charstr(const uint32_t x)
{
	uint8_t buf[4];
	buf[0] = static_cast<uint8_t> (x >> 24);
	buf[1] = static_cast<uint8_t> (x >> 16);
	buf[2] = static_cast<uint8_t> (x >>  8);
	buf[3] = static_cast<uint8_t> (x >>  0);
	std::stringstream ss;
	ss << "|"
	   << printable_char(static_cast<char>(buf[3]))
	   << printable_char(static_cast<char>(buf[2]))
	   << printable_char(static_cast<char>(buf[1]))
	   << printable_char(static_cast<char>(buf[0]))
	   << "|"
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[3]) << "."
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[2]) << "."
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[1]) << "."
	   << std::hex << std::setfill('0') << std::setw(2)
	   << static_cast<unsigned>(buf[0]) << "|"
	   << std::setfill('0') << std::setw(2)
	   << "0x"
	   << std::setfill('0') << std::setw(8)
	   << x
	   << "|"
	   << std::setfill(' ') << std::setw(10)
	   << std::dec << x
	   << "|";
	return ss.str();
}

const char* audio_format_str(const uint16_t format)
{
	if (format == FAUDIO_FORMAT_PCM)              //      1
		return "FAUDIO_FORMAT_PCM";
	if (format == FAUDIO_FORMAT_MSADPCM)          //      2
		return "FAUDIO_FORMAT_MSADPCM";
	if (format == FAUDIO_FORMAT_IEEE_FLOAT)       //      3
		return "FAUDIO_FORMAT_IEEE_FLOAT";
	if (format == FAUDIO_FORMAT_WMAUDIO2)         // 0x0161
		return "FAUDIO_FORMAT_WMAUDIO2";
	if (format == FAUDIO_FORMAT_WMAUDIO3)         // 0x0162
		return "FAUDIO_FORMAT_WMAUDIO3";
	if (format == FAUDIO_FORMAT_WMAUDIO_LOSSLESS) // 0x0163
		return "FAUDIO_FORMAT_WMAUDIO_LOSSLESS";
	if (format == FAUDIO_FORMAT_XMAUDIO2)         // 0x0166
		return "FAUDIO_FORMAT_XMAUDIO2";
	if (format == FAUDIO_FORMAT_EXTENSIBLE)       // 0xFFE
		return "FAUDIO_FORMAT_";
	return "FAUDIO_FORMAT_UNKNOWN";
}

/* based on https://docs.microsoft.com/en-us/windows/desktop/xaudio2/how-to--load-audio-data-files-in-xaudio2 */
#define fourccRIFF *((uint32_t *) "RIFF")
#define fourccDATA *((uint32_t *) "data")
#define fourccFMT  *((uint32_t *) "fmt ")
#define fourccWAVE *((uint32_t *) "WAVE")
#define fourccXWMA *((uint32_t *) "XWMA")
#define fourccDPDS *((uint32_t *) "dpds")

void print_sub_chunk(FILE *hFile, uint32_t &chunkID, uint32_t &dwChunkPosition)
{ /* data sub-chunk - 8 bytes + data */
	uint32_t chunkSize;
	if (fread(&chunkID, sizeof(uint32_t), 1, hFile) < 1)
	{
		if (feof(hFile))
		{
			std::cout << "reached end of file at: " << dwChunkPosition << std::endl;
			return;
		}
		throw std::runtime_error("can't read chunkID");
	}
	dwChunkPosition += sizeof (uint32_t);
	if (chunkID == fourccDATA)
	{
		std::cout << "data chunk position: " << dwChunkPosition - sizeof(uint32_t) << std::endl;
		std::cout << "data:             "
				  << uint32_to_charstr(chunkID) << std::endl;
		if (fread(&chunkSize, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read chunkSize");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "data chunkSize:   "
				  << uint32_to_charstr(chunkSize) << std::endl;
		// skip the rest
		fseek(hFile, chunkSize, SEEK_CUR);
		dwChunkPosition += chunkSize;
	}
	else if (chunkID == fourccDPDS)
	{
		std::cout << "dpds chunk position: " << dwChunkPosition - sizeof(uint32_t) << std::endl;
		std::cout << "dpds:             "
				  << uint32_to_charstr(chunkID) << std::endl;
		if (fread(&chunkSize, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read chunkSize");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "dpds chunkSize:   "
				  << uint32_to_charstr(chunkSize) << std::endl;
		// skip the rest
		fseek(hFile, chunkSize, SEEK_CUR);
		dwChunkPosition += chunkSize;
	}
	else
	{
		std::cout << "unknown chunkID at position: " << dwChunkPosition - sizeof(uint32_t) << std::endl;
		std::cout << "unhandled chunk:  "
				  << uint32_to_charstr(chunkID) << std::endl;
	}
}

uint32_t load_data(const char *filename)
{
	/* open the audio file */
	FILE *hFile = fopen(filename, "rb");
	if (!hFile)
		return 1;

	fseek(hFile, 0, SEEK_SET);

	uint32_t chunkID;
	uint32_t dwChunkPosition = 0;

	// search for 'RIFF' chunk
	bool foundRIFF = false;
	while (fread(&chunkID, sizeof(uint32_t), 1, hFile) > 0)
	{
		dwChunkPosition += sizeof (uint32_t);
		if (chunkID == fourccRIFF)
		{
			foundRIFF = true;
			std::cout << "found 'RIFF' at: " << dwChunkPosition - sizeof(uint32_t) << std::endl;
			break;
		}
	}
	if (!foundRIFF)
	{
		std::cout << "missing RIFF header" << std::endl;
		return 1;
	}

	{
		std::cout << "RIFF:             "
				  << uint32_to_charstr(chunkID) << std::endl;
		uint32_t filesize;
		uint32_t format;
		if (fread(&filesize, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read RIFF filesize");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "ChunkSize:        "
				  << uint32_to_charstr(filesize) << std::endl;
		if (fread(&format, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read RIFF format");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "Format:           "
				  << uint32_to_charstr(format) << std::endl;
	}
	uint32_t fmt_chunk_start = dwChunkPosition;
	uint32_t fmt_chunk_size  = 0;
	{ /* fmt sub-chunk 24 */
		if (fread(&chunkID, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt chunkID");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "fmt:              "
				  << uint32_to_charstr(chunkID) << std::endl;
		if (chunkID != fourccFMT)
		{
			std::cout << "expected chunkID 'fmt '" << std::endl;
			return 1;
		}
		if (fread(&fmt_chunk_size, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  chunkSize");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "ChunkSize:        "
				  << uint32_to_charstr(fmt_chunk_size) << std::endl;
		uint16_t audio_format;
		if (fread(&audio_format, sizeof(uint16_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  AudioFormat");
		dwChunkPosition += sizeof (uint16_t);
		std::cout << "fmt AudioFormat:  "
				  << uint16_to_charstr(audio_format)
				  << " " << audio_format_str(audio_format) << std::endl;
		uint16_t NumChannels;
		if (fread(&NumChannels, sizeof(uint16_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  NumChannels");
		dwChunkPosition += sizeof (uint16_t);
		std::cout << "fmt NumChannels:  "
				  << uint16_to_charstr(NumChannels) << std::endl;
		uint32_t SampleRate;
		if (fread(&SampleRate, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  SampleRate");
		std::cout << "fmt SampleRate:   "
				  << uint32_to_charstr(SampleRate) << std::endl;
		uint32_t ByteRate;
		if (fread(&ByteRate, sizeof(uint32_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  ByteRate");
		dwChunkPosition += sizeof (uint32_t);
		std::cout << "fmt ByteRate:     "
				  << uint32_to_charstr(ByteRate) << std::endl;
		uint16_t BloackAlign;
		if (fread(&BloackAlign, sizeof(uint16_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  BloackAlign");
		dwChunkPosition += sizeof (uint16_t);
		std::cout << "fmt BloackAlign:  "
				  << uint16_to_charstr(BloackAlign) << std::endl;
		uint16_t BitsPerSample;
		if (fread(&BitsPerSample, sizeof(uint16_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt  BitsPerSample");
		dwChunkPosition += sizeof (uint16_t);
		std::cout << "fmt BitsPerSample:"
				  << uint16_to_charstr(BitsPerSample) << std::endl;
	}
	/* in case of extensible audio format write the additional data to the file */
	if (fmt_chunk_size > 16)
	{
		std::cout << "fmt chunk position: " << dwChunkPosition - fmt_chunk_start << std::endl;
		uint16_t cb_size;
		if (fread(&cb_size, sizeof(uint16_t), 1, hFile) < 1)
			throw std::runtime_error("can't read fmt cbSize");
		dwChunkPosition += sizeof (uint16_t);
		std::cout << "fmt cbSize:       "
				  << uint32_to_charstr(cb_size) << std::endl;
		if (cb_size >= 22)
		{
			uint16_t ValidBitsPerSample;
			if (fread(&ValidBitsPerSample, sizeof(uint16_t), 1, hFile) < 1)
				throw std::runtime_error("can't read fmt ex ValidBitsPerSample");
			dwChunkPosition += sizeof (uint16_t);
			std::cout << "fmt ValidBitsPerS:"
					  << uint16_to_charstr(ValidBitsPerSample) << std::endl;
			uint32_t dwChannelMask;
			if (fread(&dwChannelMask, sizeof(uint32_t), 1, hFile) < 1)
				throw std::runtime_error("can't read fmt ex dwChannelMask");
			dwChunkPosition += sizeof (uint32_t);
			std::cout << "fmt dwChannelMask:"
					  << uint32_to_charstr(dwChannelMask) << std::endl;
			uint32_t Data1;
			if (fread(&Data1, sizeof(uint32_t), 1, hFile) < 1)
				throw std::runtime_error("can't read fmt ex Data1");
			dwChunkPosition += sizeof (uint32_t);
			std::cout << "fmt Data1:        "
					  << uint32_to_charstr(Data1) << std::endl;
			uint16_t Data2;
			if (fread(&Data2, sizeof(uint16_t), 1, hFile) < 1)
				throw std::runtime_error("can't read fmt ex Data2");
			dwChunkPosition += sizeof (uint16_t);
			std::cout << "fmt Data2:        "
					  << uint16_to_charstr(Data2) << std::endl;
			uint16_t Data3;
			if (fread(&Data3, sizeof(uint16_t), 1, hFile) < 1)
				throw std::runtime_error("can't read fmt ex Data3");
			dwChunkPosition += sizeof (uint16_t);
			std::cout << "fmt Data3:        "
					  << uint16_to_charstr(Data3) << std::endl;
			uint8_t Data4[8];
			if (fread(&Data4, sizeof(uint8_t), 8, hFile) < 1)
				throw std::runtime_error("can't read fmt ex Data4");
			dwChunkPosition += sizeof (uint8_t)*8;
			for (uint8_t i=0; i<8; i++)
			{
				std::cout << "fmt Data4["<<static_cast<uint16_t>(i)<<"]:     "
						  << uint8_to_charstr(Data4[i]) << std::endl;
			}
			uint16_t remaining_fmt_data = cb_size - 22;
			uint8_t dummy;
			std::cout << "fmt remaining data: " << remaining_fmt_data << std::endl;
			for (uint16_t i=0; i<remaining_fmt_data; i++)
			{
				if (fread(&dummy, sizeof(uint8_t), 1, hFile) < 1)
					throw std::runtime_error("can't read fmt ex padding");
				dwChunkPosition += sizeof (uint8_t);
				std::cout << "fmt dummy["<<i<<"]:     "
						  << uint8_to_charstr(dummy) << std::endl;

			}
		} // end if cbSize > 22
		std::cout << "fmt chunk position: " << dwChunkPosition - fmt_chunk_start << std::endl;
	}
	// ignore data until we find sub-chunk data

	if (!feof(hFile))
		print_sub_chunk(hFile, chunkID, dwChunkPosition);
	if (!feof(hFile))
		print_sub_chunk(hFile, chunkID, dwChunkPosition);

	return 0;
}

int main(int argc, char *argv[]) {
	
	/* process command line arguments. This is just a test program, didn't go too nuts with validation. */
	if (argc < 2) {
		printf("Usage: %s filename\n", argv[0]);
		printf(" - filename (required): can be either a WAV or xWMA audio file.\n");
		return -1;
	}

	if (load_data(argv[1]) != 0)
	{
		printf("Error loading data\n");
		return -1;
	}

	printf("success\n");
	return 0;
}
