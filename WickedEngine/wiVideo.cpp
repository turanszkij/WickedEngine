#include "wiVideo.h"
#include "wiHelper.h"
#include "wiRenderer.h"

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
			video->title = {};
			video->album = {};
			video->artist = {};
			video->year = {};
			video->comment = {};
			video->genre = {};
			if (mp4.tag.title != nullptr)
			{
				video->title = (char*)mp4.tag.title;
			}
			if (mp4.tag.album != nullptr)
			{
				video->album = (char*)mp4.tag.album;
			}
			if (mp4.tag.artist != nullptr)
			{
				video->artist = (char*)mp4.tag.artist;
			}
			if (mp4.tag.year != nullptr)
			{
				video->year = (char*)mp4.tag.year;
			}
			if (mp4.tag.comment != nullptr)
			{
				video->comment = (char*)mp4.tag.comment;
			}
			if (mp4.tag.genre != nullptr)
			{
				video->genre = (char*)mp4.tag.genre;
			}

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
							bs_t bs = {};
							bs_init(&bs, (uint8_t*)sps_data, size);
							h264::nal_header nal = {};
							h264::read_nal_header(&nal, &bs);
							assert(nal.type = h264::NAL_UNIT_TYPE_SPS);

							h264::sps_t sps = {};
							h264::read_seq_parameter_set_rbsp(&sps, &bs);

							// Some validation checks that data parsing returned expected values:
							//	https://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream
							uint32_t width = ((sps.pic_width_in_mbs_minus1 + 1) * 16) - sps.frame_crop_left_offset * 2 - sps.frame_crop_right_offset * 2;
							uint32_t height = ((2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 + 1) * 16) - (sps.frame_crop_top_offset * 2) - (sps.frame_crop_bottom_offset * 2);
							assert(track.SampleDescription.video.width == width);
							assert(track.SampleDescription.video.height == height);
							video->padded_width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
							video->padded_height = (sps.pic_height_in_map_units_minus1 + 1) * 16;

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
							bs_t bs = {};
							bs_init(&bs, (uint8_t*)pps_data, size);
							h264::nal_header nal = {};
							h264::read_nal_header(&nal, &bs);
							assert(nal.type = h264::NAL_UNIT_TYPE_PPS);

							h264::pps_t pps = {};
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

					wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();
					const uint64_t alignment = device->GetVideoDecodeBitstreamAlignment();

					h264::pps_t* pps_array = (h264::pps_t*)video->pps_datas.data();
					h264::sps_t* sps_array = (h264::sps_t*)video->sps_datas.data();
					int prev_pic_order_cnt_lsb = 0;
					int prev_pic_order_cnt_msb = 0;
					int poc_cycle = 0;

					video->frames_infos.reserve(track.sample_count);
					video->slice_header_datas.reserve(track.sample_count * sizeof(h264::slice_header_t));
					video->slice_header_count = track.sample_count;
					uint32_t track_duration = 0;
					uint64_t aligned_size = 0;
					for (uint32_t i = 0; i < track.sample_count; i++)
					{
						unsigned frame_bytes, timestamp, duration;
						MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
						track_duration += duration;

						Video::FrameInfo& info = video->frames_infos.emplace_back();
						info.offset = aligned_size;

						uint8_t* src_buffer = input_buf + ofs;
						while (frame_bytes > 0)
						{
							uint32_t size = ((uint32_t)src_buffer[0] << 24) | ((uint32_t)src_buffer[1] << 16) | ((uint32_t)src_buffer[2] << 8) | src_buffer[3];
							size += 4;
							assert(frame_bytes >= size);

							bs_t bs = {};
							bs_init(&bs, &src_buffer[4], frame_bytes);
							h264::nal_header nal = {};
							h264::read_nal_header(&nal, &bs);

							if (nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_IDR)
							{
								info.type = wi::graphics::VideoFrameType::Intra;
							}
							else if (nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR)
							{
								info.type = wi::graphics::VideoFrameType::Predictive;
							}
							else
							{
								// Continue search for frame beginning NAL unit:
								frame_bytes -= size;
								src_buffer += size;
								continue;
							}

							h264::slice_header_t* slice_header = (h264::slice_header_t*)video->slice_header_datas.data() + i;
							*slice_header = {};
							h264::read_slice_header(slice_header, &nal, pps_array, sps_array, &bs);

							const h264::pps_t& pps = pps_array[slice_header->pic_parameter_set_id];
							const h264::sps_t& sps = sps_array[pps.seq_parameter_set_id];

							// Rec. ITU-T H.264 (08/2021) page 77
							int max_pic_order_cnt_lsb = 1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
							int pic_order_cnt_lsb = slice_header->pic_order_cnt_lsb;

							if (pic_order_cnt_lsb == 0)
							{
								poc_cycle++;
							}

							// Rec. ITU-T H.264 (08/2021) page 115
							// Also: https://www.ramugedia.com/negative-pocs
							int pic_order_cnt_msb = 0;
							if (pic_order_cnt_lsb < prev_pic_order_cnt_lsb && (prev_pic_order_cnt_lsb - pic_order_cnt_lsb) >= max_pic_order_cnt_lsb / 2)
							{
								pic_order_cnt_msb = prev_pic_order_cnt_msb + max_pic_order_cnt_lsb; // pic_order_cnt_lsb wrapped around
							}
							else if (pic_order_cnt_lsb > prev_pic_order_cnt_lsb && (pic_order_cnt_lsb - prev_pic_order_cnt_lsb) > max_pic_order_cnt_lsb / 2)
							{
								pic_order_cnt_msb = prev_pic_order_cnt_msb - max_pic_order_cnt_lsb; // here negative POC might occur
							}
							else
							{
								pic_order_cnt_msb = prev_pic_order_cnt_msb;
							}
							//pic_order_cnt_msb = pic_order_cnt_msb % 256;
							prev_pic_order_cnt_lsb = pic_order_cnt_lsb;
							prev_pic_order_cnt_msb = pic_order_cnt_msb;

							// https://www.vcodex.com/h264avc-picture-management/
							info.poc = pic_order_cnt_msb + pic_order_cnt_lsb; // poc = TopFieldOrderCount
							info.gop = poc_cycle - 1;

							// Accept frame beginning NAL unit:
							info.reference_priority = nal.idc;
							break;
						}

						info.size = wi::graphics::AlignTo((uint64_t)frame_bytes + 4, alignment);
						aligned_size += info.size;
						info.timestamp_seconds = float(double(timestamp) * timescale_rcp);
						info.duration_seconds = float(double(duration) * timescale_rcp);
					}

					video->frame_display_order.resize(video->frames_infos.size());
					for (size_t i = 0; i < video->frames_infos.size(); ++i)
					{
						video->frame_display_order[i] = i;
					}
					std::sort(video->frame_display_order.begin(), video->frame_display_order.end(), [&](size_t a, size_t b) {
						const Video::FrameInfo& frameA = video->frames_infos[a];
						const Video::FrameInfo& frameB = video->frames_infos[b];
						int64_t prioA = (int64_t(frameA.gop) << 32ll) | int64_t(frameA.poc);
						int64_t prioB = (int64_t(frameB.gop) << 32ll) | int64_t(frameB.poc);
						return prioA < prioB;
					});
					for (size_t i = 0; i < video->frame_display_order.size(); ++i)
					{
						video->frames_infos[video->frame_display_order[i]].display_order = i;
					}

					video->average_frames_per_second = float(double(track.timescale) / double(track_duration) * track.sample_count);
					video->duration_seconds = float(double(track_duration) * timescale_rcp);

					auto copy_video_track = [&](void* dest) {
						for (uint32_t i = 0; i < track.sample_count; i++)
						{
							unsigned frame_bytes, timestamp, duration;
							MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
							uint8_t* dst_buffer = (uint8_t*)dest + video->frames_infos[i].offset;
							uint8_t* src_buffer = input_buf + ofs;
							while (frame_bytes > 0)
							{
								uint32_t size = ((uint32_t)src_buffer[0] << 24) | ((uint32_t)src_buffer[1] << 16) | ((uint32_t)src_buffer[2] << 8) | src_buffer[3];
								size += 4;
								assert(frame_bytes >= size);

								bs_t bs = {};
								bs_init(&bs, &src_buffer[4], sizeof(uint8_t));
								h264::nal_header nal = {};
								h264::read_nal_header(&nal, &bs);

								if (
									nal.type != h264::NAL_UNIT_TYPE_CODED_SLICE_IDR &&
									nal.type != h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR
									)
								{
									frame_bytes -= size;
									src_buffer += size;
									continue;
								}

								std::memcpy(dst_buffer, src_buffer, size);

								// overwrite beginning of GPU buffer with NAL sync unit:
								const uint8_t nal_sync[] = { 0,0,0,1 };
								std::memcpy(dst_buffer, nal_sync, sizeof(nal_sync));
								break;
							}
						}
					};

					wi::graphics::GPUBufferDesc bd;
					bd.size = aligned_size;
					bd.usage = wi::graphics::Usage::UPLOAD;
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
		instance->current_frame = 0;
		instance->flags &= ~VideoInstance::Flags::InitialFirstFrameDecoded;

		wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();

		wi::graphics::VideoDesc vd;
		vd.width = video->padded_width;
		vd.height = video->padded_height;
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
		td.width = video->width;
		td.height = video->height;
		td.format = wi::graphics::Format::R8G8B8A8_UNORM;
		if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
		{
			td.mip_levels = 0; // max mipcount
		}
		td.bind_flags = wi::graphics::BindFlag::UNORDERED_ACCESS | wi::graphics::BindFlag::SHADER_RESOURCE;
		td.misc_flags = wi::graphics::ResourceMiscFlag::TYPED_FORMAT_CASTING;
		td.layout = wi::graphics::ResourceState::SHADER_RESOURCE_COMPUTE;
		success = device->CreateTexture(&td, nullptr, &instance->output_rgb);
		device->SetName(&instance->output_rgb, "wi::VideoInstance::output_rgb");
		assert(success);

		if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
		{
			for (uint32_t i = 0; i < instance->output_rgb.GetDesc().mip_levels; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&instance->output_rgb, wi::graphics::SubresourceType::SRV, 0, 1, i, 1);
				assert(subresource_index == i);
				subresource_index = device->CreateSubresource(&instance->output_rgb, wi::graphics::SubresourceType::UAV, 0, 1, i, 1);
				assert(subresource_index == i);
			}
		}

		// This part must be AFTER mip level subresource creation:
		wi::graphics::Format srgb_format = wi::graphics::GetFormatSRGB(td.format);
		instance->output_srgb_subresource = device->CreateSubresource(
			&instance->output_rgb,
			wi::graphics::SubresourceType::SRV,
			0, -1,
			0, -1,
			&srgb_format
		);

		return success;
	}

	bool IsDecodingRequired(const VideoInstance* instance, float dt)
	{
		if (instance == nullptr || instance->video == nullptr)
			return false;
		if (!has_flag(instance->flags, VideoInstance::Flags::InitialFirstFrameDecoded))
			return true;
		if (has_flag(instance->flags, VideoInstance::Flags::Playing) && instance->time_until_next_frame - dt <= 0)
			return true;
		return false;
	}
	void UpdateVideo(VideoInstance* instance, float dt, wi::graphics::CommandList cmd)
	{
		if (instance == nullptr || instance->video == nullptr)
			return;

		if (has_flag(instance->flags, VideoInstance::Flags::InitialFirstFrameDecoded))
		{
			if (!has_flag(instance->flags, VideoInstance::Flags::Playing))
				return;
			instance->time_until_next_frame -= dt;
			//if (instance->time_until_next_frame > 0)
			//	return;
			if (instance->current_frame >= (int)instance->video->frames_infos.size() - 1)
			{
				if (has_flag(instance->flags, VideoInstance::Flags::Looped))
				{
					instance->current_frame = 0;
				}
				else
				{
					instance->flags &= ~VideoInstance::Flags::Playing;
					instance->current_frame = (int)instance->video->frames_infos.size() - 1;
				}
			}
		}

		if (!cmd.IsValid())
			return;

		wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();

		const Video* video = instance->video;
		const Video::FrameInfo& frame_info = video->frames_infos[instance->current_frame];
		instance->time_until_next_frame = frame_info.duration_seconds;

		wi::graphics::VideoDecodeOperation decode_operation;
		if (instance->current_frame == 0)
		{
			decode_operation.flags = wi::graphics::VideoDecodeOperation::FLAG_SESSION_RESET;
		}
		decode_operation.stream = &video->data_stream;
		decode_operation.stream_offset = frame_info.offset;
		decode_operation.stream_size = frame_info.size;
		decode_operation.frame_index = instance->current_frame;
		decode_operation.poc = frame_info.poc;
		decode_operation.display_order = frame_info.display_order;
		decode_operation.frame_type = frame_info.type;
		decode_operation.reference_priority = frame_info.reference_priority;
		decode_operation.slice_header = (h264::slice_header_t*)video->slice_header_datas.data() + instance->current_frame;
		device->VideoDecode(&instance->decoder, &decode_operation, cmd);

		instance->current_frame++;
		instance->flags |= VideoInstance::Flags::NeedsResolve;
		instance->flags |= VideoInstance::Flags::InitialFirstFrameDecoded;
	}
	void ResolveVideoToRGB(VideoInstance* instance, wi::graphics::CommandList cmd)
	{
		if (!has_flag(instance->flags, VideoInstance::Flags::NeedsResolve))
			return;
		instance->flags &= ~VideoInstance::Flags::NeedsResolve;

		wi::graphics::GraphicsDevice* device = wi::graphics::GetDevice();
		wi::graphics::GraphicsDevice::VideoDecoderResult decode_result = device->GetVideoDecoderResult(&instance->decoder);
		wi::renderer::YUV_to_RGB(
			decode_result.texture,
			decode_result.subresource_luminance,
			decode_result.subresource_chrominance,
			instance->output_rgb,
			cmd
		);

		if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
		{
			wi::renderer::GenerateMipChain(instance->output_rgb, wi::renderer::MIPGENFILTER_LINEAR, cmd);
		}
	}
}
