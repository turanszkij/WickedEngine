#include "wiVideo.h"
#include "wiHelper.h"
#include "wiRenderer.h"
#include "wiBacklog.h"

#include <cmath>

#include "Utility/minimp4.h"
#include "Utility/h264.h"
#include "Utility/aac.h"

//#define DEBUG_DUMP_H264
static const int dump_frame_count = 100;

using namespace wi::graphics;

namespace wi::video
{
	static int PicNum(int frame_num, int curr, int max_frame_num)
	{
		return frame_num > curr ? frame_num - max_frame_num : frame_num;
	}

	static void AppendSliceHeader(Video* video, const h264::SliceHeader& slice_header)
	{
		video->slice_header_datas.resize(video->slice_header_datas.size() + sizeof(h264::SliceHeader));
		std::memcpy((h264::SliceHeader*)video->slice_header_datas.data() + video->slice_header_count, &slice_header, sizeof(slice_header));
		video->slice_header_count++;
	}

	static int CountUsed(const VideoInstance::DPB& dpb, uint32_t slot_count)
	{
		int count = 0;
		for (uint32_t i = 0; i < slot_count; ++i)
			if (dpb.pictures[i].used)
				count++;
		return count;
	}

	static void BeginDecodePicture(VideoInstance* instance)
	{
		const Video* video = instance->video;
		VideoInstance::DPB& dpb = instance->dpb;
		const int frame_index = instance->current_decode_frame;
		const Video::FrameInfo& info = video->frame_infos[frame_index];
		const h264::SliceHeader& sh = ((const h264::SliceHeader*)video->slice_header_datas.data())[frame_index];
		const h264::PPS& pps = ((const h264::PPS*)video->pps_datas.data())[sh.pic_parameter_set_id];
		const h264::SPS& sps = ((const h264::SPS*)video->sps_datas.data())[pps.seq_parameter_set_id];

		if (frame_index == 0 || info.type == VideoFrameType::Intra)
		{
			for (auto& picture : dpb.pictures)
				picture = {};
			dpb.reference_usage.clear();
		}

		int slot = -1;
		for (uint32_t i = 0; i < video->num_dpb_slots; ++i)
		{
			if (!dpb.pictures[i].used)
			{
				slot = (int)i;
				break;
			}
		}
		assert(slot >= 0);
		dpb.current_slot = (uint8_t)slot;

		for (uint32_t i = 0; i < 17; ++i)
		{
			dpb.poc_status[i] = 0;
			dpb.framenum_status[i] = 0;
		}
		dpb.poc_status[dpb.current_slot] = info.poc;
		dpb.framenum_status[dpb.current_slot] = sh.frame_num;

		const int max_frame_num = 1 << (sps.log2_max_frame_num_minus4 + 4);
		const int curr = sh.frame_num;
		dpb.reference_usage.clear();
		for (uint32_t i = 0; i < video->num_dpb_slots; ++i)
		{
			if (!dpb.pictures[i].used)
				continue;
			dpb.reference_usage.push_back((uint8_t)i);
			dpb.poc_status[i] = dpb.pictures[i].poc;
			dpb.framenum_status[i] = dpb.pictures[i].frame_num;
		}
		std::sort(dpb.reference_usage.begin(), dpb.reference_usage.end(), [&](uint8_t a, uint8_t b) {
			return PicNum(dpb.pictures[a].frame_num, curr, max_frame_num) > PicNum(dpb.pictures[b].frame_num, curr, max_frame_num);
		});
		assert(dpb.reference_usage.size() <= 16);
		assert(dpb.reference_usage.size() <= (size_t)std::max(sps.num_ref_frames, 1));
		for (uint8_t ref_slot : dpb.reference_usage)
			assert(ref_slot != dpb.current_slot);
	}

	static void EndDecodePicture(VideoInstance* instance)
	{
		const Video* video = instance->video;
		VideoInstance::DPB& dpb = instance->dpb;
		const int frame_index = instance->current_decode_frame;
		const Video::FrameInfo& info = video->frame_infos[frame_index];
		const h264::SliceHeader& sh = ((const h264::SliceHeader*)video->slice_header_datas.data())[frame_index];
		const h264::PPS& pps = ((const h264::PPS*)video->pps_datas.data())[sh.pic_parameter_set_id];
		const h264::SPS& sps = ((const h264::SPS*)video->sps_datas.data())[pps.seq_parameter_set_id];
		const int max_frame_num = 1 << (sps.log2_max_frame_num_minus4 + 4);
		const int curr = sh.frame_num;
		const int ref_limit = std::max(sps.num_ref_frames, 1);
		const bool idr = info.type == VideoFrameType::Intra;

		if (info.reference_priority == 0)
		{
			assert(!dpb.pictures[dpb.current_slot].used);
			return;
		}

		if (!idr && sh.drpm.adaptive_ref_pic_marking_mode_flag)
		{
			for (int n = 0; n < 64; ++n)
			{
				const int op = sh.drpm.memory_management_control_operation[n];
				if (op == 0)
					break;
				if (op != 1)
					continue;
				const int pic_num = curr - (sh.drpm.difference_of_pic_nums_minus1[n] + 1);
				for (uint32_t i = 0; i < video->num_dpb_slots; ++i)
				{
					if (dpb.pictures[i].used && PicNum(dpb.pictures[i].frame_num, curr, max_frame_num) == pic_num)
					{
						dpb.pictures[i] = {};
						break;
					}
				}
			}
		}
		else if (!idr)
		{
			while (CountUsed(dpb, video->num_dpb_slots) >= ref_limit)
			{
				int oldest = -1;
				int oldest_pic = 0;
				for (uint32_t i = 0; i < video->num_dpb_slots; ++i)
				{
					if (!dpb.pictures[i].used)
						continue;
					const int pic_num = PicNum(dpb.pictures[i].frame_num, curr, max_frame_num);
					if (oldest < 0 || pic_num < oldest_pic)
					{
						oldest = (int)i;
						oldest_pic = pic_num;
					}
				}
				if (oldest < 0)
					break;
				dpb.pictures[oldest] = {};
			}
		}

		auto& picture = dpb.pictures[dpb.current_slot];
		picture.used = true;
		picture.frame_num = sh.frame_num;
		picture.poc = info.poc;
		assert(CountUsed(dpb, video->num_dpb_slots) <= ref_limit);
	}

