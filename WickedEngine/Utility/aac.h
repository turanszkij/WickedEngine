// Helper functions for extracting AAC audio stream from MP4 frame data
//	This is not using any includes or memory allocations

#ifndef AAC_H
#define AAC_H

namespace aac
{
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
		if (!read_bits(data, size, bit, 5, audio_object_type))
			return false;
		if (audio_object_type == 31)
		{
			unsigned extension = 0;
			if (!read_bits(data, size, bit, 6, extension))
				return false;
			audio_object_type = 32 + extension;
		}
		return true;
	}
	constexpr unsigned nearest_aac_sample_rate_inndex(unsigned sample_rate)
	{
		constexpr unsigned rates[] = { 96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350 };
		unsigned best = 4;
		unsigned best_diff = 0xFFFFFFFFu;
		for (unsigned i = 0; i < 13; ++i)
		{
			const unsigned diff = sample_rate > rates[i] ? sample_rate - rates[i] : rates[i] - sample_rate;
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
		if (!read_bits(data, size, bit, 4, index))
			return false;
		if (index == 0x0F)
		{
			unsigned rate = 0;
			if (!read_bits(data, size, bit, 24, rate))
				return false;
			index = nearest_aac_sample_rate_inndex(rate);
		}
		return index < 13;
	}
	struct Header
	{
		unsigned char values[7] = {};
	};
	constexpr void write_adts_header(void* dst, unsigned audio_object_type, unsigned sample_rate_index, unsigned channels, unsigned aac_bytes)
	{
		Header h;
		const unsigned profile = audio_object_type > 0 ? (audio_object_type - 1) & 3 : 1;
		const unsigned length = aac_bytes + 7;
		h.values[0] = 0xFF;
		h.values[1] = 0xF1;
		h.values[2] = (unsigned char)((profile << 6) | ((sample_rate_index & 0x0F) << 2) | ((channels >> 2) & 0x01));
		h.values[3] = (unsigned char)(((channels & 0x03) << 6) | ((length >> 11) & 0x03));
		h.values[4] = (unsigned char)((length >> 3) & 0xFF);
		h.values[5] = (unsigned char)(((length & 0x07) << 5) | 0x1F);
		h.values[6] = 0xFC;
		*(Header*)dst = h;
	}
}

#endif // AAC_H
