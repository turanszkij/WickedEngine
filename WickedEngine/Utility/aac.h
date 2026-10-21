// Helper functions for extracting AAC audio stream from MP4 frame data
//	This is not using any includes or memory allocations

#ifndef AAC_H
#define AAC_H

namespace aac
{
	// ISO/IEC 14496-3 audio object types used while reading AudioSpecificConfig.
	enum AUDIO_OBJECT_TYPE
	{
		AUDIO_OBJECT_TYPE_NULL = 0,
		AUDIO_OBJECT_TYPE_AAC_LC = 2,
		AUDIO_OBJECT_TYPE_SBR = 5,
		AUDIO_OBJECT_TYPE_PS = 29,
		AUDIO_OBJECT_TYPE_ESCAPE = 31,
	};
	// Field widths and limits from AudioSpecificConfig and the ADTS header.
	enum LIMIT
	{
		MIN_AUDIO_SPECIFIC_CONFIG_BYTES = 2,
		AUDIO_OBJECT_TYPE_BITS = 5,
		AUDIO_OBJECT_TYPE_EXT_BITS = 6,
		AUDIO_OBJECT_TYPE_EXT_BASE = 32,
		MAX_ADTS_AUDIO_OBJECT_TYPE = 4,
		SAMPLING_FREQUENCY_INDEX_BITS = 4,
		SAMPLING_FREQUENCY_INDEX_EXPLICIT = 0x0F,
		SAMPLING_FREQUENCY_EXPLICIT_BITS = 24,
		SAMPLING_FREQUENCY_INDEX_COUNT = 13,
		SAMPLING_FREQUENCY_INDEX_44100 = 4,
		CHANNEL_CONFIGURATION_BITS = 4,
		CHANNEL_CONFIGURATION_AOT_SPECIFIC = 0,
		CHANNEL_CONFIGURATION_STEREO = 2,
		MAX_ADTS_CHANNEL_CONFIGURATION = 7,
		ADTS_HEADER_BYTES = 7,
		MAX_ADTS_FRAME_LENGTH = 0x1FFF,
	};

	constexpr bool read_bits(const unsigned char* data, unsigned size, unsigned& bit, unsigned count, unsigned& value)
	{
		value = 0;
		for (unsigned i = 0; i < count; ++i)
		{
			if ((bit >> 3) >= size)
				return false;
			value = (value << 1) | ((data[bit >> 3] >> (7 - (bit & 7))) & 1);
			++bit;
		}
		return true;
	}
	constexpr bool read_audio_object_type(const unsigned char* data, unsigned size, unsigned& bit, unsigned& audio_object_type)
	{
		if (!read_bits(data, size, bit, AUDIO_OBJECT_TYPE_BITS, audio_object_type))
			return false;
		if (audio_object_type == AUDIO_OBJECT_TYPE_ESCAPE)
		{
			unsigned extension = 0;
			if (!read_bits(data, size, bit, AUDIO_OBJECT_TYPE_EXT_BITS, extension))
				return false;
			audio_object_type = AUDIO_OBJECT_TYPE_EXT_BASE + extension;
		}
		return true;
	}
	constexpr unsigned sample_rates[] = { 96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0 };
	constexpr unsigned nearest_aac_sample_rate_inndex(unsigned sample_rate)
	{
		unsigned best = SAMPLING_FREQUENCY_INDEX_44100;
		unsigned best_diff = 0xFFFFFFFFu;
		for (unsigned i = 0; i < SAMPLING_FREQUENCY_INDEX_COUNT; ++i)
		{
			const unsigned diff = sample_rate > sample_rates[i] ? sample_rate - sample_rates[i] : sample_rates[i] - sample_rate;
			if (diff < best_diff)
			{
				best_diff = diff;
				best = i;
			}
		}
		return best;
	}
	constexpr bool read_sample_rate_index(const unsigned char* data, unsigned size, unsigned& bit, unsigned& index)
	{
		if (!read_bits(data, size, bit, SAMPLING_FREQUENCY_INDEX_BITS, index))
			return false;
		if (index == SAMPLING_FREQUENCY_INDEX_EXPLICIT)
		{
			unsigned rate = 0;
			if (!read_bits(data, size, bit, SAMPLING_FREQUENCY_EXPLICIT_BITS, rate))
				return false;
			index = nearest_aac_sample_rate_inndex(rate);
		}
		return index < SAMPLING_FREQUENCY_INDEX_COUNT;
	}