	void CalculatePictureOrder(Video* video)
	{
		if (video->profile == VideoProfile::H264)
		{
			const h264::SliceHeader* slice_header_array = (const h264::SliceHeader*)video->slice_header_datas.data();
			const h264::PPS* pps_array = (const h264::PPS*)video->pps_datas.data();
			const h264::SPS* sps_array = (const h264::SPS*)video->sps_datas.data();
			int prev_poc_msb = 0;
			int prev_poc_lsb = 0;
			int prev_frame_num = 0;
			int prev_frame_num_offset = 0;
			bool has_prev_frame_num = false;
			int gop = -1;
			for (size_t i = 0; i < video->frame_infos.size(); ++i)
			{
				const h264::SliceHeader& slice_header = slice_header_array[i];
				const h264::PPS& pps = pps_array[slice_header.pic_parameter_set_id];
				const h264::SPS& sps = sps_array[pps.seq_parameter_set_id];
				const bool idr = video->frame_infos[i].type == VideoFrameType::Intra;
				const bool is_ref = video->frame_infos[i].reference_priority != 0;
				const int max_frame_num = 1 << (sps.log2_max_frame_num_minus4 + 4);

				// A pic_order_cnt_lsb of 0 is not a new GOP. IDR is.
				// The previous picture order moves only on reference pictures.
				if (idr)
				{
					prev_poc_msb = 0;
					prev_poc_lsb = 0;
					prev_frame_num = 0;
					prev_frame_num_offset = 0;
					has_prev_frame_num = false;
					gop++;
				}

				// H.264 8.2.1.3: pic_order_cnt_type 2 has no pic_order_cnt_lsb. POC comes from frame_num.
				if (sps.pic_order_cnt_type == 2)
				{
					int frame_num_offset = 0;
					if (!idr && has_prev_frame_num && prev_frame_num > slice_header.frame_num)
						frame_num_offset = prev_frame_num_offset + max_frame_num;
					else if (!idr)
						frame_num_offset = prev_frame_num_offset;

					int poc = 0;
					if (!idr)
					{
						poc = 2 * (frame_num_offset + slice_header.frame_num);
						if (!is_ref)
							poc -= 1;
					}
					video->frame_infos[i].poc = poc;
					video->frame_infos[i].gop = gop;
					if (is_ref)
					{
						prev_frame_num = slice_header.frame_num;
						prev_frame_num_offset = frame_num_offset;
						has_prev_frame_num = true;
					}
					continue;
				}

				const int max_poc_lsb = 1 << (sps.log2_max_pic_order_cnt_lsb_minus4 + 4);
				int poc_msb = 0;
				const int poc_lsb = slice_header.pic_order_cnt_lsb;
				if (poc_lsb < prev_poc_lsb && (prev_poc_lsb - poc_lsb) >= max_poc_lsb / 2)
					poc_msb = prev_poc_msb + max_poc_lsb;
				else if (poc_lsb > prev_poc_lsb && (poc_lsb - prev_poc_lsb) > max_poc_lsb / 2)
					poc_msb = prev_poc_msb - max_poc_lsb;
				else
					poc_msb = prev_poc_msb;

				video->frame_infos[i].poc = poc_msb + poc_lsb;
				video->frame_infos[i].gop = gop;

				if (is_ref)
				{
					prev_poc_msb = poc_msb;
					prev_poc_lsb = poc_lsb;
				}
			}
		}
		if (video->profile == VideoProfile::H265)
		{
			assert(0); // TODO
		}

		wi::vector<size_t> frame_display_order(video->frame_infos.size());
		for (size_t i = 0; i < video->frame_infos.size(); ++i)
		{
			frame_display_order[i] = i;
		}
		std::stable_sort(frame_display_order.begin(), frame_display_order.end(), [&](size_t a, size_t b) {
			const Video::FrameInfo& frameA = video->frame_infos[a];
			const Video::FrameInfo& frameB = video->frame_infos[b];
			if (frameA.gop != frameB.gop)
				return frameA.gop < frameB.gop;
			if (frameA.poc != frameB.poc)
				return frameA.poc < frameB.poc;
			return a < b;
		});
		float timestamp = 0;
		for (size_t i = 0; i < frame_display_order.size(); ++i)
		{
			Video::FrameInfo& frame_info = video->frame_infos[frame_display_order[i]];
			frame_info.display_order = (int)i;
			frame_info.timestamp_seconds = timestamp;
			timestamp += frame_info.duration_seconds;
		}
	}

