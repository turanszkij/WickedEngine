#include "wiVideo.h"
#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBacklog.h"

#include "Utility/minimp4.h"
#include "Utility/h264.h"

//#define DEBUG_DUMP_H264

using namespace wi::graphics;

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
		const uint8_t* input_buf = filedata;
		struct INPUT_BUFFER
		{
			const uint8_t* buffer;
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

#ifdef DEBUG_DUMP_H264
				wi::vector<uint8_t> dump;
				size_t dump_offset = 0;
#endif // DEBUG_DUMP_H264

				if (track.handler_type == MP4D_HANDLER_TYPE_VIDE)
				{
					switch (track.object_type_indication)
					{
					case MP4_OBJECT_TYPE_AVC:
						video->profile = VideoProfile::H264;
						break;
					case MP4_OBJECT_TYPE_HEVC:
						wi::helper::messageBox("H265 (HEVC) video format is not supported yet!", "Error!");
						return false;
					default:
						wi::helper::messageBox("Unknown video format!", "Error!");
						return false;
					}

					// SPS:
					video->sps_datas.clear();
					video->sps_count = 0;
					video->num_dpb_slots = 0;
					{
						int size = 0;
						int index = 0;
						const void* data = nullptr;
						while (data = MP4D_read_sps(&mp4, ntrack, index, &size))
						{
							const uint8_t* sps_data = (const uint8_t*)data;
#ifdef DEBUG_DUMP_H264
							size_t additional_dump_size = sizeof(h264::start_code) + size;
							dump.resize(dump.size() + additional_dump_size);
							std::memcpy(dump.data() + dump_offset, h264::start_code, sizeof(h264::start_code));
							std::memcpy(dump.data() + dump_offset + sizeof(h264::start_code), sps_data, size);
							dump_offset += additional_dump_size;
#endif // DEBUG_DUMP_H264

							h264::Bitstream bs = {};
							bs.init(sps_data, size);
							h264::NALHeader nal = {};
							h264::read_nal_header(&nal, &bs);
							assert(nal.type == h264::NAL_UNIT_TYPE_SPS);

							h264::SPS sps = {};
							h264::read_sps(&sps, &bs);

							// Some validation checks that data parsing returned expected values:
							//	https://stackoverflow.com/questions/6394874/fetching-the-dimensions-of-a-h264video-stream
							uint32_t width = ((sps.pic_width_in_mbs_minus1 + 1) * 16) - sps.frame_crop_left_offset * 2 - sps.frame_crop_right_offset * 2;
							uint32_t height = ((2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 + 1) * 16) - (sps.frame_crop_top_offset * 2) - (sps.frame_crop_bottom_offset * 2);
							assert(track.SampleDescription.video.width == width);
							assert(track.SampleDescription.video.height == height);
							video->padded_width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
							video->padded_height = (sps.pic_height_in_map_units_minus1 + 1) * 16;
							video->num_dpb_slots = std::max(video->num_dpb_slots, uint32_t(sps.num_ref_frames + 1));

							video->sps_datas.resize(video->sps_datas.size() + sizeof(sps));
							std::memcpy((h264::SPS*)video->sps_datas.data() + video->sps_count, &sps, sizeof(sps));
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
#ifdef DEBUG_DUMP_H264
							size_t additional_dump_size = sizeof(h264::start_code) + size;
							dump.resize(dump.size() + additional_dump_size);
							std::memcpy(dump.data() + dump_offset, h264::start_code, sizeof(h264::start_code));
							std::memcpy(dump.data() + dump_offset + sizeof(h264::start_code), pps_data, size);
							dump_offset += additional_dump_size;
#endif // DEBUG_DUMP_H264

							h264::Bitstream bs = {};
							bs.init(pps_data, size);
							h264::NALHeader nal = {};
							h264::read_nal_header(&nal, &bs);
							assert(nal.type == h264::NAL_UNIT_TYPE_PPS);

							h264::PPS pps = {};
							h264::read_pps(&pps, &bs);
							video->pps_datas.resize(video->pps_datas.size() + sizeof(pps));
							std::memcpy((h264::PPS*)video->pps_datas.data() + video->pps_count, &pps, sizeof(pps));
							video->pps_count++;
							index++;
						}
					}

					video->width = track.SampleDescription.video.width;
					video->height = track.SampleDescription.video.height;
					video->bit_rate = track.avg_bitrate_bps;

					double timescale_rcp = 1.0 / double(track.timescale);

					GraphicsDevice* device = GetDevice();
					const uint64_t alignment = device->GetVideoDecodeBitstreamAlignment();

					const h264::PPS* pps_array = (const h264::PPS*)video->pps_datas.data();
					const h264::SPS* sps_array = (const h264::SPS*)video->sps_datas.data();
					int prev_pic_order_cnt_lsb = 0;
					int prev_pic_order_cnt_msb = 0;
					int poc_cycle = 0;

					video->frames_infos.reserve(track.sample_count);
					video->slice_header_datas.reserve(track.sample_count * sizeof(h264::SliceHeader));
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

						const uint8_t* src_buffer = input_buf + ofs;
						while (frame_bytes > 0)
						{
							uint32_t size = ((uint32_t)src_buffer[0] << 24) | ((uint32_t)src_buffer[1] << 16) | ((uint32_t)src_buffer[2] << 8) | src_buffer[3];
							size += 4;
							assert(frame_bytes >= size);

#ifdef DEBUG_DUMP_H264
							size_t additional_dump_size = sizeof(h264::start_code) + size - 4;
							dump.resize(dump.size() + additional_dump_size);
							std::memcpy(dump.data() + dump_offset, h264::start_code, sizeof(h264::start_code));
							std::memcpy(dump.data() + dump_offset + sizeof(h264::start_code), src_buffer + 4, size - 4);
							dump_offset += additional_dump_size;
#endif // DEBUG_DUMP_H264

							h264::Bitstream bs = {};
							bs.init(&src_buffer[4], frame_bytes);
							h264::NALHeader nal = {};
							h264::read_nal_header(&nal, &bs);

							if (nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_IDR)
							{
								info.type = VideoFrameType::Intra;
							}
							else if (nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR)
							{
								info.type = VideoFrameType::Predictive;
							}
							else
							{
								// Continue search for frame beginning NAL unit:
								frame_bytes -= size;
								src_buffer += size;
								continue;
							}

							h264::SliceHeader* slice_header = (h264::SliceHeader*)video->slice_header_datas.data() + i;
							*slice_header = {};
							h264::read_slice_header(slice_header, &nal, pps_array, sps_array, &bs);

							const h264::PPS& pps = pps_array[slice_header->pic_parameter_set_id];
							const h264::SPS& sps = sps_array[pps.seq_parameter_set_id];

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
							info.size = sizeof(h264::nal_start_code) + size - 4;
							break;
						}

						aligned_size += AlignTo(info.size, alignment);
						info.timestamp_seconds = float(double(timestamp) * timescale_rcp);
						info.duration_seconds = float(double(duration) * timescale_rcp);
					}

#ifdef DEBUG_DUMP_H264
					wi::helper::FileWrite("dump.h264", dump.data(), dump.size());
#endif // DEBUG_DUMP_H264

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
						video->frames_infos[video->frame_display_order[i]].display_order = (int)i;
					}

					video->average_frames_per_second = float(double(track.timescale) / double(track_duration) * track.sample_count);
					video->duration_seconds = float(double(track_duration) * timescale_rcp);

					auto copy_video_track = [&](void* dest) {
						for (uint32_t i = 0; i < track.sample_count; i++)
						{
							unsigned frame_bytes, timestamp, duration;
							MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
							uint8_t* dst_buffer = (uint8_t*)dest + video->frames_infos[i].offset;
							const uint8_t* src_buffer = input_buf + ofs;
							while (frame_bytes > 0)
							{
								uint32_t size = ((uint32_t)src_buffer[0] << 24) | ((uint32_t)src_buffer[1] << 16) | ((uint32_t)src_buffer[2] << 8) | src_buffer[3];
								size += 4;
								assert(frame_bytes >= size);

								h264::Bitstream bs = {};
								bs.init(&src_buffer[4], sizeof(uint8_t));
								h264::NALHeader nal = {};
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

								std::memcpy(dst_buffer, h264::nal_start_code, sizeof(h264::nal_start_code));
								std::memcpy(dst_buffer + sizeof(h264::nal_start_code), src_buffer + 4, size - 4);
								break;
							}
						}
					};

					GPUBufferDesc bd;
					bd.size = aligned_size;
					bd.usage = Usage::UPLOAD;
					bd.misc_flags = ResourceMiscFlag::VIDEO_DECODE;
					success = device->CreateBuffer2(&bd, copy_video_track, &video->data_stream);
					assert(success);
					device->SetName(&video->data_stream, "wi::Video::data_stream");
				}
				else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN)
				{
					wi::backlog::post("Audio from video file is not implemented yet!");
				}
			}
		}
		else
		{
			wi::helper::messageBox("MP4 parsing failure!", "Error!");
			return false;
		}
		MP4D_close(&mp4);
		return success;
	}
	bool CreateVideoInstance(const Video* video, VideoInstance* instance)
	{
		instance->video = video;
		instance->current_frame = 0;
		instance->flags &= ~VideoInstance::Flags::InitialFirstFrameDecoded;

		GraphicsDevice* device = GetDevice();
		if (!device->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H264))
		{
			wi::helper::messageBox("Video decoding is not supported by your GPU!\nYou can attempt to update graphics driver.\nThere is no CPU decoding implemented yet, video will be disabled!", "Warning!");
			return false;
		}

		VideoDesc vd;
		vd.width = video->padded_width;
		vd.height = video->padded_height;
		vd.bit_rate = video->bit_rate;
		vd.format = Format::NV12;
		vd.profile = video->profile;
		vd.pps_datas = instance->video->pps_datas.data();
		vd.pps_count = instance->video->pps_count;
		vd.sps_datas = instance->video->sps_datas.data();
		vd.sps_count = instance->video->sps_count;
		vd.num_dpb_slots = video->num_dpb_slots;
		bool success = device->CreateVideoDecoder(&vd, &instance->decoder);
		assert(success);

		TextureDesc td;
		td.width = vd.width;
		td.height = vd.height;
		td.format = vd.format;
		td.array_size = video->num_dpb_slots;
		td.bind_flags = BindFlag::SHADER_RESOURCE;
		td.misc_flags = ResourceMiscFlag::VIDEO_DECODE;
		td.layout = ResourceState::VIDEO_DECODE_DST;
		success = device->CreateTexture(&td, nullptr, &instance->dpb.texture);
		assert(success);
		device->SetName(&instance->dpb.texture, "VideoInstance::DPB");

		for (uint32_t i = 0; i < td.array_size; ++i)
		{
			instance->dpb.resource_states[i] = td.layout;
			Format luminance_format = Format::R8_UNORM;
			ImageAspect luminance_aspect = ImageAspect::LUMINANCE;
			instance->dpb.subresources_luminance[i] = device->CreateSubresource(
				&instance->dpb.texture,
				SubresourceType::SRV,
				i, 1, 0, 1,
				&luminance_format, &luminance_aspect
			);

			Format chrominance_format = Format::R8G8_UNORM;
			ImageAspect chrominance_aspect = ImageAspect::CHROMINANCE;
			instance->dpb.subresources_chrominance[i] = device->CreateSubresource(
				&instance->dpb.texture,
				SubresourceType::SRV,
				i, 1, 0, 1,
				&chrominance_format, &chrominance_aspect
			);
		}

		return success;
	}

	bool IsDecodingRequired(const VideoInstance* instance, float dt)
	{
		if (!GetDevice()->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H264))
			return false;
		if (instance == nullptr || instance->video == nullptr)
			return false;
		if (!has_flag(instance->flags, VideoInstance::Flags::InitialFirstFrameDecoded))
			return true;
		if (has_flag(instance->flags, VideoInstance::Flags::Playing) && instance->time_until_next_frame - dt <= 0)
			return true;
		return false;
	}
	void UpdateVideo(VideoInstance* instance, float dt, CommandList cmd)
	{
		if (!GetDevice()->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H264))
			return;
		if (instance == nullptr || instance->video == nullptr)
			return;

		if (has_flag(instance->flags, VideoInstance::Flags::InitialFirstFrameDecoded))
		{
			if (!has_flag(instance->flags, VideoInstance::Flags::Playing))
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
					instance->flags &= ~VideoInstance::Flags::Playing;
					instance->current_frame = (int)instance->video->frames_infos.size() - 1;
				}
			}
		}

		if (!cmd.IsValid())
			return;

		GraphicsDevice* device = GetDevice();

		const Video* video = instance->video;
		instance->current_frame = std::min(instance->current_frame, std::max(0, (int)video->frames_infos.size() - 1));
		const Video::FrameInfo& frame_info = video->frames_infos[instance->current_frame];
		instance->time_until_next_frame = frame_info.duration_seconds;

		const h264::SliceHeader* slice_header = (const h264::SliceHeader*)video->slice_header_datas.data() + instance->current_frame;
		const h264::PPS* pps = (const h264::PPS*)video->pps_datas.data() + slice_header->pic_parameter_set_id;
		const h264::SPS* sps = (const h264::SPS*)video->sps_datas.data() + pps->seq_parameter_set_id;

		VideoDecodeOperation decode_operation;
		if (instance->current_frame == 0 || has_flag(instance->flags, VideoInstance::Flags::DecoderReset))
		{
			decode_operation.flags = VideoDecodeOperation::FLAG_SESSION_RESET;
			instance->flags &= ~VideoInstance::Flags::DecoderReset;
			instance->output = {};
			instance->output_textures_free.clear();
			instance->output_textures_used.clear();
			instance->target_display_order = instance->current_frame;
		}
		if (frame_info.type == VideoFrameType::Intra)
		{
			instance->dpb.reference_usage.clear();
			instance->dpb.next_ref = 0;
			instance->dpb.next_slot = 0;
		}
		instance->dpb.current_slot = instance->dpb.next_slot;
		instance->dpb.poc_status[instance->dpb.current_slot] = frame_info.poc;
		instance->dpb.framenum_status[instance->dpb.current_slot] = slice_header->frame_num;
		decode_operation.stream = &video->data_stream;
		decode_operation.stream_offset = frame_info.offset;
		decode_operation.stream_size = frame_info.size;
		decode_operation.poc[0] = frame_info.poc;
		decode_operation.poc[1] = frame_info.poc;
		decode_operation.frame_type = frame_info.type;
		decode_operation.reference_priority = frame_info.reference_priority;
		decode_operation.decoded_frame_index = instance->current_frame;
		decode_operation.slice_header = slice_header;
		decode_operation.pps = pps;
		decode_operation.sps = sps;
		decode_operation.current_dpb = instance->dpb.current_slot;
		decode_operation.dpb_reference_count = (uint32_t)instance->dpb.reference_usage.size();
		decode_operation.dpb_reference_slots = instance->dpb.reference_usage.data();
		decode_operation.dpb_poc = instance->dpb.poc_status;
		decode_operation.dpb_framenum = instance->dpb.framenum_status;
		decode_operation.DPB = &instance->dpb.texture;

		ImageAspect aspect_luma = ImageAspect::LUMINANCE;
		ImageAspect aspect_chroma = ImageAspect::CHROMINANCE;
		// Ensure that current DPB slot is in DST state:
		if (instance->dpb.resource_states[instance->dpb.current_slot] != ResourceState::VIDEO_DECODE_DST)
		{
			instance->barriers.push_back(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::VIDEO_DECODE_DST, 0, instance->dpb.current_slot, &aspect_luma));
			instance->barriers.push_back(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::VIDEO_DECODE_DST, 0, instance->dpb.current_slot, &aspect_chroma));
			instance->dpb.resource_states[instance->dpb.current_slot] = ResourceState::VIDEO_DECODE_DST;
		}
		// Ensure that reference frame DPB slots are in SRC state:
		for (size_t i = 0; i < instance->dpb.reference_usage.size(); ++i)
		{
			uint8_t ref = instance->dpb.reference_usage[i];
			if (instance->dpb.resource_states[ref] != ResourceState::VIDEO_DECODE_SRC)
			{
				instance->barriers.push_back(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[ref], ResourceState::VIDEO_DECODE_SRC, 0, ref, &aspect_luma));
				instance->barriers.push_back(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[ref], ResourceState::VIDEO_DECODE_SRC, 0, ref, &aspect_chroma));
				instance->dpb.resource_states[ref] = ResourceState::VIDEO_DECODE_SRC;
			}
		}
		if (!instance->barriers.empty())
		{
			device->Barrier(instance->barriers.data(), (uint32_t)instance->barriers.size(), cmd);
			instance->barriers.clear();
		}

		device->VideoDecode(&instance->decoder, &decode_operation, cmd);

		// The current DPB slot is transitioned into a shader readable state because it will need to be resolved into RGB on a different GPU queue:
		//	The video queue must be used to transition from video states
		if (instance->dpb.resource_states[instance->dpb.current_slot] != ResourceState::SHADER_RESOURCE_COMPUTE)
		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::SHADER_RESOURCE_COMPUTE, 0, instance->dpb.current_slot, &aspect_luma),
				GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::SHADER_RESOURCE_COMPUTE, 0, instance->dpb.current_slot, &aspect_chroma),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
			instance->dpb.resource_states[instance->dpb.current_slot] = ResourceState::SHADER_RESOURCE_COMPUTE;
		}

		// DPB slot management:
		//	When current frame was a reference, then the next frame can not overwrite its DPB slot, so increment next_slot as a ring buffer
		//	However, the ring buffer will wrap around so older reference frames can be overwritten by this
		if (frame_info.reference_priority > 0)
		{
			if (instance->dpb.next_ref >= instance->dpb.reference_usage.size())
			{
				instance->dpb.reference_usage.resize(instance->dpb.next_ref + 1);
			}
			instance->dpb.reference_usage[instance->dpb.next_ref] = instance->dpb.current_slot;
			instance->dpb.next_ref = (instance->dpb.next_ref + 1) % (instance->dpb.texture.desc.array_size - 1);
			instance->dpb.next_slot = (instance->dpb.next_slot + 1) % instance->dpb.texture.desc.array_size;
		}

		instance->flags |= VideoInstance::Flags::NeedsResolve;
		instance->flags |= VideoInstance::Flags::InitialFirstFrameDecoded;
		instance->current_frame++;
	}
	void ResolveVideoToRGB(VideoInstance* instance, CommandList cmd)
	{
		if (instance == nullptr || instance->video == nullptr)
			return;
		if (!has_flag(instance->flags, VideoInstance::Flags::NeedsResolve))
			return;
		instance->flags &= ~VideoInstance::Flags::NeedsResolve;

		const Video* video = instance->video;
		GraphicsDevice* device = GetDevice();

		if (instance->output_textures_free.empty())
		{
			VideoInstance::OutputTexture& output = instance->output_textures_free.emplace_back();
			TextureDesc td;
			td.width = video->width;
			td.height = video->height;
			td.format = Format::R8G8B8A8_UNORM;
			if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
			{
				td.mip_levels = 0; // max mipcount
			}
			td.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			td.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			bool success = device->CreateTexture(&td, nullptr, &output.texture);
			device->SetName(&output.texture, "VideoInstance::OutputTexture");
			assert(success);

			if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
			{
				for (uint32_t i = 0; i < output.texture.GetDesc().mip_levels; ++i)
				{
					int subresource_index;
					subresource_index = device->CreateSubresource(&output.texture, SubresourceType::SRV, 0, 1, i, 1);
					assert(subresource_index == i);
					subresource_index = device->CreateSubresource(&output.texture, SubresourceType::UAV, 0, 1, i, 1);
					assert(subresource_index == i);
				}
			}

			// This part must be AFTER mip level subresource creation:
			Format srgb_format = GetFormatSRGB(td.format);
			output.subresource_srgb = device->CreateSubresource(
				&output.texture,
				SubresourceType::SRV,
				0, -1,
				0, -1,
				&srgb_format
			);
		}

		VideoInstance::OutputTexture output = std::move(instance->output_textures_free.back());
		instance->output_textures_free.pop_back();
		output.display_order = video->frames_infos[std::max(instance->current_frame - 1, 0)].display_order;

		wi::renderer::YUV_to_RGB(
			instance->dpb.texture,
			instance->dpb.subresources_luminance[instance->dpb.current_slot],
			instance->dpb.subresources_chrominance[instance->dpb.current_slot],
			output.texture,
			cmd
		);

		if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
		{
			wi::renderer::GenerateMipChain(output.texture, wi::renderer::MIPGENFILTER_LINEAR, cmd);
		}

		instance->output_textures_used.push_back(std::move(output));

		for (size_t i = 0; i < instance->output_textures_used.size(); ++i)
		{
			if (instance->output_textures_used[i].display_order == instance->target_display_order)
			{
				if (instance->output.texture.IsValid())
				{
					// Free current output texture:
					instance->output_textures_free.push_back(std::move(instance->output));
				}

				// Take this used texture as current oputput:
				instance->output = std::move(instance->output_textures_used[i]);

				// Remove this used texture:
				std::swap(instance->output_textures_used[i], instance->output_textures_used.back());
				instance->output_textures_used.pop_back();

				// request next displayable picture in order:
				instance->target_display_order++;
				break;
			}
		}
	}
}
