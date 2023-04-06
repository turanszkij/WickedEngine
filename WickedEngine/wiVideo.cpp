#include "wiVideo.h"
#include "wiHelper.h"

#include "Utility/minimp4.h"
#include "Utility/h264.h"

namespace wi::video
{

	bool CreateVideo(const std::string& filename, Video* video)
	{
		wi::vector<uint8_t> filedata;
		if (!wi::helper::FileRead(filename, filedata))
			return false;
		return CreateVideo(filedata.data(), filedata.size(), video);
	}
	bool CreateVideo(const uint8_t* filedata, size_t filesize, Video* video)
	{
		bool success = false;
		uint8_t* input_buf = (uint8_t*)filedata;
		struct INPUT_BUFFER
		{
			uint8_t* buffer;
			size_t size;
		} input_buffer = { input_buf,filesize };
		MP4D_demux_t mp4 = {};
		int token = 0;
		auto read_callback = [](int64_t offset, void* buffer, size_t size, void* token) -> int
		{
			INPUT_BUFFER* buf = (INPUT_BUFFER*)token;
			size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
			std::memcpy(buffer, buf->buffer + offset, to_copy);
			return to_copy != size;
		};
		int result = MP4D_open(&mp4, read_callback, &input_buffer, (int64_t)filesize);
		if (result == 1)
		{
			for (uint32_t ntrack = 0; ntrack < mp4.track_count; ntrack++)
			{
				MP4D_track_t& track = mp4.track[ntrack];
				if (track.handler_type == MP4D_HANDLER_TYPE_VIDE)
				{
					// SPS:
					video->sps_datas.clear();
					video->sps_count = 0;
					{
						int size = 0;
						int index = 0;
						const void* data = nullptr;
						while (data = MP4D_read_sps(&mp4, ntrack, index, &size))
						{
							const uint8_t* sps_data = (const uint8_t*)data;
							assert(sps_data[0] == 0x67); // verify SPS start code prefix
							sps_data++;

							h264::sps_t sps = {};
							bs_t bs = {};
							bs_init(&bs, (uint8_t*)sps_data, size);
							h264::read_seq_parameter_set_rbsp(&sps, &bs);

							// Some validation checks that data parsing returned expected values:
							//	https://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream
							uint32_t width = ((sps.pic_width_in_mbs_minus1 + 1) * 16) - sps.frame_crop_left_offset * 2 - sps.frame_crop_right_offset * 2;
							uint32_t height = ((2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 + 1) * 16) - (sps.frame_crop_top_offset * 2) - (sps.frame_crop_bottom_offset * 2);
							assert(track.SampleDescription.video.width == width);
							assert(track.SampleDescription.video.height == height);

							video->sps_datas.resize(video->sps_datas.size() + sizeof(sps));
							std::memcpy((h264::sps_t*)video->sps_datas.data() + video->sps_count, &sps, sizeof(sps));
							video->sps_count++;
							index++;
						}
					}

					// PPS:
					video->pps_datas.clear();
					video->pps_count = 0;
					{
						int size = 0;
						int index = 0;
						const void* data = nullptr;
						while (data = MP4D_read_pps(&mp4, ntrack, index, &size))
						{
							const uint8_t* pps_data = (const uint8_t*)data;
							assert(pps_data[0] == 0x68); // verify PPS start code prefix
							pps_data++;

							h264::pps_t pps = {};
							bs_t bs = {};
							bs_init(&bs, (uint8_t*)pps_data, size);
							h264::read_pic_parameter_set_rbsp(&pps, &bs);
							video->pps_datas.resize(video->pps_datas.size() + sizeof(pps));
							std::memcpy((h264::pps_t*)video->pps_datas.data() + video->pps_count, &pps, sizeof(pps));
							video->pps_count++;
							index++;
						}
					}

					switch (track.object_type_indication)
					{
					case MP4_OBJECT_TYPE_AVC:
						video->profile = wi::graphics::VideoProfile::H264;
						break;
					case MP4_OBJECT_TYPE_HEVC:
						//video->profile = wi::graphics::VideoProfile::H265;
						assert(0); // not implemented
						break;
					default:
						assert(0); // not implemented
						break;
					}

					video->width = track.SampleDescription.video.width;
					video->height = track.SampleDescription.video.height;
					video->bit_rate = track.avg_bitrate_bps;

					double timescale_rcp = 1.0 / double(track.timescale);

					unsigned frame_bytes, timestamp, duration;
					MP4D_file_offset_t first_sample_offset = MP4D_frame_offset(&mp4, ntrack, 0, &frame_bytes, &timestamp, &duration);

					wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();
					const uint64_t alignment = device->GetVideoDecodeBitstreamAlignment();

					video->frames_infos.reserve(track.sample_count);
					uint32_t track_duration = 0;
					uint64_t aligned_size = 0;
					for (uint32_t i = 0; i < track.sample_count; i++)
					{
						MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
						track_duration += duration;
						Video::FrameInfo& info = video->frames_infos.emplace_back();
						info.offset = aligned_size;
						info.size = wi::graphics::AlignTo((uint64_t)frame_bytes, alignment);
						aligned_size += info.size;
						info.timestamp_seconds = float(double(timestamp) * timescale_rcp);
						info.duration_seconds = float(double(duration) * timescale_rcp);
					}

					video->average_frames_per_second = float(double(track.timescale) / double(track_duration) * track.sample_count);
					video->duration_seconds = float(double(track_duration) * timescale_rcp);

#define USE_SHORT_SYNC 0
					char sync[4] = { 0, 0, 0, 1 };

					auto copy_video_track = [&](void* dest) {
						for (uint32_t i = 0; i < track.sample_count; i++)
						{
							MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
							uint8_t* dest_buffer = (uint8_t*)dest + video->frames_infos[i].offset;
							std::memcpy(dest_buffer, input_buf + ofs, frame_bytes);
							//uint8_t* mem = input_buf + ofs;
							//while (frame_bytes)
							//{
							//	uint32_t size = ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) | ((uint32_t)mem[2] << 8) | mem[3];
							//	size += 4;
							//	mem[0] = 0; mem[1] = 0; mem[2] = 0; mem[3] = 1;
							//	std::memcpy(dest_buffer, mem, size - USE_SHORT_SYNC);
							//	//fwrite(mem + USE_SHORT_SYNC, 1, size - USE_SHORT_SYNC, fout);
							//	if (frame_bytes < size)
							//	{
							//		//printf("error: demux sample failed\n");
							//		//exit(1);
							//		assert(0);
							//	}
							//	frame_bytes -= size;
							//	mem += size;
							//	dest_buffer += size;
							//}
						}
					};

					wi::graphics::GPUBufferDesc bd;
					bd.size = aligned_size;
					bd.usage = wi::graphics::Usage::DEFAULT;
					bd.misc_flags = wi::graphics::ResourceMiscFlag::VIDEO_DECODE_SRC;
					success = device->CreateBuffer2(&bd, copy_video_track, &video->data_stream);
					assert(success);
					device->SetName(&video->data_stream, "wi::Video::data_stream");
				}
				else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN)
				{

				}
			}
		}
		MP4D_close(&mp4);
		return success;
	}
	bool CreateVideoInstance(const Video* video, VideoInstance* instance)
	{
		instance->video = video;

		wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();

		wi::graphics::VideoDesc vd;
		vd.width = video->width;
		vd.height = video->height;
		vd.bit_rate = video->bit_rate;
		vd.format = wi::graphics::Format::NV12;
		vd.profile = video->profile;
		vd.pps_datas = instance->video->pps_datas.data();
		vd.pps_count = instance->video->pps_count;
		vd.sps_datas = instance->video->sps_datas.data();
		vd.sps_count = instance->video->sps_count;
		bool success = device->CreateVideoDecoder(&vd, &instance->decoder);
		assert(success);
		if (!success)
			return false;

		wi::graphics::TextureDesc td;
		td.width = vd.width;
		td.height = vd.height;
		td.mip_levels = 1;
		td.format = vd.format;
		td.bind_flags = wi::graphics::BindFlag::SHADER_RESOURCE;
		//td.misc_flags = wi::graphics::ResourceMiscFlag::VIDEO_DECODE_DST;
		success = device->CreateTexture(&td, nullptr, &instance->output);
		assert(success);
		device->SetName(&instance->output, "wi::VideoInstance::output");
		wi::graphics::Format format = wi::graphics::Format::R8G8_UNORM;
		wi::graphics::ImageAspect aspect = wi::graphics::ImageAspect::CHROMINANCE;
		instance->output_subresource_chroma = device->CreateSubresource(&instance->output, wi::graphics::SubresourceType::SRV, 0, 1, 0, 1, &format, &aspect);
		assert(instance->output_subresource_chroma >= 0);
		return success;
	}

	bool IsDecodingRequired(VideoInstance* instances, size_t num_instances, float dt)
	{
		for (size_t i = 0; i < num_instances; ++i)
		{
			VideoInstance& instance = instances[i];
			if (instance.state == VideoInstance::State::Playing && instance.time_until_next_frame - dt <= 0)
				return true;
		}
		return false;
	}
	void UpdateVideo(VideoInstance* instance, float dt, wi::graphics::CommandList cmd)
	{
#if 0
		instance->current_frame = 0;
#else
		if (instance->state == VideoInstance::State::Paused)
			return;
		instance->time_until_next_frame -= dt;
		if (instance->time_until_next_frame > 0)
			return;
		if (instance->current_frame >= (int)instance->video->frames_infos.size() - 1)
		{
			if (has_flag(instance->flags, VideoInstance::Flags::Looped))
			{
				instance->current_frame = 0;
			}
			else
			{
				instance->state = VideoInstance::State::Paused;
				instance->current_frame = (int)instance->video->frames_infos.size() - 1;
			}
		}
#endif

		const Video::FrameInfo& frame_info = instance->video->frames_infos[instance->current_frame];
		instance->time_until_next_frame = frame_info.duration_seconds;

		wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();

		wi::graphics::VideoDecodeOperation decode_operation;
		if (instance->current_frame == 0)
		{
			decode_operation.flags = wi::graphics::VideoDecodeOperation::FLAG_SESSION_RESET;
		}
		decode_operation.stream = &instance->video->data_stream;
		decode_operation.stream_offset = frame_info.offset;
		decode_operation.stream_size = frame_info.size;
		decode_operation.output = &instance->output;
		decode_operation.frame_index = instance->current_frame;
		device->VideoDecode(&instance->decoder, &decode_operation, cmd);

		instance->current_frame++;
	}
}