	constexpr bool parse(const unsigned char* data, unsigned size, unsigned& audio_object_type, unsigned& sample_rate_index, unsigned& channels)
	{
		audio_object_type = 0;
		sample_rate_index = 0;
		channels = 0;

		unsigned bit = 0;
		if (!aac::read_audio_object_type(data, size, bit, audio_object_type) ||
			!aac::read_sample_rate_index(data, size, bit, sample_rate_index) ||
			!aac::read_bits(data, size, bit, aac::CHANNEL_CONFIGURATION_BITS, channels))
		{
			return false;
		}
		if (audio_object_type == aac::AUDIO_OBJECT_TYPE_SBR || audio_object_type == aac::AUDIO_OBJECT_TYPE_PS)
		{
			uint32_t extension_rate_index = 0;
			uint32_t core_object_type = 0;
			if (!aac::read_sample_rate_index(data, size, bit, extension_rate_index) ||
				!aac::read_audio_object_type(data, size, bit, core_object_type))
			{
				return false;
			}
			audio_object_type = core_object_type;
		}
		if (audio_object_type == aac::AUDIO_OBJECT_TYPE_NULL || audio_object_type > aac::MAX_ADTS_AUDIO_OBJECT_TYPE)
			audio_object_type = aac::AUDIO_OBJECT_TYPE_AAC_LC;
		if (channels == aac::CHANNEL_CONFIGURATION_AOT_SPECIFIC || channels > aac::MAX_ADTS_CHANNEL_CONFIGURATION)
			channels = aac::CHANNEL_CONFIGURATION_STEREO;
		return true;
	}

	struct ADTSHeader
	{
		unsigned char values[ADTS_HEADER_BYTES] = {};

		constexpr unsigned get_channel_count() const
		{
			return ((values[2] & 0x01) << 2) | ((values[3] >> 6) & 0x03);
		}
		constexpr unsigned get_sample_rate_index() const
		{
			return (values[2] >> 2) & 0x0F;
		}
		constexpr unsigned get_sample_rate() const
		{
			const unsigned sfi = get_sample_rate_index();
			return sfi < 16 ? sample_rates[sfi] : 0;
		}
	};
	constexpr void write_adts_header(void* dst, unsigned audio_object_type, unsigned sample_rate_index, unsigned channels, unsigned aac_bytes)
	{
		ADTSHeader h;
		const unsigned profile = audio_object_type > AUDIO_OBJECT_TYPE_NULL ? (audio_object_type - 1) & 3 : 1;
		const unsigned length = aac_bytes + ADTS_HEADER_BYTES;
		h.values[0] = 0xFF;
		h.values[1] = 0xF1;
		h.values[2] = (unsigned char)((profile << 6) | ((sample_rate_index & 0x0F) << 2) | ((channels >> 2) & 0x01));
		h.values[3] = (unsigned char)(((channels & 0x03) << 6) | ((length >> 11) & 0x03));
		h.values[4] = (unsigned char)((length >> 3) & 0xFF);
		h.values[5] = (unsigned char)(((length & 0x07) << 5) | 0x1F);
		h.values[6] = 0xFC;
		*(ADTSHeader*)dst = h;
	}

	constexpr bool is_adts(const unsigned char* data, unsigned long long size)
	{
		if (data == nullptr || size < 7)
			return false;
		if (data[0] != 0xFF || (data[1] & 0xF6) != 0xF0)
			return false;
		const unsigned frame_length = ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xE0) >> 5);
		if (frame_length < 7 || frame_length > size)
			return false;
		if (frame_length == size)
			return true;
		return data[frame_length] == 0xFF && (data[frame_length + 1] & 0xF6) == 0xF0;
	}
}

#endif // AAC_H
