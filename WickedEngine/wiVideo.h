#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiVector.h"

#include <memory>
#include <string>

namespace wi::video
{
	struct Video
	{
		uint32_t padded_width = 0;
		uint32_t padded_height = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bit_rate = 0;
		wi::graphics::VideoProfile profile = wi::graphics::VideoProfile::H264;
		wi::vector<uint8_t> sps_datas;
		wi::vector<uint8_t> pps_datas;
		uint32_t sps_count = 0;
		uint32_t pps_count = 0;
		wi::graphics::GPUBuffer data_stream;
		float average_frames_per_second = 0;
		float duration_seconds = 0;
		struct FrameInfo
		{
			uint64_t offset = 0;
			uint64_t size = 0;
			float timestamp_seconds = 0;
			float duration_seconds = 0;
		};
		wi::vector<FrameInfo> frames_infos;
		inline bool IsValid() const { return data_stream.IsValid(); }
	};

	struct VideoInstance
	{
		const Video* video = nullptr;
		wi::graphics::VideoDecoder decoder;
		wi::graphics::Texture output_rgb;
		uint64_t current_frame = 0;
		float time_until_next_frame = 0;
		enum class State
		{
			Paused,
			Playing,
		} state = State::Playing;
		enum class Flags
		{
			Empty = 0,
			Looped = 1 << 0,
			Mipmapped = 1 << 1,
			NeedsResolve = 1 << 2,
		};
		Flags flags = Flags::Empty;
		inline bool IsValid() const { return decoder.IsValid(); }
	};

	bool CreateVideo(const std::string& filename, Video* video);
	bool CreateVideo(const uint8_t* filedata, size_t filesize, Video* video);
	bool CreateVideoInstance(const Video* video, VideoInstance* instance);

	bool IsDecodingRequired(VideoInstance* instances, size_t num_instances, float dt);
	void UpdateVideo(VideoInstance* instance, float dt, wi::graphics::CommandList cmd);
	void ResolveVideoToRGB(VideoInstance* instance, wi::graphics::CommandList cmd);
}

template<>
struct enable_bitmask_operators<wi::video::VideoInstance::Flags> {
	static const bool enable = true;
};