	bool CreateVideo(const std::string& filename, Video* video)
	{
		wi::vector<uint8_t> filedata;
		if (!wi::helper::FileRead(filename, filedata))
			return false;
		std::string ext = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(filename));
		if (ext.find("MP4") != std::string::npos)
		{
			return CreateVideoMP4(filedata.data(), filedata.size(), video);
		}
		if (ext.find("H264") != std::string::npos)
		{
			return CreateVideoH264RAW(filedata.data(), filedata.size(), video);
		}
		return false;
	}

	bool CreateVideoMP4(const uint8_t* filedata, size_t filesize, Video* video)
	{
		*video = {};
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
			if (offset < 0 || (uint64_t)offset >= buf->size)
				return 1;
			size_t to_copy = MINIMP4_MIN(size, buf->size - (size_t)offset);
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
				const MP4D_track_t& track = mp4.track[ntrack];

#ifdef DEBUG_DUMP_H264
				wi::vector<uint8_t> dump;
				size_t dump_offset = 0;
				int dump_frame_counter = 0;
#endif // DEBUG_DUMP_H264

				if (track.handler_type == MP4D_HANDLER_TYPE_VIDE)
				{
					switch (track.object_type_indication)
					{
					case MP4_OBJECT_TYPE_AVC:
						video->profile = VideoProfile::H264;
						break;
					case MP4_OBJECT_TYPE_HEVC:
						video->profile = VideoProfile::H265;
						wilog_warning("H265 (HEVC) video format is not supported yet!");
						return false;
					default:
						wilog_warning("Unknown video format! track.object_type_indication = %d", (int)track.object_type_indication);
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
						while ((data = MP4D_read_sps(&mp4, ntrack, index, &size)))
						{
							const uint8_t* sps_data = (const uint8_t*)data;
#ifdef DEBUG_DUMP_H264
							size_t additional_dump_size = sizeof(h264::nal_start_code) + size;
							dump.resize(dump.size() + additional_dump_size);
							std::memcpy(dump.data() + dump_offset, h264::nal_start_code, sizeof(h264::nal_start_code));
							std::memcpy(dump.data() + dump_offset + sizeof(h264::nal_start_code), sps_data, size);
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
						while ((data = MP4D_read_pps(&mp4, ntrack, index, &size)))
						{
							const uint8_t* pps_data = (const uint8_t*)data;
#ifdef DEBUG_DUMP_H264
							size_t additional_dump_size = sizeof(h264::nal_start_code) + size;
							dump.resize(dump.size() + additional_dump_size);
							std::memcpy(dump.data() + dump_offset, h264::nal_start_code, sizeof(h264::nal_start_code));
							std::memcpy(dump.data() + dump_offset + sizeof(h264::nal_start_code), pps_data, size);
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

					video->frame_infos.reserve(track.sample_count);
					video->slice_header_datas.reserve(track.sample_count * sizeof(h264::SliceHeader));
					video->slice_offsets.reserve(track.sample_count);
					struct SlicePiece
					{
						const uint8_t* payload = nullptr;
						uint32_t size = 0;
					};
					wi::vector<SlicePiece> slice_pieces;
					slice_pieces.reserve(track.sample_count);
					uint32_t track_duration = 0;
					uint64_t aligned_size = 0;
					for (uint32_t i = 0; i < track.sample_count; i++)
					{
						unsigned frame_bytes, timestamp, duration;
						MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
						track_duration += duration;

						const uint8_t* src_buffer = input_buf + ofs;
						int frame_index = -1;
						uint32_t cursor = 0;
						while (frame_bytes > 0)
						{
							if (frame_bytes < 4)
								break;
							uint32_t payload_size = ((uint32_t)src_buffer[0] << 24) | ((uint32_t)src_buffer[1] << 16) | ((uint32_t)src_buffer[2] << 8) | src_buffer[3];
							if (payload_size > frame_bytes - 4)
								break;
							const uint32_t sample_bytes = payload_size + 4;

							h264::Bitstream bs = {};
							bs.init(&src_buffer[4], payload_size);
							h264::NALHeader nal = {};
							if (!h264::read_nal_header(&nal, &bs))
							{
								frame_bytes -= sample_bytes;
								src_buffer += sample_bytes;
								continue;
							}

							const bool idr = nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_IDR;
							const bool non_idr = nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR;
							if (!idr && !non_idr)
							{
								frame_bytes -= sample_bytes;
								src_buffer += sample_bytes;
								continue;
							}

							// One MP4 sample is one access unit. Every slice NAL has to reach the
							// decoder; the first slice is often only the top macroblock rows.
							if (frame_index < 0)
							{
								frame_index = (int)video->frame_infos.size();
								Video::FrameInfo& info = video->frame_infos.emplace_back();
								info.offset = aligned_size;
								info.duration_seconds = float(double(duration) * timescale_rcp);
								info.type = idr ? VideoFrameType::Intra : VideoFrameType::Predictive;
								info.reference_priority = nal.idc;
								info.slice_offset_index = (uint32_t)video->slice_offsets.size();
								h264::SliceHeader slice_header = {};
								h264::read_slice_header(&slice_header, &nal, (const h264::PPS*)video->pps_datas.data(), (const h264::SPS*)video->sps_datas.data(), &bs);
								AppendSliceHeader(video, slice_header);
							}

#ifdef DEBUG_DUMP_H264
							if (dump_frame_counter < dump_frame_count)
							{
								size_t additional_dump_size = sizeof(h264::nal_start_code) + payload_size;
								dump.resize(dump.size() + additional_dump_size);
								std::memcpy(dump.data() + dump_offset, h264::nal_start_code, sizeof(h264::nal_start_code));
								std::memcpy(dump.data() + dump_offset + sizeof(h264::nal_start_code), src_buffer + 4, payload_size);
								dump_offset += additional_dump_size;
								dump_frame_counter++;
							}
#endif // DEBUG_DUMP_H264

							video->slice_offsets.push_back(cursor);
							video->frame_infos[frame_index].slice_count++;
							slice_pieces.push_back({ src_buffer + 4, payload_size });
							cursor += (uint32_t)sizeof(h264::nal_start_code) + payload_size;

							frame_bytes -= sample_bytes;
							src_buffer += sample_bytes;
						}

						if (frame_index >= 0)
						{
							video->frame_infos[frame_index].size = cursor;
							aligned_size += align((uint64_t)cursor, alignment);
						}
					}

#ifdef DEBUG_DUMP_H264
					wi::helper::FileWrite("dump.h264", dump.data(), dump.size());

					// validate dump:
					if (wi::helper::FileRead("dump.h264", dump))
					{
						using namespace h264;
						Bitstream bs;
						bs.init(dump.data(), dump.size());
						while (find_next_nal(&bs))
						{
							NALHeader nal;
							if (read_nal_header(&nal, &bs))
							{
								switch (nal.type)
								{
								case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_CODED_SLICE_NON_IDR");
									break;
								case NAL_UNIT_TYPE_CODED_SLICE_IDR:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_CODED_SLICE_IDR");
									break;
								case NAL_UNIT_TYPE_SPS:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_SPS");
									break;
								case NAL_UNIT_TYPE_PPS:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_PPS");
									break;
								case NAL_UNIT_TYPE_SEI:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_SEI");
									break;
								case NAL_UNIT_TYPE_AUD:
									wilog("H264 dump validation NAL found: NAL_UNIT_TYPE_AUD");
									break;
								default:
									wilog("H264 dump validation NAL found: %d", (int)nal.type);
									break;
								}
							}
							else
							{
								wilog_assert(0, "H264 dump validation failed: invalid NAL!");
							}
						}
					}
					else
					{
						wilog_assert(0, "H264 dump validation failed: file doesn't exist!");
					}
#endif // DEBUG_DUMP_H264

					if (!video->frame_infos.empty())
					{
						CalculatePictureOrder(video);

						video->average_frames_per_second = float(double(track.timescale) / double(track_duration) * video->frame_infos.size());
						video->duration_seconds = float(double(track_duration) * timescale_rcp);

						auto copy_video_track = [&](void* dest) {
							uint32_t piece_index = 0;
							for (const Video::FrameInfo& frame_info : video->frame_infos)
							{
								uint8_t* dst_buffer = (uint8_t*)dest + frame_info.offset;
								const uint64_t slot = align(frame_info.size, alignment);
								std::memset(dst_buffer, 0, slot);
								uint32_t written = 0;
								for (uint32_t slice = 0; slice < frame_info.slice_count; ++slice)
								{
									const SlicePiece& piece = slice_pieces[piece_index++];
									std::memcpy(dst_buffer + written, h264::nal_start_code, sizeof(h264::nal_start_code));
									written += (uint32_t)sizeof(h264::nal_start_code);
									std::memcpy(dst_buffer + written, piece.payload, piece.size);
									written += piece.size;
								}
							}
						};

						GPUBufferDesc bd;
						bd.size = aligned_size;
						bd.usage = Usage::DEFAULT;
						bd.misc_flags = ResourceMiscFlag::VIDEO_DECODE | ResourceMiscFlag::VIDEO_COMPATIBILITY_H264;
						success = device->CreateBuffer2(&bd, copy_video_track, &video->data_stream);
						assert(success);
						device->SetName(&video->data_stream, "wi::Video::data_stream");
					}
					else
					{
						wilog_warning("MP4 video track does not contain any decodable frames");
					}
				}
				else if (track.handler_type == MP4D_HANDLER_TYPE_SOUN)
				{
					if (video->audio.IsValid())
						continue;
					
					const MP4D_track_t& track = mp4.track[ntrack];
					const bool aac =
						track.object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3 ||
						track.object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE ||
						track.object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE ||
						track.object_type_indication == MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE;
					if (!aac || track.dsi == nullptr || track.dsi_bytes < aac::MIN_AUDIO_SPECIFIC_CONFIG_BYTES || track.sample_count == 0)
					{
						wilog_warning("Video: audio format not supported!\nsupported formats = ISO_IEC_14496_3, ISO_IEC_13818_7_MAIN_PROFILE, ISO_IEC_13818_7_LC_PROFILE, IEC_13818_7_SSR_PROFILE\ncurrent value = %d", (int)track.object_type_indication);
						continue;
					}

					uint32_t audio_object_type = 0;
					uint32_t sample_rate_index = 0;
					uint32_t channels = 0;
					if (!aac::parse(track.dsi, track.dsi_bytes, audio_object_type, sample_rate_index, channels))
					{
						wilog_warning("Video: audio parameter error!");
						continue;
					}
					if (channels == aac::CHANNEL_CONFIGURATION_AOT_SPECIFIC)
						channels = track.SampleDescription.audio.channelcount;

					// AAC is raw AAC compressed audio frame
					//	It will be converted to ADTS which is AAC frames + ADTS headers separating frames, that can be fed into platform's audio API later
					wi::vector<uint8_t> adts;
					adts.reserve(size_t(track.sample_count) * 256);
					for (unsigned sample = 0; sample < track.sample_count; ++sample)
					{
						unsigned frame_bytes = 0;
						unsigned timestamp = 0;
						unsigned duration = 0;
						const MP4D_file_offset_t offset = MP4D_frame_offset(&mp4, ntrack, sample, &frame_bytes, &timestamp, &duration);
						if (frame_bytes < 1 || frame_bytes + sizeof(aac::ADTSHeader) > aac::MAX_ADTS_FRAME_LENGTH)
							continue;
						if ((uint64_t)offset + frame_bytes > filesize)
							continue;
						aac::ADTSHeader header;
						aac::write_adts_header(&header, audio_object_type, sample_rate_index, channels, frame_bytes);
						adts.insert(adts.end(), header.values, header.values + arraysize(header.values));
						const uint8_t* src = filedata + offset;
						adts.insert(adts.end(), src, src + frame_bytes);
					}
					if (adts.empty())
					{
						wilog_warning("Video: audio AAC -> ADTS conversion error!");
						continue;
					}
					if (!wi::audio::CreateSound(adts.data(), adts.size(), &video->audio))
					{
						wilog_warning("Video: audio creation error!");
					}
				}
			}
		}
		else
		{
			wilog_warning("MP4 parsing failure! MP4D_open result = %d", result);
			return false;
		}
		MP4D_close(&mp4);
		return success;
	}
	bool CreateVideoH264RAW(const uint8_t* filedata, size_t filesize, Video* video)
	{
		*video = {};
		bool success = false;
		GraphicsDevice* device = GetDevice();

		h264::Bitstream bs;
		bs.init(filedata, filesize);
		int open_frame = -1;
		while (h264::find_next_nal(&bs))
		{
			const uint64_t nal_offset = bs.byte_offset() - sizeof(h264::nal_start_code);

			h264::NALHeader nal = {};
			if (h264::read_nal_header(&nal, &bs))
			{
				const bool is_slice = nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_IDR || nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR;
				h264::SliceHeader slice_header = {};
				if (is_slice)
					h264::read_slice_header(&slice_header, &nal, (const h264::PPS*)video->pps_datas.data(), (const h264::SPS*)video->sps_datas.data(), &bs);

				const bool continuation = is_slice && open_frame >= 0 && video->frame_infos[open_frame].size == 0 && slice_header.first_mb_in_slice != 0;
				if (open_frame >= 0 && video->frame_infos[open_frame].size == 0 && !continuation)
					video->frame_infos[open_frame].size = nal_offset - video->frame_infos[open_frame].offset;

				if (continuation)
				{
					Video::FrameInfo& frame_info = video->frame_infos[open_frame];
					video->slice_offsets.push_back((uint32_t)(nal_offset - frame_info.offset));
					frame_info.slice_count++;
					continue;
				}

				switch (nal.type)
				{
				case h264::NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
				case h264::NAL_UNIT_TYPE_CODED_SLICE_IDR:
				{
					open_frame = (int)video->frame_infos.size();
					Video::FrameInfo& frame_info = video->frame_infos.emplace_back();
					frame_info.offset = nal_offset;
					frame_info.type = nal.type == h264::NAL_UNIT_TYPE_CODED_SLICE_IDR ? wi::graphics::VideoFrameType::Intra : wi::graphics::VideoFrameType::Predictive;
					frame_info.reference_priority = nal.idc;
					frame_info.slice_offset_index = (uint32_t)video->slice_offsets.size();
					frame_info.slice_count = 1;
					video->slice_offsets.push_back(0);
					AppendSliceHeader(video, slice_header);
				}
				break;
				case h264::NAL_UNIT_TYPE_SPS:
				{
					if (video->sps_count == 0) // TODO: fix issues with multiple SPS
					{
						h264::SPS sps = {};
						h264::read_sps(&sps, &bs);
						video->sps_datas.resize(video->sps_datas.size() + sizeof(h264::SPS));
						std::memcpy((h264::SPS*)video->sps_datas.data() + video->sps_count, &sps, sizeof(sps));
						video->sps_count++;

						video->width = ((sps.pic_width_in_mbs_minus1 + 1) * 16) - sps.frame_crop_left_offset * 2 - sps.frame_crop_right_offset * 2;
						video->height = ((2 - sps.frame_mbs_only_flag) * (sps.pic_height_in_map_units_minus1 + 1) * 16) - (sps.frame_crop_top_offset * 2) - (sps.frame_crop_bottom_offset * 2);
						video->padded_width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
						video->padded_height = (sps.pic_height_in_map_units_minus1 + 1) * 16;
						video->num_dpb_slots = std::max(video->num_dpb_slots, uint32_t(sps.num_ref_frames + 1));
					}
				}
				break;
				case h264::NAL_UNIT_TYPE_PPS:
				{
					if (video->pps_count == 0) // TODO: fix issues with multiple PPS
					{
						h264::PPS pps = {};
						h264::read_pps(&pps, &bs);
						video->pps_datas.resize(video->pps_datas.size() + sizeof(h264::PPS));
						std::memcpy((h264::PPS*)video->pps_datas.data() + video->pps_count, &pps, sizeof(pps));
						video->pps_count++;
					}
				}
				break;
				default:
					break;
				}
			}
			else
			{
				wilog_warning("CreateVideoH264RAW: found invalid NAL unit!\n");
			}
		}

		if (open_frame >= 0 && video->frame_infos[open_frame].size == 0)
			video->frame_infos[open_frame].size = filesize - video->frame_infos[open_frame].offset;

		// Calculate the total aligned size of the bitstream buffer that will contain the slice datas:
		const uint64_t alignment = device->GetVideoDecodeBitstreamAlignment();
		uint64_t aligned_size = 0;
		video->duration_seconds = 0;
		video->average_frames_per_second = 60;
		for (Video::FrameInfo& frame_info : video->frame_infos)
		{
			aligned_size += align(frame_info.size, alignment);
			frame_info.duration_seconds = 1.0f / 60.0f; // 60 FPS lock
			video->duration_seconds += frame_info.duration_seconds;
		}

		CalculatePictureOrder(video);

		// Write the slice datas into the aligned offsets, and store the aligned offsets in frame_infos, from here they will be storing offsets into the bitstream buffer, and not the source file:
		uint64_t aligned_offset = 0;
		auto copy_video_track = [&](void* dest) {
			for (Video::FrameInfo& frame_info : video->frame_infos)
			{
				uint8_t* dst = (uint8_t*)dest + aligned_offset;
				const uint64_t slot = align(frame_info.size, alignment);
				std::memset(dst, 0, slot);
				std::memcpy(dst, filedata + frame_info.offset, (size_t)frame_info.size);
				frame_info.offset = aligned_offset;
				aligned_offset += slot;
			}
		};

		GPUBufferDesc bd;
		bd.size = aligned_size;
		bd.usage = Usage::DEFAULT;
		bd.misc_flags = ResourceMiscFlag::VIDEO_DECODE | ResourceMiscFlag::VIDEO_COMPATIBILITY_H264;
		success = device->CreateBuffer2(&bd, copy_video_track, &video->data_stream);
		assert(success);
		device->SetName(&video->data_stream, "wi::Video::data_stream");
		return success;
	}
	bool CreateVideoInstance(const Video* video, VideoInstance* instance)
	{
		instance->video = video;
		instance->audio = {};
		instance->audio_time_origin = 0;
		instance->audio_sync_time = 0;
		instance->audio_unavailable = false;
		instance->current_decode_frame = 0;
		instance->flags &= ~VideoInstance::Flags::InitialFirstFrameDecoded;
		instance->output_textures_free.clear();
		instance->output_textures_used.clear();
		instance->output_textures_resolve_request.clear();

		GraphicsDevice* device = GetDevice();
		if (video->profile == VideoProfile::H264 && !device->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H264))
		{
			wilog_warning("The H264 video decoding implementation is not supported by your GPU!\nYou can attempt to update graphics driver.\nThere is no CPU decoding implemented now, video will be disabled!");
			return false;
		}
		if (video->profile == VideoProfile::H265 && !device->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H265))
		{
			wilog_warning("The H265 video decoding implementation is not supported by your GPU!\nYou can attempt to update graphics driver.\nThere is no CPU decoding implemented now, video will be disabled!");
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
		if (has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
		{
			td.bind_flags = BindFlag::SHADER_RESOURCE;
			td.misc_flags = ResourceMiscFlag::VIDEO_DECODE;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
		}
		else
		{
			td.misc_flags = ResourceMiscFlag::VIDEO_DECODE_DPB_ONLY;
			td.layout = ResourceState::VIDEO_DECODE_DPB;
		}
		td.misc_flags |= ResourceMiscFlag::VIDEO_COMPATIBILITY_H264;
		success = device->CreateTexture(&td, nullptr, &instance->dpb.texture);
		assert(success);
		device->SetName(&instance->dpb.texture, "VideoInstance::DPB::texture");

		if (has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
		{
			// DPB_AND_OUTPUT_COINCIDE so DPB can be used for output:
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
		}

		if (video->audio.IsValid())
		{
			instance->audio.SetLooped(false);
			if (!wi::audio::CreateSoundInstance(&video->audio, &instance->audio))
				instance->audio_unavailable = true;
		}

		return success;
	}

	static float VideoAudioTime(const VideoInstance* instance)
	{
		if (instance->video == nullptr || !instance->video->audio.IsValid() || !instance->audio.IsValid())
			return 0;
		const wi::audio::SampleInfo info = wi::audio::GetSampleInfo(&instance->video->audio);
		if (info.sample_rate <= 0)
			return 0;
		const float played = (float)wi::audio::GetTotalSamplesPlayed(&instance->audio) / (float)info.sample_rate;
		return instance->audio_time_origin + played;
	}
	static void SyncVideoAudio(VideoInstance* instance, bool force)
	{
		if (instance == nullptr || instance->video == nullptr || !instance->video->audio.IsValid() || instance->audio_unavailable)
			return;
		const bool playing = has_flag(instance->flags, VideoInstance::Flags::Playing);
		const float video_time = instance->current_time > 0.f ? instance->current_time : 0.f;
		if (!has_flag(instance->flags, VideoInstance::Flags::Looped) && video_time > instance->video->duration_seconds)
		{
			if (instance->audio.IsValid())
				wi::audio::Pause(&instance->audio);
			return;
		}
		if (!instance->audio.IsValid())
			force = true;
		if (!force)
		{
			const float audio_time = VideoAudioTime(instance);
			const bool jumped = video_time + 0.05f < instance->audio_sync_time;
			const bool drifted = std::abs(audio_time - video_time) > 0.2f && (video_time - instance->audio_sync_time) > 0.5f;
			force = jumped || drifted;
		}
		if (force)
		{
			instance->audio = {};
			instance->audio.begin = video_time;
			instance->audio.SetLooped(false);
			if (!wi::audio::CreateSoundInstance(&instance->video->audio, &instance->audio))
			{
				instance->audio_unavailable = true;
				return;
			}
			instance->audio_time_origin = video_time;
			instance->audio_sync_time = video_time;
			if (playing)
			{
				wi::audio::Play(&instance->audio);
			}
			else
			{
				wi::audio::Pause(&instance->audio);
			}
			instance->audio.begin = 0;
			return;
		}
		if (playing)
			wi::audio::Play(&instance->audio);
		else
			wi::audio::Pause(&instance->audio);
	}

	void UpdateDisplayOrderOutput(VideoInstance* instance)
	{
		// Check if current output texture can be replaced by a newer one:
		if (instance->output.display_order != instance->target_display_order)
		{
			for (size_t i = 0; i < instance->output_textures_used.size(); ++i)
			{
				if (instance->output_textures_used[i].display_order == instance->target_display_order)
				{
					if (instance->output.texture.IsValid())
					{
						// Free current output texture:
						instance->output_textures_free.push_back(instance->output);
						instance->output = {};
					}

					// Take this used texture as current output:
					instance->output = std::move(instance->output_textures_used[i]);

					// Remove this used texture:
					std::swap(instance->output_textures_used[i], instance->output_textures_used.back());
					instance->output_textures_used.pop_back();
					break;
				}
			}
		}

		// Check if any used textures can be freed because their display order is outdated:
		for (size_t i = 0; i < instance->output_textures_used.size(); ++i)
		{
			if (instance->output_textures_used[i].display_order < instance->target_display_order)
			{
				// Remove this used texture:
				instance->output_textures_free.push_back(std::move(instance->output_textures_used[i]));
				std::swap(instance->output_textures_used[i], instance->output_textures_used.back());
				instance->output_textures_used.pop_back();
			}
		}
	}

	void UpdateVideo(VideoInstance* instance, float dt)
	{
		if (instance == nullptr || instance->video == nullptr)
			return;

		bool restart_audio = false;
		if (has_flag(instance->flags, VideoInstance::Flags::Looped) && instance->current_time > instance->video->duration_seconds)
		{
			instance->current_time = 0;
			instance->current_decode_frame = 0;
			restart_audio = true;
		}

		// Search for the exact frame that we should currently be displaying:
		instance->target_display_order = 0;
		for (size_t i = 0; i < instance->video->frame_infos.size(); ++i)
		{
			const Video::FrameInfo& frame_info = instance->video->frame_infos[i];
			if (frame_info.timestamp_seconds <= instance->current_time && frame_info.timestamp_seconds + frame_info.duration_seconds > instance->current_time)
			{
				instance->target_display_order = frame_info.display_order;
				break;
			}
		}

		UpdateDisplayOrderOutput(instance);
		SyncVideoAudio(instance, restart_audio);

		// Important that this is at the end of func:
		if (has_flag(instance->flags, VideoInstance::Flags::Playing))
		{
			instance->current_time += dt;
		}
	}
	bool IsDecodingRequired(const VideoInstance* instance)
	{
		if (!GetDevice()->CheckCapability(GraphicsDeviceCapability::VIDEO_DECODE_H264))
			return false;
		if (instance == nullptr || instance->video == nullptr)
			return false;
		if (instance->current_decode_frame >= (int)instance->video->frame_infos.size())
			return false;
		if (!has_flag(instance->flags, VideoInstance::Flags::InitialFirstFrameDecoded))
			return true;
		// Ensure that we decode into the future so that always the next displayable picture will be available:
		bool found_next_displayable = false;
		for (auto& x : instance->output_textures_used)
		{
			if (x.display_order == instance->target_display_order + 1)
			{
				found_next_displayable = true;
				break;
			}
		}
		if (!found_next_displayable)
		{
			return true;
		}
		return false;
	}
	void DecodeVideo(VideoInstance* instance, CommandList cmd)
	{
		if (!cmd.IsValid())
			return;

		GraphicsDevice* device = GetDevice();
		const Video* video = instance->video;

		while (IsDecodingRequired(instance))
		{
			instance->current_decode_frame = clamp(instance->current_decode_frame, 0, (int)video->frame_infos.size() - 1);

			BeginDecodePicture(instance);

			// Allocate one output texture:
			if (instance->output_textures_free.empty())
			{
				VideoInstance::OutputTexture& output = instance->output_textures_free.emplace_back();
				TextureDesc td;
				td.width = video->width;
				td.height = video->height;
				td.format = Format::R8G8B8A8_UNORM;
				if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
				{
					td.mip_levels = GetMipCount(td.width, td.height);
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

				if (!has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
				{
					// Must create separate YUV texture from DPB in non-coincide mode:
					td.width = video->padded_width;
					td.height = video->padded_height;
					td.format = Format::NV12;
					td.mip_levels = 1;
					td.bind_flags = BindFlag::SHADER_RESOURCE;
					td.misc_flags = ResourceMiscFlag::VIDEO_DECODE_OUTPUT_ONLY;
					td.misc_flags |= ResourceMiscFlag::VIDEO_COMPATIBILITY_H264;
					td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
					success = device->CreateTexture(&td, nullptr, &output.src);
					assert(success);
					device->SetName(&output.src, "VideoInstance::OutputTexture::src");

					Format luminance_format = Format::R8_UNORM;
					ImageAspect luminance_aspect = ImageAspect::LUMINANCE;
					output.src_subresource_luminance = device->CreateSubresource(
						&output.src,
						SubresourceType::SRV,
						0, 1, 0, 1,
						&luminance_format, &luminance_aspect
					);

					Format chrominance_format = Format::R8G8_UNORM;
					ImageAspect chrominance_aspect = ImageAspect::CHROMINANCE;
					output.src_subresource_chrominance = device->CreateSubresource(
						&output.src,
						SubresourceType::SRV,
						0, 1, 0, 1,
						&chrominance_format, &chrominance_aspect
					);
				}
			}
			VideoInstance::OutputTexture decode = std::move(instance->output_textures_free.back());
			instance->output_textures_free.pop_back();
			decode.display_order = video->frame_infos[instance->current_decode_frame].display_order;
			if (has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
			{
				decode.src = instance->dpb.texture;
				decode.src_subresource_luminance = instance->dpb.subresources_luminance[instance->dpb.current_slot];
				decode.src_subresource_chrominance = instance->dpb.subresources_chrominance[instance->dpb.current_slot];
			}

			if (decode.display_order < instance->target_display_order)
			{
				// next decoded is lower display order than we will need, it can be immediately freed after decode
				instance->output_textures_free.push_back(decode);
			}
			else
			{
				// next display order is high enough display order that we will need to use for displaying, so a resolving request is made for it:
				instance->output_textures_resolve_request.push_back(decode);
				instance->output_textures_used.push_back(decode);
			}

			instance->current_decode_frame = clamp(instance->current_decode_frame, 0, (int)video->frame_infos.size() - 1);
			const Video::FrameInfo& frame_info = video->frame_infos[instance->current_decode_frame];

			const h264::SliceHeader* slice_header = (const h264::SliceHeader*)video->slice_header_datas.data() + instance->current_decode_frame;
			const h264::PPS* pps = (const h264::PPS*)video->pps_datas.data() + slice_header->pic_parameter_set_id;
			const h264::SPS* sps = (const h264::SPS*)video->sps_datas.data() + pps->seq_parameter_set_id;

			VideoDecodeOperation decode_operation;
			if (instance->current_decode_frame == 0 || has_flag(instance->flags, VideoInstance::Flags::DecoderReset))
			{
				decode_operation.flags = VideoDecodeOperation::FLAG_SESSION_RESET;
				instance->flags &= ~VideoInstance::Flags::DecoderReset;
			}
			decode_operation.stream = &video->data_stream;
			decode_operation.stream_offset = frame_info.offset;
			decode_operation.stream_size = frame_info.size;
			decode_operation.slice_count = frame_info.slice_count;
			decode_operation.slice_offsets = video->slice_offsets.data() + frame_info.slice_offset_index;
			decode_operation.poc[0] = frame_info.poc;
			decode_operation.poc[1] = frame_info.poc;
			decode_operation.frame_type = frame_info.type;
			decode_operation.reference_priority = frame_info.reference_priority;
			decode_operation.decoded_frame_index = instance->current_decode_frame;
			decode_operation.slice_header = slice_header;
			decode_operation.pps = pps;
			decode_operation.sps = sps;
			decode_operation.current_dpb = instance->dpb.current_slot;
			decode_operation.dpb_reference_count = (uint32_t)instance->dpb.reference_usage.size();
			decode_operation.dpb_reference_slots = instance->dpb.reference_usage.data();
			decode_operation.dpb_poc = instance->dpb.poc_status;
			decode_operation.dpb_framenum = instance->dpb.framenum_status;
			decode_operation.DPB = &instance->dpb.texture;

			if (decode.display_order >= instance->target_display_order || frame_info.reference_priority > 0) // this check can skip decoding for non-reference frame that is a low enough display order
			{
				ImageAspect aspect_luma = ImageAspect::LUMINANCE;
				ImageAspect aspect_chroma = ImageAspect::CHROMINANCE;
				if (has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
				{
					decode_operation.output = nullptr;
					// Ensure that current DPB slot is in DST state:
					if (instance->dpb.resource_states[instance->dpb.current_slot] != ResourceState::VIDEO_DECODE_DPB)
					{
						wi::renderer::PushBarrier(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::VIDEO_DECODE_DPB, 0, instance->dpb.current_slot, &aspect_luma));
						wi::renderer::PushBarrier(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[instance->dpb.current_slot], ResourceState::VIDEO_DECODE_DPB, 0, instance->dpb.current_slot, &aspect_chroma));
						instance->dpb.resource_states[instance->dpb.current_slot] = ResourceState::VIDEO_DECODE_DPB;
					}
					// Ensure that reference frame DPB slots are in SRC state:
					for (size_t i = 0; i < instance->dpb.reference_usage.size(); ++i)
					{
						uint8_t ref = instance->dpb.reference_usage[i];
						if (instance->dpb.resource_states[ref] != ResourceState::VIDEO_DECODE_SRC)
						{
							wi::renderer::PushBarrier(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[ref], ResourceState::VIDEO_DECODE_SRC, 0, ref, &aspect_luma));
							wi::renderer::PushBarrier(GPUBarrier::Image(&instance->dpb.texture, instance->dpb.resource_states[ref], ResourceState::VIDEO_DECODE_SRC, 0, ref, &aspect_chroma));
							instance->dpb.resource_states[ref] = ResourceState::VIDEO_DECODE_SRC;
						}
					}
				}
				else
				{
					// if DPB_AND_OUTPUT_COINCIDE is NOT supported, then DPB is kept always in DPB state, and only the output tex is ever a shader resource:
					decode_operation.output = &decode.src;
					wi::renderer::PushBarrier(GPUBarrier::Image(&decode.src, decode.src.desc.layout, ResourceState::VIDEO_DECODE_DST, -1, -1, &aspect_luma));
					wi::renderer::PushBarrier(GPUBarrier::Image(&decode.src, decode.src.desc.layout, ResourceState::VIDEO_DECODE_DST, -1, -1, &aspect_chroma));
				}
				wi::renderer::FlushBarriers(cmd);

				device->VideoDecode(&instance->decoder, &decode_operation, cmd);

				if (has_flag(instance->decoder.support, VideoDecoderSupportFlags::DPB_AND_OUTPUT_COINCIDE))
				{
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
				}
				else
				{
					// if DPB_AND_OUTPUT_COINCIDE is NOT supported, then DPB is kept always in DPB state, and only the output tex is ever a shader resource:
					GPUBarrier barriers[] = {
						GPUBarrier::Image(&decode.src, ResourceState::VIDEO_DECODE_DST, decode.src.desc.layout, -1, -1, &aspect_luma),
						GPUBarrier::Image(&decode.src, ResourceState::VIDEO_DECODE_DST, decode.src.desc.layout, -1, -1, &aspect_chroma),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);
				}
			}
			else
			{
				//wilog("Video frame decoding skipped entirely due to catching up.");
			}

			EndDecodePicture(instance);

			instance->flags |= VideoInstance::Flags::NeedsResolve;
			instance->flags |= VideoInstance::Flags::InitialFirstFrameDecoded;
			instance->current_decode_frame++;
		}
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

		for (auto& resolve_request : instance->output_textures_resolve_request)
		{
			wi::renderer::YUV_to_RGB(
				resolve_request.src,
				resolve_request.src_subresource_luminance,
				resolve_request.src_subresource_chrominance,
				resolve_request.texture,
				cmd
			);
			if (has_flag(instance->flags, VideoInstance::Flags::Mipmapped))
			{
				wi::renderer::GenerateMipChain(resolve_request.texture, wi::renderer::MIPGENFILTER_LINEAR, cmd);
			}
		}
		instance->output_textures_resolve_request.clear();

		UpdateDisplayOrderOutput(instance);
	}

	void Seek(VideoInstance* instance, float timerSeconds)
	{
		if (instance == nullptr)
			return;
		const Video* video = instance->video;
		if (video == nullptr)
			return;
		int max_iframe = 0;
		for (size_t i = 0; i < video->frame_infos.size(); ++i)
		{
			auto& frame_info = video->frame_infos[i];
			if (frame_info.type == wi::graphics::VideoFrameType::Intra)
			{
				max_iframe = (int)i;
			}
			if (frame_info.timestamp_seconds > timerSeconds)
				break;
		}
		if (instance->current_decode_frame != max_iframe)
		{
			instance->current_decode_frame = max_iframe;
			instance->current_time = video->frame_infos[max_iframe].timestamp_seconds;
			instance->target_display_order = video->frame_infos[max_iframe].display_order;
			instance->flags |= wi::video::VideoInstance::Flags::DecoderReset;
			instance->flags &= ~wi::video::VideoInstance::Flags::InitialFirstFrameDecoded;
			instance->output_textures_free.clear();
			instance->output_textures_used.clear();
			instance->output_textures_resolve_request.clear();
			SyncVideoAudio(instance, true);
		}
	}
}
