#ifndef MINIMP4_H
#define MINIMP4_H
/*
    https://github.com/aspt/mp4
    https://github.com/lieff/minimp4
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#ifdef _WIN32
#define strdup _strdup
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#define MINIMP4_MIN(x, y) ((x) < (y) ? (x) : (y))

/************************************************************************/
/*                  Build configuration                                 */
/************************************************************************/

#define FIX_BAD_ANDROID_META_BOX  1

#define MAX_CHUNKS_DEPTH          64 // Max chunks nesting level

#define MINIMP4_MAX_SPS 32
#define MINIMP4_MAX_PPS 256

#define MINIMP4_TRANSCODE_SPS_ID  1

// Support indexing of MP4 files over 4 GB.
// If disabled, files with 64-bit offset fields is still supported,
// but error signaled if such field contains too big offset
// This switch affect return type of MP4D_frame_offset() function
#define MINIMP4_ALLOW_64BIT       1

#define MP4D_TRACE_SUPPORTED      0 // Debug trace
#define MP4D_TRACE_TIMESTAMPS     1
// Support parsing of supplementary information, not necessary for decoding:
// duration, language, bitrate, metadata tags, etc
#define MP4D_INFO_SUPPORTED       1

// Enable code, which prints to stdout supplementary MP4 information:
#define MP4D_PRINT_INFO_SUPPORTED 0

#define MP4D_AVC_SUPPORTED        1
#define MP4D_HEVC_SUPPORTED       1
#define MP4D_TIMESTAMPS_SUPPORTED 1

// Enable TrackFragmentBaseMediaDecodeTimeBox support
#define MP4D_TFDT_SUPPORT         0

/************************************************************************/
/*          Some values of MP4(E/D)_track_t->object_type_indication     */
/************************************************************************/
// MPEG-4 AAC (all profiles)
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3                  0x40
// MPEG-2 AAC, Main profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_MAIN_PROFILE     0x66
// MPEG-2 AAC, LC profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_LC_PROFILE       0x67
// MPEG-2 AAC, SSR profile
#define MP4_OBJECT_TYPE_AUDIO_ISO_IEC_13818_7_SSR_PROFILE      0x68
// H.264 (AVC) video
#define MP4_OBJECT_TYPE_AVC                                    0x21
// H.265 (HEVC) video
#define MP4_OBJECT_TYPE_HEVC                                   0x23
// http://www.mp4ra.org/object.html 0xC0-E0  && 0xE2 - 0xFE are specified as "user private"
#define MP4_OBJECT_TYPE_USER_PRIVATE                           0xC0

/************************************************************************/
/*          API error codes                                             */
/************************************************************************/
#define MP4E_STATUS_OK                       0
#define MP4E_STATUS_BAD_ARGUMENTS           -1
#define MP4E_STATUS_NO_MEMORY               -2
#define MP4E_STATUS_FILE_WRITE_ERROR        -3
#define MP4E_STATUS_ONLY_ONE_DSI_ALLOWED    -4

/************************************************************************/
/*          Sample kind for MP4E_put_sample()                           */
/************************************************************************/
#define MP4E_SAMPLE_DEFAULT             0   // (beginning of) audio or video frame
#define MP4E_SAMPLE_RANDOM_ACCESS       1   // mark sample as random access point (key frame)
#define MP4E_SAMPLE_CONTINUATION        2   // Not a sample, but continuation of previous sample (new slice)

/************************************************************************/
/*                  Portable 64-bit type definition                     */
/************************************************************************/

#if MINIMP4_ALLOW_64BIT
    typedef uint64_t boxsize_t;
#else
    typedef unsigned int boxsize_t;
#endif
typedef boxsize_t MP4D_file_offset_t;

/************************************************************************/
/*          Some values of MP4D_track_t->handler_type              */
/************************************************************************/
// Video track : 'vide'
#define MP4D_HANDLER_TYPE_VIDE                                  0x76696465
// Audio track : 'soun'
#define MP4D_HANDLER_TYPE_SOUN                                  0x736F756E
// General MPEG-4 systems streams (without specific handler).
// Used for private stream, as suggested in http://www.mp4ra.org/handler.html
#define MP4E_HANDLER_TYPE_GESM                                  0x6765736D


#define HEVC_NAL_VPS 32
#define HEVC_NAL_SPS 33
#define HEVC_NAL_PPS 34
#define HEVC_NAL_BLA_W_LP 16
#define HEVC_NAL_CRA_NUT  21

/************************************************************************/
/*          Data structures                                             */
/************************************************************************/

typedef struct MP4E_mux_tag MP4E_mux_t;

typedef enum
{
    e_audio,
    e_video,
    e_private
} track_media_kind_t;

typedef struct
{
    // MP4 object type code, which defined codec class for the track.
    // See MP4E_OBJECT_TYPE_* values for some codecs
    unsigned object_type_indication;

    // Track language: 3-char ISO 639-2T code: "und", "eng", "rus", "jpn" etc...
    unsigned char language[4];

    track_media_kind_t track_media_kind;

    // 90000 for video, sample rate for audio
    unsigned time_scale;
    unsigned default_duration;

    union
    {
        struct
        {
            // number of channels in the audio track.
            unsigned channelcount;
        } a;

        struct
        {
            int width;
            int height;
        } v;
    } u;

} MP4E_track_t;

typedef struct MP4D_sample_to_chunk_t_tag MP4D_sample_to_chunk_t;

typedef struct
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    // How many 'samples' in the track
    // The 'sample' is MP4 term, denoting audio or video frame
    unsigned sample_count;

    // Decoder-specific info (DSI) data
    unsigned char *dsi;

    // DSI data size
    unsigned dsi_bytes;

    // MP4 object type code
    // case 0x00: return "Forbidden";
    // case 0x01: return "Systems ISO/IEC 14496-1";
    // case 0x02: return "Systems ISO/IEC 14496-1";
    // case 0x20: return "Visual ISO/IEC 14496-2";
    // case 0x40: return "Audio ISO/IEC 14496-3";
    // case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    // case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    // case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    // case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    // case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    // case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    // case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    // case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    // case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    // case 0x69: return "Audio ISO/IEC 13818-3";
    // case 0x6A: return "Visual ISO/IEC 11172-2";
    // case 0x6B: return "Audio ISO/IEC 11172-3";
    // case 0x6C: return "Visual ISO/IEC 10918-1";
    unsigned object_type_indication;

#if MP4D_INFO_SUPPORTED
    /************************************************************************/
    /*                 informational public data                            */
    /************************************************************************/
    // handler_type when present in a media box, is an integer containing one of
    // the following values, or a value from a derived specification:
    // 'vide' Video track
    // 'soun' Audio track
    // 'hint' Hint track
    unsigned handler_type;

    // Track duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Average bitrate, bits per second
    unsigned avg_bitrate_bps;

    // Track language: 3-char ISO 639-2T code: "und", "eng", "rus", "jpn" etc...
    unsigned char language[4];

    // MP4 stream type
    // case 0x00: return "Forbidden";
    // case 0x01: return "ObjectDescriptorStream";
    // case 0x02: return "ClockReferenceStream";
    // case 0x03: return "SceneDescriptionStream";
    // case 0x04: return "VisualStream";
    // case 0x05: return "AudioStream";
    // case 0x06: return "MPEG7Stream";
    // case 0x07: return "IPMPStream";
    // case 0x08: return "ObjectContentInfoStream";
    // case 0x09: return "MPEGJStream";
    unsigned stream_type;

    union
    {
        // for handler_type == 'soun' tracks
        struct
        {
            unsigned channelcount;
            unsigned samplerate_hz;
        } audio;

        // for handler_type == 'vide' tracks
        struct
        {
            unsigned width;
            unsigned height;
        } video;
    } SampleDescription;
#endif

    /************************************************************************/
    /*                 private data: MP4 indexes                            */
    /************************************************************************/
    unsigned *entry_size;

    unsigned sample_to_chunk_count;
    struct MP4D_sample_to_chunk_t_tag *sample_to_chunk;

    unsigned chunk_count;
    MP4D_file_offset_t *chunk_offset;

#if MP4D_TIMESTAMPS_SUPPORTED
    unsigned *timestamp;
    unsigned *duration;
#endif

} MP4D_track_t;

typedef struct MP4D_demux_tag
{
    /************************************************************************/
    /*                 mandatory public data                                */
    /************************************************************************/
    int64_t read_pos;
    int64_t read_size;
    MP4D_track_t *track;
    int (*read_callback)(int64_t offset, void *buffer, size_t size, void *token);
    void *token;

    unsigned track_count; // number of tracks in the movie

#if MP4D_INFO_SUPPORTED
    /************************************************************************/
    /*                 informational public data                            */
    /************************************************************************/
    // Movie duration: 64-bit value split into 2 variables
    unsigned duration_hi;
    unsigned duration_lo;

    // duration scale: duration = timescale*seconds
    unsigned timescale;

    // Metadata tag (optional)
    // Tags provided 'as-is', without any re-encoding
    struct
    {
        unsigned char *title;
        unsigned char *artist;
        unsigned char *album;
        unsigned char *year;
        unsigned char *comment;
        unsigned char *genre;
    } tag;
#endif

} MP4D_demux_t;

struct MP4D_sample_to_chunk_t_tag
{
    unsigned first_chunk;
    unsigned samples_per_chunk;
};

typedef struct
{
    void *sps_cache[MINIMP4_MAX_SPS];
    void *pps_cache[MINIMP4_MAX_PPS];
    int sps_bytes[MINIMP4_MAX_SPS];
    int pps_bytes[MINIMP4_MAX_PPS];

    int map_sps[MINIMP4_MAX_SPS];
    int map_pps[MINIMP4_MAX_PPS];

} h264_sps_id_patcher_t;

typedef struct mp4_h26x_writer_tag
{
#if MINIMP4_TRANSCODE_SPS_ID
    h264_sps_id_patcher_t sps_patcher;
#endif
    MP4E_mux_t *mux;
    int mux_track_id, is_hevc, need_vps, need_sps, need_pps, need_idr;
} mp4_h26x_writer_t;

int mp4_h26x_write_init(mp4_h26x_writer_t *h, MP4E_mux_t *mux, int width, int height, int is_hevc);
void mp4_h26x_write_close(mp4_h26x_writer_t *h);
int mp4_h26x_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int length, unsigned timeStamp90kHz_next);

/************************************************************************/
/*          API                                                         */
/************************************************************************/

/**
*   Parse given input stream as MP4 file. Allocate and store data indexes.
*   return 1 on success, 0 on failure
*   The MP4 indexes may be stored at the end of stream, so this
*   function may parse all stream.
*   It is guaranteed that function will read/seek sequentially,
*   and will never jump back.
*/
int MP4D_open(MP4D_demux_t *mp4, int (*read_callback)(int64_t offset, void *buffer, size_t size, void *token), void *token, int64_t file_size);

/**
*   Return position and size for given sample from given track. The 'sample' is a
*   MP4 term for 'frame'
*
*   frame_bytes [OUT]   - return coded frame size in bytes
*   timestamp [OUT]     - return frame timestamp (in mp4->timescale units)
*   duration [OUT]      - return frame duration (in mp4->timescale units)
*
*   function return offset for the frame
*/
MP4D_file_offset_t MP4D_frame_offset(const MP4D_demux_t *mp4, unsigned int ntrack, unsigned int nsample, unsigned int *frame_bytes, unsigned *timestamp, unsigned *duration);

/**
*   De-allocated memory
*/
void MP4D_close(MP4D_demux_t *mp4);

/**
*   Helper functions to parse mp4.track[ntrack].dsi for H.264 SPS/PPS
*   Return pointer to internal mp4 memory, it must not be free()-ed
*
*   Example: process all SPS in MP4 file:
*       while (sps = MP4D_read_sps(mp4, num_of_avc_track, sps_count, &sps_bytes))
*       {
*           process(sps, sps_bytes);
*           sps_count++;
*       }
*/
const void *MP4D_read_sps(const MP4D_demux_t *mp4, unsigned int ntrack, int nsps, int *sps_bytes);
const void *MP4D_read_pps(const MP4D_demux_t *mp4, unsigned int ntrack, int npps, int *pps_bytes);

#if MP4D_PRINT_INFO_SUPPORTED
/**
*   Print MP4 information to stdout.
*   Uses printf() as well as floating-point functions
*   Given as implementation example and for test purposes
*/
void MP4D_printf_info(const MP4D_demux_t *mp4);
#endif

/**
*   Allocates and initialize mp4 multiplexor
*   Given file handler is transparent to the MP4 library, and used only as
*   argument for given fwrite_callback() function.  By appropriate definition
*   of callback function application may use any other file output API (for
*   example C++ streams, or Win32 file functions)
*
*   return multiplexor handle on success; NULL on failure
*/
MP4E_mux_t *MP4E_open(int sequential_mode_flag, int enable_fragmentation, void *token,
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token));

/**
*   Add new track
*   The track_data parameter does not referred by the multiplexer after function
*   return, and may be allocated in short-time memory. The dsi member of
*   track_data parameter is mandatory.
*
*   return ID of added track, or error code MP4E_STATUS_*
*/
int MP4E_add_track(MP4E_mux_t *mux, const MP4E_track_t *track_data);

/**
*   Add new sample to specified track
*   The tracks numbered starting with 0, according to order of MP4E_add_track() calls
*   'kind' is one of MP4E_SAMPLE_... defines
*
*   return error code MP4E_STATUS_*
*
*   Example:
*       MP4E_put_sample(mux, 0, data, data_bytes, duration, MP4E_SAMPLE_DEFAULT);
*/
int MP4E_put_sample(MP4E_mux_t *mux, int track_num, const void *data, int data_bytes, int duration, int kind);

/**
*   Finalize MP4 file, de-allocated memory, and closes MP4 multiplexer.
*   The close operation takes a time and disk space, since it writes MP4 file
*   indexes.  Please note that this function does not closes file handle,
*   which was passed to open function.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_close(MP4E_mux_t *mux);

/**
*   Set Decoder Specific Info (DSI)
*   Can be used for audio and private tracks.
*   MUST be used for AAC track.
*   Only one DSI can be set. It is an error to set DSI again
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_dsi(MP4E_mux_t *mux, int track_id, const void *dsi, int bytes);

/**
*   Set VPS data. MUST be used for HEVC (H.265) track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_vps(MP4E_mux_t *mux, int track_id, const void *vps, int bytes);

/**
*   Set SPS data. MUST be used for AVC (H.264) track. Up to 32 different SPS can be used in one track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_sps(MP4E_mux_t *mux, int track_id, const void *sps, int bytes);

/**
*   Set PPS data. MUST be used for AVC (H.264) track. Up to 256 different PPS can be used in one track.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_pps(MP4E_mux_t *mux, int track_id, const void *pps, int bytes);

/**
*   Set or replace ASCII test comment for the file. Set comment to NULL to remove comment.
*
*   return error code MP4E_STATUS_*
*/
int MP4E_set_text_comment(MP4E_mux_t *mux, const char *comment);

#ifdef __cplusplus
}
#endif
#endif //MINIMP4_H

#if defined(MINIMP4_IMPLEMENTATION) && !defined(MINIMP4_IMPLEMENTATION_GUARD)
#define MINIMP4_IMPLEMENTATION_GUARD

#define FOUR_CHAR_INT(a, b, c, d) (((uint32_t)(a) << 24) | ((b) << 16) | ((c) << 8) | (d))
enum
{
    BOX_co64    = FOUR_CHAR_INT( 'c', 'o', '6', '4' ),//ChunkLargeOffsetAtomType
    BOX_stco    = FOUR_CHAR_INT( 's', 't', 'c', 'o' ),//ChunkOffsetAtomType
    BOX_crhd    = FOUR_CHAR_INT( 'c', 'r', 'h', 'd' ),//ClockReferenceMediaHeaderAtomType
    BOX_ctts    = FOUR_CHAR_INT( 'c', 't', 't', 's' ),//CompositionOffsetAtomType
    BOX_cprt    = FOUR_CHAR_INT( 'c', 'p', 'r', 't' ),//CopyrightAtomType
    BOX_url_    = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),//DataEntryURLAtomType
    BOX_urn_    = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),//DataEntryURNAtomType
    BOX_dinf    = FOUR_CHAR_INT( 'd', 'i', 'n', 'f' ),//DataInformationAtomType
    BOX_dref    = FOUR_CHAR_INT( 'd', 'r', 'e', 'f' ),//DataReferenceAtomType
    BOX_stdp    = FOUR_CHAR_INT( 's', 't', 'd', 'p' ),//DegradationPriorityAtomType
    BOX_edts    = FOUR_CHAR_INT( 'e', 'd', 't', 's' ),//EditAtomType
    BOX_elst    = FOUR_CHAR_INT( 'e', 'l', 's', 't' ),//EditListAtomType
    BOX_uuid    = FOUR_CHAR_INT( 'u', 'u', 'i', 'd' ),//ExtendedAtomType
    BOX_free    = FOUR_CHAR_INT( 'f', 'r', 'e', 'e' ),//FreeSpaceAtomType
    BOX_hdlr    = FOUR_CHAR_INT( 'h', 'd', 'l', 'r' ),//HandlerAtomType
    BOX_hmhd    = FOUR_CHAR_INT( 'h', 'm', 'h', 'd' ),//HintMediaHeaderAtomType
    BOX_hint    = FOUR_CHAR_INT( 'h', 'i', 'n', 't' ),//HintTrackReferenceAtomType
    BOX_mdia    = FOUR_CHAR_INT( 'm', 'd', 'i', 'a' ),//MediaAtomType
    BOX_mdat    = FOUR_CHAR_INT( 'm', 'd', 'a', 't' ),//MediaDataAtomType
    BOX_mdhd    = FOUR_CHAR_INT( 'm', 'd', 'h', 'd' ),//MediaHeaderAtomType
    BOX_minf    = FOUR_CHAR_INT( 'm', 'i', 'n', 'f' ),//MediaInformationAtomType
    BOX_moov    = FOUR_CHAR_INT( 'm', 'o', 'o', 'v' ),//MovieAtomType
    BOX_mvhd    = FOUR_CHAR_INT( 'm', 'v', 'h', 'd' ),//MovieHeaderAtomType
    BOX_stsd    = FOUR_CHAR_INT( 's', 't', 's', 'd' ),//SampleDescriptionAtomType
    BOX_stsz    = FOUR_CHAR_INT( 's', 't', 's', 'z' ),//SampleSizeAtomType
    BOX_stz2    = FOUR_CHAR_INT( 's', 't', 'z', '2' ),//CompactSampleSizeAtomType
    BOX_stbl    = FOUR_CHAR_INT( 's', 't', 'b', 'l' ),//SampleTableAtomType
    BOX_stsc    = FOUR_CHAR_INT( 's', 't', 's', 'c' ),//SampleToChunkAtomType
    BOX_stsh    = FOUR_CHAR_INT( 's', 't', 's', 'h' ),//ShadowSyncAtomType
    BOX_skip    = FOUR_CHAR_INT( 's', 'k', 'i', 'p' ),//SkipAtomType
    BOX_smhd    = FOUR_CHAR_INT( 's', 'm', 'h', 'd' ),//SoundMediaHeaderAtomType
    BOX_stss    = FOUR_CHAR_INT( 's', 't', 's', 's' ),//SyncSampleAtomType
    BOX_stts    = FOUR_CHAR_INT( 's', 't', 't', 's' ),//TimeToSampleAtomType
    BOX_trak    = FOUR_CHAR_INT( 't', 'r', 'a', 'k' ),//TrackAtomType
    BOX_tkhd    = FOUR_CHAR_INT( 't', 'k', 'h', 'd' ),//TrackHeaderAtomType
    BOX_tref    = FOUR_CHAR_INT( 't', 'r', 'e', 'f' ),//TrackReferenceAtomType
    BOX_udta    = FOUR_CHAR_INT( 'u', 'd', 't', 'a' ),//UserDataAtomType
    BOX_vmhd    = FOUR_CHAR_INT( 'v', 'm', 'h', 'd' ),//VideoMediaHeaderAtomType
    BOX_url     = FOUR_CHAR_INT( 'u', 'r', 'l', ' ' ),
    BOX_urn     = FOUR_CHAR_INT( 'u', 'r', 'n', ' ' ),

    BOX_gnrv    = FOUR_CHAR_INT( 'g', 'n', 'r', 'v' ),//GenericVisualSampleEntryAtomType
    BOX_gnra    = FOUR_CHAR_INT( 'g', 'n', 'r', 'a' ),//GenericAudioSampleEntryAtomType

    //V2 atoms
    BOX_ftyp    = FOUR_CHAR_INT( 'f', 't', 'y', 'p' ),//FileTypeAtomType
    BOX_padb    = FOUR_CHAR_INT( 'p', 'a', 'd', 'b' ),//PaddingBitsAtomType

    //MP4 Atoms
    BOX_sdhd    = FOUR_CHAR_INT( 's', 'd', 'h', 'd' ),//SceneDescriptionMediaHeaderAtomType
    BOX_dpnd    = FOUR_CHAR_INT( 'd', 'p', 'n', 'd' ),//StreamDependenceAtomType
    BOX_iods    = FOUR_CHAR_INT( 'i', 'o', 'd', 's' ),//ObjectDescriptorAtomType
    BOX_odhd    = FOUR_CHAR_INT( 'o', 'd', 'h', 'd' ),//ObjectDescriptorMediaHeaderAtomType
    BOX_mpod    = FOUR_CHAR_INT( 'm', 'p', 'o', 'd' ),//ODTrackReferenceAtomType
    BOX_nmhd    = FOUR_CHAR_INT( 'n', 'm', 'h', 'd' ),//MPEGMediaHeaderAtomType
    BOX_esds    = FOUR_CHAR_INT( 'e', 's', 'd', 's' ),//ESDAtomType
    BOX_sync    = FOUR_CHAR_INT( 's', 'y', 'n', 'c' ),//OCRReferenceAtomType
    BOX_ipir    = FOUR_CHAR_INT( 'i', 'p', 'i', 'r' ),//IPIReferenceAtomType
    BOX_mp4s    = FOUR_CHAR_INT( 'm', 'p', '4', 's' ),//MPEGSampleEntryAtomType
    BOX_mp4a    = FOUR_CHAR_INT( 'm', 'p', '4', 'a' ),//MPEGAudioSampleEntryAtomType
    BOX_mp4v    = FOUR_CHAR_INT( 'm', 'p', '4', 'v' ),//MPEGVisualSampleEntryAtomType

    // http://www.itscj.ipsj.or.jp/sc29/open/29view/29n7644t.doc
    BOX_avc1    = FOUR_CHAR_INT( 'a', 'v', 'c', '1' ),
    BOX_avc2    = FOUR_CHAR_INT( 'a', 'v', 'c', '2' ),
    BOX_svc1    = FOUR_CHAR_INT( 's', 'v', 'c', '1' ),
    BOX_avcC    = FOUR_CHAR_INT( 'a', 'v', 'c', 'C' ),
    BOX_svcC    = FOUR_CHAR_INT( 's', 'v', 'c', 'C' ),
    BOX_btrt    = FOUR_CHAR_INT( 'b', 't', 'r', 't' ),
    BOX_m4ds    = FOUR_CHAR_INT( 'm', '4', 'd', 's' ),
    BOX_seib    = FOUR_CHAR_INT( 's', 'e', 'i', 'b' ),

    // H264/HEVC
    BOX_hev1    = FOUR_CHAR_INT( 'h', 'e', 'v', '1' ),
    BOX_hvc1    = FOUR_CHAR_INT( 'h', 'v', 'c', '1' ),
    BOX_hvcC    = FOUR_CHAR_INT( 'h', 'v', 'c', 'C' ),

    //3GPP atoms
    BOX_samr    = FOUR_CHAR_INT( 's', 'a', 'm', 'r' ),//AMRSampleEntryAtomType
    BOX_sawb    = FOUR_CHAR_INT( 's', 'a', 'w', 'b' ),//WB_AMRSampleEntryAtomType
    BOX_damr    = FOUR_CHAR_INT( 'd', 'a', 'm', 'r' ),//AMRConfigAtomType
    BOX_s263    = FOUR_CHAR_INT( 's', '2', '6', '3' ),//H263SampleEntryAtomType
    BOX_d263    = FOUR_CHAR_INT( 'd', '2', '6', '3' ),//H263ConfigAtomType

    //V2 atoms - Movie Fragments
    BOX_mvex    = FOUR_CHAR_INT( 'm', 'v', 'e', 'x' ),//MovieExtendsAtomType
    BOX_trex    = FOUR_CHAR_INT( 't', 'r', 'e', 'x' ),//TrackExtendsAtomType
    BOX_moof    = FOUR_CHAR_INT( 'm', 'o', 'o', 'f' ),//MovieFragmentAtomType
    BOX_mfhd    = FOUR_CHAR_INT( 'm', 'f', 'h', 'd' ),//MovieFragmentHeaderAtomType
    BOX_traf    = FOUR_CHAR_INT( 't', 'r', 'a', 'f' ),//TrackFragmentAtomType
    BOX_tfhd    = FOUR_CHAR_INT( 't', 'f', 'h', 'd' ),//TrackFragmentHeaderAtomType
    BOX_tfdt    = FOUR_CHAR_INT( 't', 'f', 'd', 't' ),//TrackFragmentBaseMediaDecodeTimeBox
    BOX_trun    = FOUR_CHAR_INT( 't', 'r', 'u', 'n' ),//TrackFragmentRunAtomType
    BOX_mehd    = FOUR_CHAR_INT( 'm', 'e', 'h', 'd' ),//MovieExtendsHeaderBox

    // Object Descriptors (OD) data coding
    // These takes only 1 byte; this implementation translate <od_tag> to
    // <od_tag> + OD_BASE to keep API uniform and safe for string functions
    OD_BASE    = FOUR_CHAR_INT( '$', '$', '$', '0' ),//
    OD_ESD     = FOUR_CHAR_INT( '$', '$', '$', '3' ),//SDescriptor_Tag
    OD_DCD     = FOUR_CHAR_INT( '$', '$', '$', '4' ),//DecoderConfigDescriptor_Tag
    OD_DSI     = FOUR_CHAR_INT( '$', '$', '$', '5' ),//DecoderSpecificInfo_Tag
    OD_SLC     = FOUR_CHAR_INT( '$', '$', '$', '6' ),//SLConfigDescriptor_Tag

    BOX_meta   = FOUR_CHAR_INT( 'm', 'e', 't', 'a' ),
    BOX_ilst   = FOUR_CHAR_INT( 'i', 'l', 's', 't' ),

    // Metagata tags, see http://atomicparsley.sourceforge.net/mpeg-4files.html
    BOX_calb    = FOUR_CHAR_INT( '\xa9', 'a', 'l', 'b'),    // album
    BOX_cart    = FOUR_CHAR_INT( '\xa9', 'a', 'r', 't'),    // artist
    BOX_aART    = FOUR_CHAR_INT( 'a', 'A', 'R', 'T' ),      // album artist
    BOX_ccmt    = FOUR_CHAR_INT( '\xa9', 'c', 'm', 't'),    // comment
    BOX_cday    = FOUR_CHAR_INT( '\xa9', 'd', 'a', 'y'),    // year (as string)
    BOX_cnam    = FOUR_CHAR_INT( '\xa9', 'n', 'a', 'm'),    // title
    BOX_cgen    = FOUR_CHAR_INT( '\xa9', 'g', 'e', 'n'),    // custom genre (as string or as byte!)
    BOX_trkn    = FOUR_CHAR_INT( 't', 'r', 'k', 'n'),       // track number (byte)
    BOX_disk    = FOUR_CHAR_INT( 'd', 'i', 's', 'k'),       // disk number (byte)
    BOX_cwrt    = FOUR_CHAR_INT( '\xa9', 'w', 'r', 't'),    // composer
    BOX_ctoo    = FOUR_CHAR_INT( '\xa9', 't', 'o', 'o'),    // encoder
    BOX_tmpo    = FOUR_CHAR_INT( 't', 'm', 'p', 'o'),       // bpm (byte)
    BOX_cpil    = FOUR_CHAR_INT( 'c', 'p', 'i', 'l'),       // compilation (byte)
    BOX_covr    = FOUR_CHAR_INT( 'c', 'o', 'v', 'r'),       // cover art (JPEG/PNG)
    BOX_rtng    = FOUR_CHAR_INT( 'r', 't', 'n', 'g'),       // rating/advisory (byte)
    BOX_cgrp    = FOUR_CHAR_INT( '\xa9', 'g', 'r', 'p'),    // grouping
    BOX_stik    = FOUR_CHAR_INT( 's', 't', 'i', 'k'),       // stik (byte)  0 = Movie   1 = Normal  2 = Audiobook  5 = Whacked Bookmark  6 = Music Video  9 = Short Film  10 = TV Show  11 = Booklet  14 = Ringtone
    BOX_pcst    = FOUR_CHAR_INT( 'p', 'c', 's', 't'),       // podcast (byte)
    BOX_catg    = FOUR_CHAR_INT( 'c', 'a', 't', 'g'),       // category
    BOX_keyw    = FOUR_CHAR_INT( 'k', 'e', 'y', 'w'),       // keyword
    BOX_purl    = FOUR_CHAR_INT( 'p', 'u', 'r', 'l'),       // podcast URL (byte)
    BOX_egid    = FOUR_CHAR_INT( 'e', 'g', 'i', 'd'),       // episode global unique ID (byte)
    BOX_desc    = FOUR_CHAR_INT( 'd', 'e', 's', 'c'),       // description
    BOX_clyr    = FOUR_CHAR_INT( '\xa9', 'l', 'y', 'r'),    // lyrics (may be > 255 bytes)
    BOX_tven    = FOUR_CHAR_INT( 't', 'v', 'e', 'n'),       // tv episode number
    BOX_tves    = FOUR_CHAR_INT( 't', 'v', 'e', 's'),       // tv episode (byte)
    BOX_tvnn    = FOUR_CHAR_INT( 't', 'v', 'n', 'n'),       // tv network name
    BOX_tvsh    = FOUR_CHAR_INT( 't', 'v', 's', 'h'),       // tv show name
    BOX_tvsn    = FOUR_CHAR_INT( 't', 'v', 's', 'n'),       // tv season (byte)
    BOX_purd    = FOUR_CHAR_INT( 'p', 'u', 'r', 'd'),       // purchase date
    BOX_pgap    = FOUR_CHAR_INT( 'p', 'g', 'a', 'p'),       // Gapless Playback (byte)

    //BOX_aart   = FOUR_CHAR_INT( 'a', 'a', 'r', 't' ),     // Album artist
    BOX_cART    = FOUR_CHAR_INT( '\xa9', 'A', 'R', 'T'),    // artist
    BOX_gnre    = FOUR_CHAR_INT( 'g', 'n', 'r', 'e'),

    // 3GPP metatags  (http://cpansearch.perl.org/src/JHAR/MP4-Info-1.12/Info.pm)
    BOX_auth    = FOUR_CHAR_INT( 'a', 'u', 't', 'h'),       // author
    BOX_titl    = FOUR_CHAR_INT( 't', 'i', 't', 'l'),       // title
    BOX_dscp    = FOUR_CHAR_INT( 'd', 's', 'c', 'p'),       // description
    BOX_perf    = FOUR_CHAR_INT( 'p', 'e', 'r', 'f'),       // performer
    BOX_mean    = FOUR_CHAR_INT( 'm', 'e', 'a', 'n'),       //
    BOX_name    = FOUR_CHAR_INT( 'n', 'a', 'm', 'e'),       //
    BOX_data    = FOUR_CHAR_INT( 'd', 'a', 't', 'a'),       //

    // these from http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2008-September/053151.html
    BOX_albm    = FOUR_CHAR_INT( 'a', 'l', 'b', 'm'),      // album
    BOX_yrrc    = FOUR_CHAR_INT( 'y', 'r', 'r', 'c')       // album
};

// Video track : 'vide'
#define MP4E_HANDLER_TYPE_VIDE                                  0x76696465
// Audio track : 'soun'
#define MP4E_HANDLER_TYPE_SOUN                                  0x736F756E
// General MPEG-4 systems streams (without specific handler).
// Used for private stream, as suggested in http://www.mp4ra.org/handler.html
#define MP4E_HANDLER_TYPE_GESM                                  0x6765736D

typedef struct
{
    boxsize_t size;
    boxsize_t offset;
    unsigned duration;
    unsigned flag_random_access;
} sample_t;

typedef struct {
    unsigned char *data;
    int bytes;
    int capacity;
} minimp4_vector_t;

typedef struct
{
    MP4E_track_t info;
    minimp4_vector_t smpl;  // sample descriptor
    minimp4_vector_t pending_sample;

    minimp4_vector_t vsps;  // or dsi for audio
    minimp4_vector_t vpps;  // not used for audio
    minimp4_vector_t vvps;  // used for HEVC

} track_t;

typedef struct MP4E_mux_tag
{
    minimp4_vector_t tracks;

    int64_t write_pos;
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token);
    void *token;
    char *text_comment;

    int sequential_mode_flag;
    int enable_fragmentation; // flag, indicating streaming-friendly 'fragmentation' mode
    int fragments_count;      // # of fragments in 'fragmentation' mode

} MP4E_mux_t;

static const unsigned char box_ftyp[] = {
#if 1
    0,0,0,0x18,'f','t','y','p',
    'm','p','4','2',0,0,0,0,
    'm','p','4','2','i','s','o','m',
#else
    // as in ffmpeg
    0,0,0,0x20,'f','t','y','p',
    'i','s','o','m',0,0,2,0,
    'm','p','4','1','i','s','o','m',
    'i','s','o','2','a','v','c','1',
#endif
};

/**
*   Endian-independent byte-write macros
*/
#define WR(x, n) *p++ = (unsigned char)((x) >> 8*n)
#define WRITE_1(x) WR(x, 0);
#define WRITE_2(x) WR(x, 1); WR(x, 0);
#define WRITE_3(x) WR(x, 2); WR(x, 1); WR(x, 0);
#define WRITE_4(x) WR(x, 3); WR(x, 2); WR(x, 1); WR(x, 0);
#define WR4(p, x) (p)[0] = (char)((x) >> 8*3); (p)[1] = (char)((x) >> 8*2); (p)[2] = (char)((x) >> 8*1); (p)[3] = (char)((x));

// Finish atom: update atom size field
#define END_ATOM --stack; WR4((unsigned char*)*stack, p - *stack);

// Initiate atom: save position of size field on stack
#define ATOM(x)  *stack++ = p; p += 4; WRITE_4(x);

// Atom with 'FullAtomVersionFlags' field
#define ATOM_FULL(x, flag)  ATOM(x); WRITE_4(flag);

#define ERR(func) { int err = func; if (err) return err; }

/**
    Allocate vector with given size, return 1 on success, 0 on fail
*/
static int minimp4_vector_init(minimp4_vector_t *h, int capacity)
{
    h->bytes = 0;
    h->capacity = capacity;
    h->data = capacity ? (unsigned char *)malloc(capacity) : NULL;
    return !capacity || !!h->data;
}

/**
    Deallocates vector memory
*/
static void minimp4_vector_reset(minimp4_vector_t *h)
{
    if (h->data)
        free(h->data);
    memset(h, 0, sizeof(minimp4_vector_t));
}

/**
    Reallocate vector memory to the given size
*/
static int minimp4_vector_grow(minimp4_vector_t *h, int bytes)
{
    void *p;
    int new_size = h->capacity*2 + 1024;
    if (new_size < h->capacity + bytes)
        new_size = h->capacity + bytes + 1024;
    p = realloc(h->data, new_size);
    if (!p)
        return 0;
    h->data = (unsigned char*)p;
    h->capacity = new_size;
    return 1;
}

/**
    Allocates given number of bytes at the end of vector data, increasing
    vector memory if necessary.
    Return allocated memory.
*/
static unsigned char *minimp4_vector_alloc_tail(minimp4_vector_t *h, int bytes)
{
    unsigned char *p;
    if (!h->data && !minimp4_vector_init(h, 2*bytes + 1024))
        return NULL;
    if ((h->capacity - h->bytes) < bytes && !minimp4_vector_grow(h, bytes))
        return NULL;
    assert(h->data);
    assert((h->capacity - h->bytes) >= bytes);
    p = h->data + h->bytes;
    h->bytes += bytes;
    return p;
}

/**
    Append data to the end of the vector (accumulate ot enqueue)
*/
static unsigned char *minimp4_vector_put(minimp4_vector_t *h, const void *buf, int bytes)
{
    unsigned char *tail = minimp4_vector_alloc_tail(h, bytes);
    if (tail)
        memcpy(tail, buf, bytes);
    return tail;
}

/**
*   Allocates and initialize mp4 multiplexer
*   return multiplexor handle on success; NULL on failure
*/
MP4E_mux_t *MP4E_open(int sequential_mode_flag, int enable_fragmentation, void *token,
    int (*write_callback)(int64_t offset, const void *buffer, size_t size, void *token))
{
    if (write_callback(0, box_ftyp, sizeof(box_ftyp), token)) // Write fixed header: 'ftyp' box
        return 0;
    MP4E_mux_t *mux = (MP4E_mux_t*)malloc(sizeof(MP4E_mux_t));
    if (!mux)
        return mux;
    mux->sequential_mode_flag = sequential_mode_flag || enable_fragmentation;
    mux->enable_fragmentation = enable_fragmentation;
    mux->fragments_count = 0;
    mux->write_callback = write_callback;
    mux->token = token;
    mux->text_comment = NULL;
    mux->write_pos = sizeof(box_ftyp);

    if (!mux->sequential_mode_flag)
    {   // Write filler, which would be updated later
        if (mux->write_callback(mux->write_pos, box_ftyp, 8, mux->token))
        {
            free(mux);
            return 0;
        }
        mux->write_pos += 16; // box_ftyp + box_free for 32bit or 64bit size encoding
    }
    minimp4_vector_init(&mux->tracks, 2*sizeof(track_t));
    return mux;
}

/**
*   Add new track
*/
int MP4E_add_track(MP4E_mux_t *mux, const MP4E_track_t *track_data)
{
    track_t *tr;
    int ntr = mux->tracks.bytes / sizeof(track_t);

    if (!mux || !track_data)
        return MP4E_STATUS_BAD_ARGUMENTS;

    tr = (track_t*)minimp4_vector_alloc_tail(&mux->tracks, sizeof(track_t));
    if (!tr)
        return MP4E_STATUS_NO_MEMORY;
    memset(tr, 0, sizeof(track_t));
    memcpy(&tr->info, track_data, sizeof(*track_data));
    if (!minimp4_vector_init(&tr->smpl, 256))
        return MP4E_STATUS_NO_MEMORY;
    minimp4_vector_init(&tr->vsps, 0);
    minimp4_vector_init(&tr->vpps, 0);
    minimp4_vector_init(&tr->pending_sample, 0);
    return ntr;
}

static const unsigned char *next_dsi(const unsigned char *p, const unsigned char *end, int *bytes)
{
    if (p < end + 2)
    {
        *bytes = p[0]*256 + p[1];
        return p + 2;
    } else
        return NULL;
}

static int append_mem(minimp4_vector_t *v, const void *mem, int bytes)
{
    int i;
    unsigned char size[2];
    const unsigned char *p = v->data;
    for (i = 0; i + 2 < v->bytes;)
    {
        int cb = p[i]*256 + p[i + 1];
        if (cb == bytes && !memcmp(p + i + 2, mem, cb))
            return 1;
        i += 2 + cb;
    }
    size[0] = bytes >> 8;
    size[1] = bytes;
    return minimp4_vector_put(v, size, 2) && minimp4_vector_put(v, mem, bytes);
}

static int items_count(minimp4_vector_t *v)
{
    int i, count = 0;
    const unsigned char *p = v->data;
    for (i = 0; i + 2 < v->bytes;)
    {
        int cb = p[i]*256 + p[i + 1];
        count++;
        i += 2 + cb;
    }
    return count;
}

int MP4E_set_dsi(MP4E_mux_t *mux, int track_id, const void *dsi, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_audio ||
           tr->info.track_media_kind == e_private);
    if (tr->vsps.bytes)
        return MP4E_STATUS_ONLY_ONE_DSI_ALLOWED;   // only one DSI allowed
    return append_mem(&tr->vsps, dsi, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_vps(MP4E_mux_t *mux, int track_id, const void *vps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vvps, vps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_sps(MP4E_mux_t *mux, int track_id, const void *sps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vsps, sps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

int MP4E_set_pps(MP4E_mux_t *mux, int track_id, const void *pps, int bytes)
{
    track_t* tr = ((track_t*)mux->tracks.data) + track_id;
    assert(tr->info.track_media_kind == e_video);
    return append_mem(&tr->vpps, pps, bytes) ? MP4E_STATUS_OK : MP4E_STATUS_NO_MEMORY;
}

static unsigned get_duration(const track_t *tr)
{
    unsigned i, sum_duration = 0;
    const sample_t *s = (const sample_t *)tr->smpl.data;
    for (i = 0; i < tr->smpl.bytes/sizeof(sample_t); i++)
    {
        sum_duration += s[i].duration;
    }
    return sum_duration;
}

static int write_pending_data(MP4E_mux_t *mux, track_t *tr)
{
    // if have pending sample && have at least one sample in the index
    if (tr->pending_sample.bytes > 0 && tr->smpl.bytes >= sizeof(sample_t))
    {
        // Complete pending sample
        sample_t *smpl_desc;
        unsigned char base[8], *p = base;

        assert(mux->sequential_mode_flag);

        // Write each sample to a separate atom
        assert(mux->sequential_mode_flag);      // Separate atom needed for sequential_mode only
        WRITE_4(tr->pending_sample.bytes + 8);
        WRITE_4(BOX_mdat);
        ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
        mux->write_pos += p - base;

        // Update sample descriptor with size and offset
        smpl_desc = ((sample_t*)minimp4_vector_alloc_tail(&tr->smpl, 0)) - 1;
        smpl_desc->size = tr->pending_sample.bytes;
        smpl_desc->offset = (boxsize_t)mux->write_pos;

        // Write data
        ERR(mux->write_callback(mux->write_pos, tr->pending_sample.data, tr->pending_sample.bytes, mux->token));
        mux->write_pos += tr->pending_sample.bytes;

        // reset buffer
        tr->pending_sample.bytes = 0;
    }
    return MP4E_STATUS_OK;
}

static int add_sample_descriptor(MP4E_mux_t *mux, track_t *tr, int data_bytes, int duration, int kind)
{
    sample_t smp;
    smp.size = data_bytes;
    smp.offset = (boxsize_t)mux->write_pos;
    smp.duration = (duration ? duration : tr->info.default_duration);
    smp.flag_random_access = (kind == MP4E_SAMPLE_RANDOM_ACCESS);
    return NULL != minimp4_vector_put(&tr->smpl, &smp, sizeof(sample_t));
}

static int mp4e_flush_index(MP4E_mux_t *mux);

/**
*   Write Movie Fragment: 'moof' box
*/
static int mp4e_write_fragment_header(MP4E_mux_t *mux, int track_num, int data_bytes, int duration, int kind
#if MP4D_TFDT_SUPPORT
, uint64_t timestamp
#endif
)
{
    unsigned char base[888], *p = base;
    unsigned char *stack_base[20]; // atoms nesting stack
    unsigned char **stack = stack_base;
    unsigned char *pdata_offset;
    unsigned flags;
    enum
    {
        default_sample_duration_present = 0x000008,
        default_sample_flags_present = 0x000020,
    } e;

    track_t *tr = ((track_t*)mux->tracks.data) + track_num;

    ATOM(BOX_moof)
        ATOM_FULL(BOX_mfhd, 0)
            WRITE_4(mux->fragments_count);  // start from 1
        END_ATOM
        ATOM(BOX_traf)
            flags = 0;
            if (tr->info.track_media_kind == e_video)
                flags |= 0x20;          // default-sample-flags-present
            else
                flags |= 0x08;          // default-sample-duration-present
            flags =  (tr->info.track_media_kind == e_video) ? 0x20020 : 0x20008;

            ATOM_FULL(BOX_tfhd, flags)
                WRITE_4(track_num + 1); // track_ID
                if (tr->info.track_media_kind == e_video)
                {
                    WRITE_4(0x1010000); // default_sample_flags
                } else
                {
                    WRITE_4(duration);
                }
            END_ATOM
            #if MP4D_TFDT_SUPPORT
            ATOM_FULL(BOX_tfdt, 0x01000000) // version 1
                WRITE_4(timestamp >> 32); // upper timestamp
                WRITE_4(timestamp & 0xffffffff); // lower timestamp
            END_ATOM
            #endif
            if (tr->info.track_media_kind == e_audio)
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;  // save ptr to data_offset
                    WRITE_4(data_bytes);// sample_size
                END_ATOM
            } else if (kind == MP4E_SAMPLE_RANDOM_ACCESS)
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x004;         // first-sample-flags-present
                flags |= 0x100;         // sample-duration-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;   // save ptr to data_offset
                    WRITE_4(0x2000000); // first_sample_flags
                    WRITE_4(duration);  // sample_duration
                    WRITE_4(data_bytes);// sample_size
                END_ATOM
            } else
            {
                flags  = 0;
                flags |= 0x001;         // data-offset-present
                flags |= 0x100;         // sample-duration-present
                flags |= 0x200;         // sample-size-present
                ATOM_FULL(BOX_trun, flags)
                    WRITE_4(1);         // sample_count
                    pdata_offset = p; p += 4;   // save ptr to data_offset
                    WRITE_4(duration);  // sample_duration
                    WRITE_4(data_bytes);// sample_size
                END_ATOM
            }
        END_ATOM
    END_ATOM
    WR4(pdata_offset, (p - base) + 8);

    ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
    mux->write_pos += p - base;
    return MP4E_STATUS_OK;
}

static int mp4e_write_mdat_box(MP4E_mux_t *mux, uint32_t size)
{
    unsigned char base[8], *p = base;
    WRITE_4(size);
    WRITE_4(BOX_mdat);
    ERR(mux->write_callback(mux->write_pos, base, p - base, mux->token));
    mux->write_pos += p - base;
    return MP4E_STATUS_OK;
}

/**
*   Add new sample to specified track
*/
int MP4E_put_sample(MP4E_mux_t *mux, int track_num, const void *data, int data_bytes, int duration, int kind)
{
    track_t *tr;
    if (!mux || !data)
        return MP4E_STATUS_BAD_ARGUMENTS;
    tr = ((track_t*)mux->tracks.data) + track_num;

    if (mux->enable_fragmentation)
    {
        #if MP4D_TFDT_SUPPORT
        // NOTE: assume a constant `duration` to calculate current timestamp
        uint64_t timestamp = (uint64_t)mux->fragments_count * duration;
        #endif
        if (!mux->fragments_count++)
            ERR(mp4e_flush_index(mux)); // write file headers before 1st sample
        // write MOOF + MDAT + sample data
        #if MP4D_TFDT_SUPPORT
        ERR(mp4e_write_fragment_header(mux, track_num, data_bytes, duration, kind, timestamp));
        #else
        ERR(mp4e_write_fragment_header(mux, track_num, data_bytes, duration, kind));
        #endif
        // write MDAT box for each sample
        ERR(mp4e_write_mdat_box(mux, data_bytes + 8));
        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
        return MP4E_STATUS_OK;
    }

    if (kind != MP4E_SAMPLE_CONTINUATION)
    {
        if (mux->sequential_mode_flag)
            ERR(write_pending_data(mux, tr));
        if (!add_sample_descriptor(mux, tr, data_bytes, duration, kind))
            return MP4E_STATUS_NO_MEMORY;
    } else
    {
        if (!mux->sequential_mode_flag)
        {
            sample_t *smpl_desc;
            if (tr->smpl.bytes < sizeof(sample_t))
                return MP4E_STATUS_NO_MEMORY; // write continuation, but there are no samples in the index
            // Accumulate size of the continuation in the sample descriptor
            smpl_desc = (sample_t*)(tr->smpl.data + tr->smpl.bytes) - 1;
            smpl_desc->size += data_bytes;
        }
    }

    if (mux->sequential_mode_flag)
    {
        if (!minimp4_vector_put(&tr->pending_sample, data, data_bytes))
            return MP4E_STATUS_NO_MEMORY;
    } else
    {
        ERR(mux->write_callback(mux->write_pos, data, data_bytes, mux->token));
        mux->write_pos += data_bytes;
    }
    return MP4E_STATUS_OK;
}

/**
*   calculate size of length field of OD box
*/
static int od_size_of_size(int size)
{
    int i, size_of_size = 1;
    for (i = size; i > 0x7F; i -= 0x7F)
        size_of_size++;
    return size_of_size;
}

/**
*   Add or remove MP4 file text comment according to Apple specs:
*   https://developer.apple.com/library/mac/documentation/QuickTime/QTFF/Metadata/Metadata.html#//apple_ref/doc/uid/TP40000939-CH1-SW1
*   http://atomicparsley.sourceforge.net/mpeg-4files.html
*   note that ISO did not specify comment format.
*/
int MP4E_set_text_comment(MP4E_mux_t *mux, const char *comment)
{
    if (!mux || !comment)
        return MP4E_STATUS_BAD_ARGUMENTS;
    if (mux->text_comment)
        free(mux->text_comment);
    mux->text_comment = strdup(comment);
    if (!mux->text_comment)
        return MP4E_STATUS_NO_MEMORY;
    return MP4E_STATUS_OK;
}

/**
*   Write file index 'moov' box with all its boxes and indexes
*/
static int mp4e_flush_index(MP4E_mux_t *mux)
{
    unsigned char *stack_base[20]; // atoms nesting stack
    unsigned char **stack = stack_base;
    unsigned char *base, *p;
    unsigned int ntr, index_bytes, ntracks = mux->tracks.bytes / sizeof(track_t);
    int i, err;

    // How much memory needed for indexes
    // Experimental data:
    // file with 1 track = 560 bytes
    // file with 2 tracks = 972 bytes
    // track size = 412 bytes;
    // file header size = 148 bytes
#define FILE_HEADER_BYTES 256
#define TRACK_HEADER_BYTES 512
    index_bytes = FILE_HEADER_BYTES;
    if (mux->text_comment)
        index_bytes += 128 + strlen(mux->text_comment);
    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        index_bytes += TRACK_HEADER_BYTES;          // fixed amount (implementation-dependent)
        // may need extra 4 bytes for duration field + 4 bytes for worst-case random access box
        index_bytes += tr->smpl.bytes * (sizeof(sample_t) + 4 + 4) / sizeof(sample_t);
        index_bytes += tr->vsps.bytes;
        index_bytes += tr->vpps.bytes;

        ERR(write_pending_data(mux, tr));
    }

    base = (unsigned char*)malloc(index_bytes);
    if (!base)
        return MP4E_STATUS_NO_MEMORY;
    p = base;

    if (!mux->sequential_mode_flag)
    {
        // update size of mdat box.
        // One of 2 points, which requires random file access.
        // Second is optional duration update at beginning of file in fragmentation mode.
        // This can be avoided using "till eof" size code, but in this case indexes must be
        // written before the mdat....
        int64_t size = mux->write_pos - sizeof(box_ftyp);
        const int64_t size_limit = (int64_t)(uint64_t)0xfffffffe;
        if (size > size_limit)
        {
            WRITE_4(1);
            WRITE_4(BOX_mdat);
            WRITE_4((size >> 32) & 0xffffffff);
            WRITE_4(size & 0xffffffff);
        } else
        {
            WRITE_4(8);
            WRITE_4(BOX_free);
            WRITE_4(size - 8);
            WRITE_4(BOX_mdat);
        }
        ERR(mux->write_callback(sizeof(box_ftyp), base, p - base, mux->token));
        p = base;
    }

    // Write index atoms; order taken from Table 1 of [1]
#define MOOV_TIMESCALE 1000
    ATOM(BOX_moov);
        ATOM_FULL(BOX_mvhd, 0);
        WRITE_4(0); // creation_time
        WRITE_4(0); // modification_time

        if (ntracks)
        {
            track_t *tr = ((track_t*)mux->tracks.data) + 0;    // take 1st track
            unsigned duration = get_duration(tr);
            duration = (unsigned)(duration * 1LL * MOOV_TIMESCALE / tr->info.time_scale);
            WRITE_4(MOOV_TIMESCALE); // duration
            WRITE_4(duration); // duration
        }

        WRITE_4(0x00010000); // rate
        WRITE_2(0x0100); // volume
        WRITE_2(0); // reserved
        WRITE_4(0); // reserved
        WRITE_4(0); // reserved

        // matrix[9]
        WRITE_4(0x00010000); WRITE_4(0); WRITE_4(0);
        WRITE_4(0); WRITE_4(0x00010000); WRITE_4(0);
        WRITE_4(0); WRITE_4(0); WRITE_4(0x40000000);

        // pre_defined[6]
        WRITE_4(0); WRITE_4(0); WRITE_4(0);
        WRITE_4(0); WRITE_4(0); WRITE_4(0);

        //next_track_ID is a non-zero integer that indicates a value to use for the track ID of the next track to be
        //added to this presentation. Zero is not a valid track ID value. The value of next_track_ID shall be
        //larger than the largest track-ID in use.
        WRITE_4(ntracks + 1);
        END_ATOM;

    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        unsigned duration = get_duration(tr);
        int samples_count = tr->smpl.bytes / sizeof(sample_t);
        const sample_t *sample = (const sample_t *)tr->smpl.data;
        unsigned handler_type;
        const char *handler_ascii = NULL;

        if (mux->enable_fragmentation)
            samples_count = 0;
        else if (samples_count <= 0)
            continue;   // skip empty track

        switch (tr->info.track_media_kind)
        {
            case e_audio:
                handler_type = MP4E_HANDLER_TYPE_SOUN;
                handler_ascii = "SoundHandler";
                break;
            case e_video:
                handler_type = MP4E_HANDLER_TYPE_VIDE;
                handler_ascii = "VideoHandler";
                break;
            case e_private:
                handler_type = MP4E_HANDLER_TYPE_GESM;
                break;
            default:
                return MP4E_STATUS_BAD_ARGUMENTS;
        }

        ATOM(BOX_trak);
            ATOM_FULL(BOX_tkhd, 7); // flag: 1=trak enabled; 2=track in movie; 4=track in preview
            WRITE_4(0);             // creation_time
            WRITE_4(0);             // modification_time
            WRITE_4(ntr + 1);       // track_ID
            WRITE_4(0);             // reserved
            WRITE_4((unsigned)(duration * 1LL * MOOV_TIMESCALE / tr->info.time_scale));
            WRITE_4(0); WRITE_4(0); // reserved[2]
            WRITE_2(0);             // layer
            WRITE_2(0);             // alternate_group
            WRITE_2(0x0100);        // volume {if track_is_audio 0x0100 else 0};
            WRITE_2(0);             // reserved

            // matrix[9]
            WRITE_4(0x00010000); WRITE_4(0); WRITE_4(0);
            WRITE_4(0); WRITE_4(0x00010000); WRITE_4(0);
            WRITE_4(0); WRITE_4(0); WRITE_4(0x40000000);

            if (tr->info.track_media_kind == e_audio || tr->info.track_media_kind == e_private)
            {
                WRITE_4(0); // width
                WRITE_4(0); // height
            } else
            {
                WRITE_4(tr->info.u.v.width*0x10000);  // width
                WRITE_4(tr->info.u.v.height*0x10000); // height
            }
            END_ATOM;

            ATOM(BOX_mdia);
                ATOM_FULL(BOX_mdhd, 0);
                WRITE_4(0); // creation_time
                WRITE_4(0); // modification_time
                WRITE_4(tr->info.time_scale);
                WRITE_4(duration); // duration
                {
                    int lang_code = ((tr->info.language[0] & 31) << 10) | ((tr->info.language[1] & 31) << 5) | (tr->info.language[2] & 31);
                    WRITE_2(lang_code); // language
                }
                WRITE_2(0); // pre_defined
                END_ATOM;

                ATOM_FULL(BOX_hdlr, 0);
                WRITE_4(0); // pre_defined
                WRITE_4(handler_type); // handler_type
                WRITE_4(0); WRITE_4(0); WRITE_4(0); // reserved[3]
                // name is a null-terminated string in UTF-8 characters which gives a human-readable name for the track type (for debugging and inspection purposes).
                // set mdia hdlr name field to what quicktime uses.
                // Sony smartphone may fail to decode short files w/o handler name
                if (handler_ascii)
                {
                    for (i = 0; i < (int)strlen(handler_ascii) + 1; i++)
                    {
                        WRITE_1(handler_ascii[i]);
                    }
                } else
                {
                    WRITE_4(0);
                }
                END_ATOM;

                ATOM(BOX_minf);

                    if (tr->info.track_media_kind == e_audio)
                    {
                        // Sound Media Header Box
                        ATOM_FULL(BOX_smhd, 0);
                        WRITE_2(0);   // balance
                        WRITE_2(0);   // reserved
                        END_ATOM;
                    }
                    if (tr->info.track_media_kind == e_video)
                    {
                        // mandatory Video Media Header Box
                        ATOM_FULL(BOX_vmhd, 1);
                        WRITE_2(0); // graphicsmode
                        WRITE_2(0); WRITE_2(0); WRITE_2(0); // opcolor[3]
                        END_ATOM;
                    }

                    ATOM(BOX_dinf);
                        ATOM_FULL(BOX_dref, 0);
                        WRITE_4(1); // entry_count
                            // If the flag is set indicating that the data is in the same file as this box, then no string (not even an empty one)
                            // shall be supplied in the entry field.

                            // ASP the correct way to avoid supply the string, is to use flag 1
                            // otherwise ISO reference demux crashes
                            ATOM_FULL(BOX_url, 1);
                            END_ATOM;
                        END_ATOM;
                    END_ATOM;

                    ATOM(BOX_stbl);
                        ATOM_FULL(BOX_stsd, 0);
                        WRITE_4(1); // entry_count;

                        if (tr->info.track_media_kind == e_audio || tr->info.track_media_kind == e_private)
                        {
                            // AudioSampleEntry() assume MP4E_HANDLER_TYPE_SOUN
                            if (tr->info.track_media_kind == e_audio)
                            {
                                ATOM(BOX_mp4a);
                            } else
                            {
                                ATOM(BOX_mp4s);
                            }

                            // SampleEntry
                            WRITE_4(0); WRITE_2(0); // reserved[6]
                            WRITE_2(1); // data_reference_index; - this is a tag for descriptor below

                            if (tr->info.track_media_kind == e_audio)
                            {
                                // AudioSampleEntry
                                WRITE_4(0); WRITE_4(0); // reserved[2]
                                WRITE_2(tr->info.u.a.channelcount); // channelcount
                                WRITE_2(16); // samplesize
                                WRITE_4(0);  // pre_defined+reserved
                                WRITE_4((tr->info.time_scale << 16));  // samplerate == = {timescale of media}<<16;
                            }

                                ATOM_FULL(BOX_esds, 0);
                                if (tr->vsps.bytes > 0)
                                {
                                    int dsi_bytes = tr->vsps.bytes - 2; //  - two bytes size field
                                    int dsi_size_size = od_size_of_size(dsi_bytes);
                                    int dcd_bytes = dsi_bytes + dsi_size_size + 1 + (1 + 1 + 3 + 4 + 4);
                                    int dcd_size_size = od_size_of_size(dcd_bytes);
                                    int esd_bytes = dcd_bytes + dcd_size_size + 1 + 3;

#define WRITE_OD_LEN(size) if (size > 0x7F) do { size -= 0x7F; WRITE_1(0x00ff); } while (size > 0x7F); WRITE_1(size)
                                    WRITE_1(3); // OD_ESD
                                    WRITE_OD_LEN(esd_bytes);
                                    WRITE_2(0); // ES_ID(2) // TODO - what is this?
                                    WRITE_1(0); // flags(1)

                                    WRITE_1(4); // OD_DCD
                                    WRITE_OD_LEN(dcd_bytes);
                                    if (tr->info.track_media_kind == e_audio)
                                    {
                                        WRITE_1(MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3); // OD_DCD
                                        WRITE_1(5 << 2); // stream_type == AudioStream
                                    } else
                                    {
                                        // http://xhelmboyx.tripod.com/formats/mp4-layout.txt
                                        WRITE_1(208); // 208 = private video
                                        WRITE_1(32 << 2); // stream_type == user private
                                    }
                                    WRITE_3(tr->info.u.a.channelcount * 6144/8); // bufferSizeDB in bytes, constant as in reference decoder
                                    WRITE_4(0); // maxBitrate TODO
                                    WRITE_4(0); // avg_bitrate_bps TODO

                                    WRITE_1(5); // OD_DSI
                                    WRITE_OD_LEN(dsi_bytes);
                                    for (i = 0; i < dsi_bytes; i++)
                                    {
                                        WRITE_1(tr->vsps.data[2 + i]);
                                    }
                                }
                                END_ATOM;
                            END_ATOM;
                        }

                        if (tr->info.track_media_kind == e_video && (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication || MP4_OBJECT_TYPE_HEVC == tr->info.object_type_indication))
                        {
                            int numOfSequenceParameterSets = items_count(&tr->vsps);
                            int numOfPictureParameterSets  = items_count(&tr->vpps);
                            if (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication)
                            {
                                ATOM(BOX_avc1);
                            } else
                            {
                                ATOM(BOX_hvc1);
                            }
                            // VisualSampleEntry  8.16.2
                            // extends SampleEntry
                            WRITE_2(0); // reserved
                            WRITE_2(0); // reserved
                            WRITE_2(0); // reserved
                            WRITE_2(1); // data_reference_index

                            WRITE_2(0); // pre_defined
                            WRITE_2(0); // reserved
                            WRITE_4(0); // pre_defined
                            WRITE_4(0); // pre_defined
                            WRITE_4(0); // pre_defined
                            WRITE_2(tr->info.u.v.width);
                            WRITE_2(tr->info.u.v.height);
                            WRITE_4(0x00480000); // horizresolution = 72 dpi
                            WRITE_4(0x00480000); // vertresolution  = 72 dpi
                            WRITE_4(0); // reserved
                            WRITE_2(1); // frame_count
                            for (i = 0; i < 32; i++)
                            {
                                WRITE_1(0); //  compressorname
                            }
                            WRITE_2(24); // depth
                            WRITE_2(-1); // pre_defined

                            if (MP4_OBJECT_TYPE_AVC == tr->info.object_type_indication)
                            {
                                ATOM(BOX_avcC);
                                // AVCDecoderConfigurationRecord 5.2.4.1.1
                                WRITE_1(1); // configurationVersion
                                WRITE_1(tr->vsps.data[2 + 1]);
                                WRITE_1(tr->vsps.data[2 + 2]);
                                WRITE_1(tr->vsps.data[2 + 3]);
                                WRITE_1(255); // 0xfc + NALU_len - 1
                                WRITE_1(0xe0 | numOfSequenceParameterSets);
                                for (i = 0; i < tr->vsps.bytes; i++)
                                {
                                    WRITE_1(tr->vsps.data[i]);
                                }
                                WRITE_1(numOfPictureParameterSets);
                                for (i = 0; i < tr->vpps.bytes; i++)
                                {
                                    WRITE_1(tr->vpps.data[i]);
                                }
                            } else
                            {
                                int numOfVPS  = items_count(&tr->vpps);
                                ATOM(BOX_hvcC);
                                // TODO: read actual params from stream
                                WRITE_1(1);    // configurationVersion
                                WRITE_1(1);    // Profile Space (2), Tier (1), Profile (5)
                                WRITE_4(0x60000000); // Profile Compatibility
                                WRITE_2(0);    // progressive, interlaced, non packed constraint, frame only constraint flags
                                WRITE_4(0);    // constraint indicator flags
                                WRITE_1(0);    // level_idc
                                WRITE_2(0xf000); // Min Spatial Segmentation
                                WRITE_1(0xfc); // Parallelism Type
                                WRITE_1(0xfc); // Chroma Format
                                WRITE_1(0xf8); // Luma Depth
                                WRITE_1(0xf8); // Chroma Depth
                                WRITE_2(0);    // Avg Frame Rate
                                WRITE_1(3);    // ConstantFrameRate (2), NumTemporalLayers (3), TemporalIdNested (1), LengthSizeMinusOne (2)

                                WRITE_1(3);    // Num Of Arrays
                                WRITE_1((1 << 7) | (HEVC_NAL_VPS & 0x3f)); // Array Completeness + NAL Unit Type
                                WRITE_2(numOfVPS);
                                for (i = 0; i < tr->vvps.bytes; i++)
                                {
                                    WRITE_1(tr->vvps.data[i]);
                                }
                                WRITE_1((1 << 7) | (HEVC_NAL_SPS & 0x3f));
                                WRITE_2(numOfSequenceParameterSets);
                                for (i = 0; i < tr->vsps.bytes; i++)
                                {
                                    WRITE_1(tr->vsps.data[i]);
                                }
                                WRITE_1((1 << 7) | (HEVC_NAL_PPS & 0x3f));
                                WRITE_2(numOfPictureParameterSets);
                                for (i = 0; i < tr->vpps.bytes; i++)
                                {
                                    WRITE_1(tr->vpps.data[i]);
                                }
                            }

                            END_ATOM;
                            END_ATOM;
                        }
                        END_ATOM;

                        /************************************************************************/
                        /*      indexes                                                         */
                        /************************************************************************/

                        // Time to Sample Box
                        ATOM_FULL(BOX_stts, 0);
                        {
                            unsigned char *pentry_count = p;
                            int cnt = 1, entry_count = 0;
                            WRITE_4(0);
                            for (i = 0; i < samples_count; i++, cnt++)
                            {
                                if (i == (samples_count - 1) || sample[i].duration != sample[i + 1].duration)
                                {
                                    WRITE_4(cnt);
                                    WRITE_4(sample[i].duration);
                                    cnt = 0;
                                    entry_count++;
                                }
                            }
                            WR4(pentry_count, entry_count);
                        }
                        END_ATOM;

                        // Sample To Chunk Box
                        ATOM_FULL(BOX_stsc, 0);
                        if (mux->enable_fragmentation)
                        {
                            WRITE_4(0); // entry_count
                        } else
                        {
                            WRITE_4(1); // entry_count
                            WRITE_4(1); // first_chunk;
                            WRITE_4(1); // samples_per_chunk;
                            WRITE_4(1); // sample_description_index;
                        }
                        END_ATOM;

                        // Sample Size Box
                        ATOM_FULL(BOX_stsz, 0);
                        WRITE_4(0); // sample_size  If this field is set to 0, then the samples have different sizes, and those sizes
                                    //  are stored in the sample size table.
                        WRITE_4(samples_count);  // sample_count;
                        for (i = 0; i < samples_count; i++)
                        {
                            WRITE_4(sample[i].size);
                        }
                        END_ATOM;

                        // Chunk Offset Box
                        int is_64_bit = 0;
                        if (samples_count && sample[samples_count - 1].offset > 0xffffffff)
                            is_64_bit = 1;
                        if (!is_64_bit)
                        {
                            ATOM_FULL(BOX_stco, 0);
                            WRITE_4(samples_count);
                            for (i = 0; i < samples_count; i++)
                            {
                                WRITE_4(sample[i].offset);
                            }
                        } else
                        {
                            ATOM_FULL(BOX_co64, 0);
                            WRITE_4(samples_count);
                            for (i = 0; i < samples_count; i++)
                            {
                                WRITE_4((sample[i].offset >> 32) & 0xffffffff);
                                WRITE_4(sample[i].offset & 0xffffffff);
                            }
                        }
                        END_ATOM;

                        // Sync Sample Box
                        {
                            int ra_count = 0;
                            for (i = 0; i < samples_count; i++)
                            {
                                ra_count += !!sample[i].flag_random_access;
                            }
                            if (ra_count != samples_count)
                            {
                                // If the sync sample box is not present, every sample is a random access point.
                                ATOM_FULL(BOX_stss, 0);
                                WRITE_4(ra_count);
                                for (i = 0; i < samples_count; i++)
                                {
                                    if (sample[i].flag_random_access)
                                    {
                                        WRITE_4(i + 1);
                                    }
                                }
                                END_ATOM;
                            }
                        }
                    END_ATOM;
                END_ATOM;
            END_ATOM;
        END_ATOM;
    } // tracks loop

    if (mux->text_comment)
    {
        ATOM(BOX_udta);
            ATOM_FULL(BOX_meta, 0);
                ATOM_FULL(BOX_hdlr, 0);
                    WRITE_4(0); // pre_defined
#define MP4E_HANDLER_TYPE_MDIR  0x6d646972
                    WRITE_4(MP4E_HANDLER_TYPE_MDIR); // handler_type
                    WRITE_4(0); WRITE_4(0); WRITE_4(0); // reserved[3]
                    WRITE_4(0); // name is a null-terminated string in UTF-8 characters which gives a human-readable name for the track type (for debugging and inspection purposes).
                END_ATOM;
                ATOM(BOX_ilst);
                    ATOM(BOX_ccmt);
                        ATOM(BOX_data);
                            WRITE_4(1); // type
                            WRITE_4(0); // lang
                            for (i = 0; i < (int)strlen(mux->text_comment) + 1; i++)
                            {
                                WRITE_1(mux->text_comment[i]);
                            }
                        END_ATOM;
                    END_ATOM;
                END_ATOM;
            END_ATOM;
        END_ATOM;
    }

    if (mux->enable_fragmentation)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + 0;
        uint32_t movie_duration = get_duration(tr);

        ATOM(BOX_mvex);
            ATOM_FULL(BOX_mehd, 0);
                WRITE_4(movie_duration); // duration
            END_ATOM;
        for (ntr = 0; ntr < ntracks; ntr++)
        {
            ATOM_FULL(BOX_trex, 0);
                WRITE_4(ntr + 1);        // track_ID
                WRITE_4(1);              // default_sample_description_index
                WRITE_4(0);              // default_sample_duration
                WRITE_4(0);              // default_sample_size
                WRITE_4(0);              // default_sample_flags
            END_ATOM;
        }
        END_ATOM;
    }
    END_ATOM;   // moov atom

    assert((unsigned)(p - base) <= index_bytes);

    err = mux->write_callback(mux->write_pos, base, p - base, mux->token);
    mux->write_pos += p - base;
    free(base);
    return err;
}

int MP4E_close(MP4E_mux_t *mux)
{
    int err = MP4E_STATUS_OK;
    unsigned ntr, ntracks;
    if (!mux)
        return MP4E_STATUS_BAD_ARGUMENTS;
    if (!mux->enable_fragmentation)
        err = mp4e_flush_index(mux);
    if (mux->text_comment)
        free(mux->text_comment);
    ntracks = mux->tracks.bytes / sizeof(track_t);
    for (ntr = 0; ntr < ntracks; ntr++)
    {
        track_t *tr = ((track_t*)mux->tracks.data) + ntr;
        minimp4_vector_reset(&tr->vsps);
        minimp4_vector_reset(&tr->vpps);
        minimp4_vector_reset(&tr->smpl);
        minimp4_vector_reset(&tr->pending_sample);
    }
    minimp4_vector_reset(&mux->tracks);
    free(mux);
    return err;
}

typedef uint32_t bs_item_t;
#define BS_BITS 32

typedef struct
{
    // Look-ahead bit cache: MSB aligned, 17 bits guaranteed, zero stuffing
    unsigned int cache;

    // Bit counter = 16 - (number of bits in wCache)
    // cache refilled when cache_free_bits >= 0
    int cache_free_bits;

    // Current read position
    const uint16_t *buf;

    // original data buffer
    const uint16_t *origin;

    // original data buffer length, bytes
    unsigned origin_bytes;
} bit_reader_t;


#define LOAD_SHORT(x) ((uint16_t)(x << 8) | (x >> 8))

static unsigned int show_bits(bit_reader_t *bs, int n)
{
    unsigned int retval;
    assert(n > 0 && n <= 16);
    retval = (unsigned int)(bs->cache >> (32 - n));
    return retval;
}

static void flush_bits(bit_reader_t *bs, int n)
{
    assert(n >= 0 && n <= 16);
    bs->cache <<= n;
    bs->cache_free_bits += n;
    if (bs->cache_free_bits >= 0)
    {
        bs->cache |= ((uint32_t)LOAD_SHORT(*bs->buf)) << bs->cache_free_bits;
        bs->buf++;
        bs->cache_free_bits -= 16;
    }
}

static unsigned int get_bits(bit_reader_t *bs, int n)
{
    unsigned int retval = show_bits(bs, n);
    flush_bits(bs, n);
    return retval;
}

static void set_pos_bits(bit_reader_t *bs, unsigned pos_bits)
{
    assert((int)pos_bits >= 0);

    bs->buf = bs->origin + pos_bits/16;
    bs->cache = 0;
    bs->cache_free_bits = 16;
    flush_bits(bs, 0);
    flush_bits(bs, pos_bits & 15);
}

static unsigned get_pos_bits(const bit_reader_t *bs)
{
    // Current bitbuffer position =
    // position of next wobits in the internal buffer
    // minus bs, available in bit cache wobits
    unsigned pos_bits = (unsigned)(bs->buf - bs->origin)*16;
    pos_bits -= 16 - bs->cache_free_bits;
    assert((int)pos_bits >= 0);
    return pos_bits;
}

static int remaining_bits(const bit_reader_t *bs)
{
    return bs->origin_bytes * 8 - get_pos_bits(bs);
}

static void init_bits(bit_reader_t *bs, const void *data, unsigned data_bytes)
{
    bs->origin = (const uint16_t *)data;
    bs->origin_bytes = data_bytes;
    set_pos_bits(bs, 0);
}

#define GetBits(n) get_bits(bs, n)

/**
*   Unsigned Golomb code
*/
static int ue_bits(bit_reader_t *bs)
{
    int clz;
    int val;
    for (clz = 0; !get_bits(bs, 1); clz++) {}
    //get_bits(bs, clz + 1);
    val = (1 << clz) - 1 + (clz ? get_bits(bs, clz) : 0);
    return val;
}

#if MINIMP4_TRANSCODE_SPS_ID

/**
*   Output bitstream
*/
typedef struct
{
    int        shift;    // bit position in the cache
    uint32_t   cache;    // bit cache
    bs_item_t  *buf;     // current position
    bs_item_t  *origin;  // initial position
} bs_t;

#define SWAP32(x) (uint32_t)((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | (((x) << 8) & 0xFF0000) | ((x & 0xFF) << 24))

static void h264e_bs_put_bits(bs_t *bs, unsigned n, unsigned val)
{
    assert(!(val >> n));
    bs->shift -= n;
    assert((unsigned)n <= 32);
    if (bs->shift < 0)
    {
        assert(-bs->shift < 32);
        bs->cache |= val >> -bs->shift;
        *bs->buf++ = SWAP32(bs->cache);
        bs->shift = 32 + bs->shift;
        bs->cache = 0;
    }
    bs->cache |= val << bs->shift;
}

static void h264e_bs_flush(bs_t *bs)
{
    *bs->buf = SWAP32(bs->cache);
}

static unsigned h264e_bs_get_pos_bits(const bs_t *bs)
{
    unsigned pos_bits = (unsigned)((bs->buf - bs->origin)*BS_BITS);
    pos_bits += BS_BITS - bs->shift;
    assert((int)pos_bits >= 0);
    return pos_bits;
}

static unsigned h264e_bs_byte_align(bs_t *bs)
{
    int pos = h264e_bs_get_pos_bits(bs);
    h264e_bs_put_bits(bs, -pos & 7, 0);
    return pos + (-pos & 7);
}

/**
*   Golomb code
*   0 => 1
*   1 => 01 0
*   2 => 01 1
*   3 => 001 00
*   4 => 001 01
*
*   [0]     => 1
*   [1..2]  => 01x
*   [3..6]  => 001xx
*   [7..14] => 0001xxx
*
*/
static void h264e_bs_put_golomb(bs_t *bs, unsigned val)
{
    int size = 0;
    unsigned t = val + 1;
    do
    {
        size++;
    } while (t >>= 1);

    h264e_bs_put_bits(bs, 2*size - 1, val + 1);
}

static void h264e_bs_init_bits(bs_t *bs, void *data)
{
    bs->origin = (bs_item_t*)data;
    bs->buf = bs->origin;
    bs->shift = BS_BITS;
    bs->cache = 0;
}

static int find_mem_cache(void *cache[], int cache_bytes[], int cache_size, void *mem, int bytes)
{
    int i;
    if (!bytes)
        return -1;
    for (i = 0; i < cache_size; i++)
    {
        if (cache_bytes[i] == bytes && !memcmp(mem, cache[i], bytes))
            return i;   // found
    }
    for (i = 0; i < cache_size; i++)
    {
        if (!cache_bytes[i])
        {
            cache[i] = malloc(bytes);
            if (cache[i])
            {
                memcpy(cache[i], mem, bytes);
                cache_bytes[i] = bytes;
            }
            return i;   // put in
        }
    }
    return -1;  // no room
}

/**
*   7.4.1.1. "Encapsulation of an SODB within an RBSP"
*/
static int remove_nal_escapes(unsigned char *dst, const unsigned char *src, int h264_data_bytes)
{
    int i = 0, j = 0, zero_cnt = 0;
    for (j = 0; j < h264_data_bytes; j++)
    {
        if (zero_cnt == 2 && src[j] <= 3)
        {
            if (src[j] == 3)
            {
                if (j == h264_data_bytes - 1)
                {
                    // cabac_zero_word: no action
                } else if (src[j + 1] <= 3)
                {
                    j++;
                    zero_cnt = 0;
                } else
                {
                    // TODO: assume end-of-nal
                    //return 0;
                }
            } else
                return 0;
        }
        dst[i++] = src[j];
        if (src[j])
            zero_cnt = 0;
        else
            zero_cnt++;
    }
    //while (--j > i) src[j] = 0;
    return i;
}

/**
*   Put NAL escape codes to the output bitstream
*/
static int nal_put_esc(uint8_t *d, const uint8_t *s, int n)
{
    int i, j = 4, cntz = 0;
    d[0] = d[1] = d[2] = 0; d[3] = 1; // start code
    for (i = 0; i < n; i++)
    {
        uint8_t byte = *s++;
        if (cntz == 2 && byte <= 3)
        {
            d[j++] = 3;
            cntz = 0;
        }
        if (byte)
            cntz = 0;
        else
            cntz++;
        d[j++] = byte;
    }
    return j;
}

static void copy_bits(bit_reader_t *bs, bs_t *bd)
{
    unsigned cb, bits;
    int bit_count = remaining_bits(bs);
    while (bit_count > 7)
    {
        cb = MINIMP4_MIN(bit_count - 7, 8);
        bits = GetBits(cb);
        h264e_bs_put_bits(bd, cb, bits);
        bit_count -= cb;
    }

    // cut extra zeros after stop-bit
    bits = GetBits(bit_count);
    for (; bit_count && ~bits & 1; bit_count--)
    {
        bits >>= 1;
    }
    if (bit_count)
    {
        h264e_bs_put_bits(bd, bit_count, bits);
    }
}

static int change_sps_id(bit_reader_t *bs, bs_t *bd, int new_id, int *old_id)
{
    unsigned bits, sps_id, i, bytes;
    for (i = 0; i < 3; i++)
    {
        bits = GetBits(8);
        h264e_bs_put_bits(bd, 8, bits);
    }
    sps_id = ue_bits(bs);               // max = 31

    *old_id = sps_id;
    sps_id = new_id;
    assert(sps_id <= 31);

    h264e_bs_put_golomb(bd, sps_id);
    copy_bits(bs, bd);

    bytes = h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);
    return bytes;
}

static int patch_pps(h264_sps_id_patcher_t *h, bit_reader_t *bs, bs_t *bd, int new_pps_id, int *old_id)
{
    int bytes;
    unsigned pps_id = ue_bits(bs);  // max = 255
    unsigned sps_id = ue_bits(bs);  // max = 31

    *old_id = pps_id;
    sps_id = h->map_sps[sps_id];
    pps_id = new_pps_id;

    assert(sps_id <= 31);
    assert(pps_id <= 255);

    h264e_bs_put_golomb(bd, pps_id);
    h264e_bs_put_golomb(bd, sps_id);
    copy_bits(bs, bd);

    bytes = h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);
    return bytes;
}

static void patch_slice_header(h264_sps_id_patcher_t *h, bit_reader_t *bs, bs_t *bd)
{
    unsigned first_mb_in_slice = ue_bits(bs);
    unsigned slice_type = ue_bits(bs);
    unsigned pps_id = ue_bits(bs);

    pps_id = h->map_pps[pps_id];

    assert(pps_id <= 255);

    h264e_bs_put_golomb(bd, first_mb_in_slice);
    h264e_bs_put_golomb(bd, slice_type);
    h264e_bs_put_golomb(bd, pps_id);
    copy_bits(bs, bd);
}

static int transcode_nalu(h264_sps_id_patcher_t *h, const unsigned char *src, int nalu_bytes, unsigned char *dst)
{
    int old_id;

    bit_reader_t bst[1];
    bs_t bdt[1];

    bit_reader_t bs[1];
    bs_t bd[1];
    int payload_type = src[0] & 31;

    *dst = *src;
    h264e_bs_init_bits(bd, dst + 1);
    init_bits(bs, src + 1, nalu_bytes - 1);
    h264e_bs_init_bits(bdt, dst + 1);
    init_bits(bst, src + 1, nalu_bytes - 1);

    switch(payload_type)
    {
    case 7:
        {
            int cb = change_sps_id(bst, bdt, 0, &old_id);
            int id = find_mem_cache(h->sps_cache, h->sps_bytes, MINIMP4_MAX_SPS, dst + 1, cb);
            if (id == -1)
                return 0;
            h->map_sps[old_id] = id;
            change_sps_id(bs, bd, id, &old_id);
        }
        break;
    case 8:
        {
            int cb = patch_pps(h, bst, bdt, 0, &old_id);
            int id = find_mem_cache(h->pps_cache, h->pps_bytes, MINIMP4_MAX_PPS, dst + 1, cb);
            if (id == -1)
                return 0;
            h->map_pps[old_id] = id;
            patch_pps(h, bs, bd, id, &old_id);
        }
        break;
    case 1:
    case 2:
    case 5:
        patch_slice_header(h, bs, bd);
        break;
    default:
        memcpy(dst, src, nalu_bytes);
        return nalu_bytes;
    }

    nalu_bytes = 1 + h264e_bs_byte_align(bd) / 8;
    h264e_bs_flush(bd);

    return nalu_bytes;
}

#endif

/**
*   Set pointer just after start code (00 .. 00 01), or to EOF if not found:
*
*   NZ NZ ... NZ 00 00 00 00 01 xx xx ... xx (EOF)
*                               ^            ^
*   non-zero head.............. here ....... or here if no start code found
*
*/
static const uint8_t *find_start_code(const uint8_t *h264_data, int h264_data_bytes, int *zcount)
{
    const uint8_t *eof = h264_data + h264_data_bytes;
    const uint8_t *p = h264_data;
    do
    {
        int zero_cnt = 1;
        const uint8_t* found = (uint8_t*)memchr(p, 0, eof - p);
        p = found ? found : eof;
        while (p + zero_cnt < eof && !p[zero_cnt]) zero_cnt++;
        if (zero_cnt >= 2 && p[zero_cnt] == 1)
        {
            *zcount = zero_cnt + 1;
            return p + zero_cnt + 1;
        }
        p += zero_cnt;
    } while (p < eof);
    *zcount = 0;
    return eof;
}

/**
*   Locate NAL unit in given buffer, and calculate it's length
*/
static const uint8_t *find_nal_unit(const uint8_t *h264_data, int h264_data_bytes, int *pnal_unit_bytes)
{
    const uint8_t *eof = h264_data + h264_data_bytes;
    int zcount;
    const uint8_t *start = find_start_code(h264_data, h264_data_bytes, &zcount);
    const uint8_t *stop = start;
    if (start)
    {
        stop = find_start_code(start, (int)(eof - start), &zcount);
        while (stop > start && !stop[-1])
        {
            stop--;
        }
    }

    *pnal_unit_bytes = (int)(stop - start - zcount);
    return start;
}

int mp4_h26x_write_init(mp4_h26x_writer_t *h, MP4E_mux_t *mux, int width, int height, int is_hevc)
{
    MP4E_track_t tr;
    tr.track_media_kind = e_video;
    tr.language[0] = 'u';
    tr.language[1] = 'n';
    tr.language[2] = 'd';
    tr.language[3] = 0;
    tr.object_type_indication = is_hevc ? MP4_OBJECT_TYPE_HEVC : MP4_OBJECT_TYPE_AVC;
    tr.time_scale = 90000;
    tr.default_duration = 0;
    tr.u.v.width = width;
    tr.u.v.height = height;
    h->mux_track_id = MP4E_add_track(mux, &tr);
    h->mux = mux;

    h->is_hevc  = is_hevc;
    h->need_vps = is_hevc;
    h->need_sps = 1;
    h->need_pps = 1;
    h->need_idr = 1;
#if MINIMP4_TRANSCODE_SPS_ID
    memset(&h->sps_patcher, 0, sizeof(h264_sps_id_patcher_t));
#endif
    return MP4E_STATUS_OK;
}

void mp4_h26x_write_close(mp4_h26x_writer_t *h)
{
#if MINIMP4_TRANSCODE_SPS_ID
    h264_sps_id_patcher_t *p = &h->sps_patcher;
    int i;
    for (i = 0; i < MINIMP4_MAX_SPS; i++)
    {
        if (p->sps_cache[i])
            free(p->sps_cache[i]);
    }
    for (i = 0; i < MINIMP4_MAX_PPS; i++)
    {
        if (p->pps_cache[i])
            free(p->pps_cache[i]);
    }
#endif
    memset(h, 0, sizeof(*h));
}

static int mp4_h265_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int sizeof_nal, unsigned timeStamp90kHz_next)
{
    int payload_type = (nal[0] >> 1) & 0x3f;
    int is_intra = payload_type >= HEVC_NAL_BLA_W_LP && payload_type <= HEVC_NAL_CRA_NUT;
    int err = MP4E_STATUS_OK;
    //printf("payload_type=%d, intra=%d\n", payload_type, is_intra);

    if (is_intra && !h->need_sps && !h->need_pps && !h->need_vps)
        h->need_idr = 0;
    switch (payload_type)
    {
    case HEVC_NAL_VPS:
        MP4E_set_vps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_vps = 0;
        break;
    case HEVC_NAL_SPS:
        MP4E_set_sps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_sps = 0;
        break;
    case HEVC_NAL_PPS:
        MP4E_set_pps(h->mux, h->mux_track_id, nal, sizeof_nal);
        h->need_pps = 0;
        break;
    default:
        if (h->need_vps || h->need_sps || h->need_pps || h->need_idr)
            return MP4E_STATUS_BAD_ARGUMENTS;
        {
            unsigned char *tmp = (unsigned char *)malloc(4 + sizeof_nal);
            if (!tmp)
                return MP4E_STATUS_NO_MEMORY;
            int sample_kind = MP4E_SAMPLE_DEFAULT;
            tmp[0] = (unsigned char)(sizeof_nal >> 24);
            tmp[1] = (unsigned char)(sizeof_nal >> 16);
            tmp[2] = (unsigned char)(sizeof_nal >>  8);
            tmp[3] = (unsigned char)(sizeof_nal);
            memcpy(tmp + 4, nal, sizeof_nal);
            if (is_intra)
                sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
            err = MP4E_put_sample(h->mux, h->mux_track_id, tmp, 4 + sizeof_nal, timeStamp90kHz_next, sample_kind);
            free(tmp);
        }
        break;
    }
    return err;
}

int mp4_h26x_write_nal(mp4_h26x_writer_t *h, const unsigned char *nal, int length, unsigned timeStamp90kHz_next)
{
    const unsigned char *eof = nal + length;
    int payload_type, sizeof_nal, err = MP4E_STATUS_OK;
    for (;;nal++)
    {
#if MINIMP4_TRANSCODE_SPS_ID
        unsigned char *nal1, *nal2;
#endif
        nal = find_nal_unit(nal, (int)(eof - nal), &sizeof_nal);
        if (!sizeof_nal)
            break;
        if (h->is_hevc)
        {
            ERR(mp4_h265_write_nal(h, nal, sizeof_nal, timeStamp90kHz_next));
            continue;
        }
        payload_type = nal[0] & 31;
        if (9 == payload_type)
            continue;  // access unit delimiter, nothing to be done
#if MINIMP4_TRANSCODE_SPS_ID
        // Transcode SPS, PPS and slice headers, reassigning ID's for SPS and  PPS:
        // - assign unique ID's to different SPS and PPS
        // - assign same ID's to equal (except ID) SPS and PPS
        // - save all different SPS and PPS
        nal1 = (unsigned char *)malloc(sizeof_nal*17/16 + 32);
        if (!nal1)
            return MP4E_STATUS_NO_MEMORY;
        nal2 = (unsigned char *)malloc(sizeof_nal*17/16 + 32);
        if (!nal2)
        {
            free(nal1);
            return MP4E_STATUS_NO_MEMORY;
        }
        sizeof_nal = remove_nal_escapes(nal2, nal, sizeof_nal);
        if (!sizeof_nal)
        {
exit_with_free:
            free(nal1);
            free(nal2);
            return MP4E_STATUS_BAD_ARGUMENTS;
        }

        sizeof_nal = transcode_nalu(&h->sps_patcher, nal2, sizeof_nal, nal1);
        sizeof_nal = nal_put_esc(nal2, nal1, sizeof_nal);

        switch (payload_type) {
        case 7:
            MP4E_set_sps(h->mux, h->mux_track_id, nal2 + 4, sizeof_nal - 4);
            h->need_sps = 0;
            break;
        case 8:
            if (h->need_sps)
                goto exit_with_free;
            MP4E_set_pps(h->mux, h->mux_track_id, nal2 + 4, sizeof_nal - 4);
            h->need_pps = 0;
            break;
        case 5:
            if (h->need_sps)
                goto exit_with_free;
            h->need_idr = 0;
            // flow through
        default:
            if (h->need_sps)
                goto exit_with_free;
            if (!h->need_pps && !h->need_idr)
            {
                bit_reader_t bs[1];
                init_bits(bs, nal + 1, sizeof_nal - 4 - 1);
                unsigned first_mb_in_slice = ue_bits(bs);
                //unsigned slice_type = ue_bits(bs);
                int sample_kind = MP4E_SAMPLE_DEFAULT;
                nal2[0] = (unsigned char)((sizeof_nal - 4) >> 24);
                nal2[1] = (unsigned char)((sizeof_nal - 4) >> 16);
                nal2[2] = (unsigned char)((sizeof_nal - 4) >>  8);
                nal2[3] = (unsigned char)((sizeof_nal - 4));
                if (first_mb_in_slice)
                    sample_kind = MP4E_SAMPLE_CONTINUATION;
                else if (payload_type == 5)
                    sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
                err = MP4E_put_sample(h->mux, h->mux_track_id, nal2, sizeof_nal, timeStamp90kHz_next, sample_kind);
            }
            break;
        }
        free(nal1);
        free(nal2);
#else
        // No SPS/PPS transcoding
        // This branch assumes that encoder use correct SPS/PPS ID's
        switch (payload_type) {
            case 7:
                MP4E_set_sps(h->mux, h->mux_track_id, nal, sizeof_nal);
                h->need_sps = 0;
                break;
            case 8:
                MP4E_set_pps(h->mux, h->mux_track_id, nal, sizeof_nal);
                h->need_pps = 0;
                break;
            case 5:
                if (h->need_sps)
                    return MP4E_STATUS_BAD_ARGUMENTS;
                h->need_idr = 0;
                // flow through
            default:
                if (h->need_sps)
                    return MP4E_STATUS_BAD_ARGUMENTS;
                if (!h->need_pps && !h->need_idr)
                {
                    bit_reader_t bs[1];
                    unsigned char *tmp = (unsigned char *)malloc(4 + sizeof_nal);
                    if (!tmp)
                        return MP4E_STATUS_NO_MEMORY;
                    init_bits(bs, nal + 1, sizeof_nal - 1);
                    unsigned first_mb_in_slice = ue_bits(bs);
                    int sample_kind = MP4E_SAMPLE_DEFAULT;
                    tmp[0] = (unsigned char)(sizeof_nal >> 24);
                    tmp[1] = (unsigned char)(sizeof_nal >> 16);
                    tmp[2] = (unsigned char)(sizeof_nal >>  8);
                    tmp[3] = (unsigned char)(sizeof_nal);
                    memcpy(tmp + 4, nal, sizeof_nal);
                    if (first_mb_in_slice)
                        sample_kind = MP4E_SAMPLE_CONTINUATION;
                    else if (payload_type == 5)
                        sample_kind = MP4E_SAMPLE_RANDOM_ACCESS;
                    err = MP4E_put_sample(h->mux, h->mux_track_id, tmp, 4 + sizeof_nal, timeStamp90kHz_next, sample_kind);
                    free(tmp);
                }
                break;
        }
#endif
        if (err)
            break;
    }
    return err;
}

#if MP4D_TRACE_SUPPORTED
#   define TRACE(x) printf x
#else
#   define TRACE(x)
#endif

#define NELEM(x)  (sizeof(x) / sizeof((x)[0]))

static int minimp4_fgets(MP4D_demux_t *mp4)
{
    uint8_t c;
    if (mp4->read_callback(mp4->read_pos, &c, 1, mp4->token))
        return -1;
    mp4->read_pos++;
    return c;
}

/**
*   Read given number of bytes from input stream
*   Used to read box headers
*/
static unsigned minimp4_read(MP4D_demux_t *mp4, int nb, int *eof_flag)
{
    uint32_t v = 0; int last_byte;
    switch (nb)
    {
    case 4: v = (v << 8) | minimp4_fgets(mp4);
    case 3: v = (v << 8) | minimp4_fgets(mp4);
    case 2: v = (v << 8) | minimp4_fgets(mp4);
    default:
    case 1: v = (v << 8) | (last_byte = minimp4_fgets(mp4));
    }
    if (last_byte < 0)
    {
        *eof_flag = 1;
    }
    return v;
}

/**
*   Read given number of bytes, but no more than *payload_bytes specifies...
*   Used to read box payload
*/
static uint32_t read_payload(MP4D_demux_t *mp4, unsigned nb, boxsize_t *payload_bytes, int *eof_flag)
{
    if (*payload_bytes < nb)
    {
        *eof_flag = 1;
        nb = (int)*payload_bytes;
    }
    *payload_bytes -= nb;

    return minimp4_read(mp4, nb, eof_flag);
}

/**
*   Skips given number of bytes.
*   Avoid math operations with fpos_t
*/
static void my_fseek(MP4D_demux_t *mp4, boxsize_t pos, int *eof_flag)
{
    mp4->read_pos += pos;
    if (mp4->read_pos >= mp4->read_size)
        *eof_flag = 1;
}

#define READ(n) read_payload(mp4, n, &payload_bytes, &eof_flag)
#define SKIP(n) { boxsize_t t = MINIMP4_MIN(payload_bytes, n); my_fseek(mp4, t, &eof_flag); payload_bytes -= t; }
#define MALLOC(t, p, size) p = (t)malloc(size); if (!(p)) { ERROR("out of memory"); }

/*
*   On error: release resources.
*/
#define RETURN_ERROR(mess) {        \
    TRACE(("\nMP4 ERROR: " mess));  \
    MP4D_close(mp4);                \
    return 0;                       \
}

/*
*   Any errors, occurred on top-level hierarchy is passed to exit check: 'if (!mp4->track_count) ... '
*/
#define ERROR(mess)  \
    if (!depth)      \
        break;       \
    else             \
        RETURN_ERROR(mess);

typedef enum { BOX_ATOM, BOX_OD } boxtype_t;

int MP4D_open(MP4D_demux_t *mp4, int (*read_callback)(int64_t offset, void *buffer, size_t size, void *token), void *token, int64_t file_size)
{
    // box stack size
    int depth = 0;

    struct
    {
        // remaining bytes for box in the stack
        boxsize_t bytes;

        // kind of box children's: OD chunks handled in the same manner as name chunks
        boxtype_t format;

    } stack[MAX_CHUNKS_DEPTH];

#if MP4D_TRACE_SUPPORTED
    // path of current element: List0/List1/... etc
    uint32_t box_path[MAX_CHUNKS_DEPTH];
#endif

    int eof_flag = 0;
    unsigned i;
    MP4D_track_t *tr = NULL;

    if (!mp4 || !read_callback)
    {
        TRACE(("\nERROR: invlaid arguments!"));
        return 0;
    }

    memset(mp4, 0, sizeof(MP4D_demux_t));
    mp4->read_callback = read_callback;
    mp4->token = token;
    mp4->read_size = file_size;

    stack[0].format = BOX_ATOM;   // start with atom box
    stack[0].bytes = 0;           // never accessed

    do
    {
        // List of boxes, derived from 'FullBox'
        //                ~~~~~~~~~~~~~~~~~~~~~
        // need read version field and check version for these boxes
        static const struct
        {
            uint32_t name;
            unsigned max_version;
            unsigned use_track_flag;
        } g_fullbox[] =
        {
#if MP4D_INFO_SUPPORTED
            {BOX_mdhd, 1, 1},
            {BOX_mvhd, 1, 0},
            {BOX_hdlr, 0, 0},
            {BOX_meta, 0, 0},   // Android can produce meta box without 'FullBox' field, comment this line to simulate the bug
#endif
#if MP4D_TRACE_TIMESTAMPS
            {BOX_stts, 0, 0},
            {BOX_ctts, 0, 0},
#endif
            {BOX_stz2, 0, 1},
            {BOX_stsz, 0, 1},
            {BOX_stsc, 0, 1},
            {BOX_stco, 0, 1},
            {BOX_co64, 0, 1},
            {BOX_stsd, 0, 0},
            {BOX_esds, 0, 1}    // esds does not use track, but switches to OD mode. Check here, to avoid OD check
        };

        // List of boxes, which contains other boxes ('envelopes')
        // Parser will descend down for boxes in this list, otherwise parsing will proceed to
        // the next sibling box
        // OD boxes handled in the same way as atom boxes...
        static const struct
        {
            uint32_t name;
            boxtype_t type;
        } g_envelope_box[] =
        {
            {BOX_esds, BOX_OD},     // TODO: BOX_esds can be used for both audio and video, but this code supports audio only!
            {OD_ESD,   BOX_OD},
            {OD_DCD,   BOX_OD},
            {OD_DSI,   BOX_OD},
            {BOX_trak, BOX_ATOM},
            {BOX_moov, BOX_ATOM},
            //{BOX_moof, BOX_ATOM},
            {BOX_mdia, BOX_ATOM},
            {BOX_tref, BOX_ATOM},
            {BOX_minf, BOX_ATOM},
            {BOX_dinf, BOX_ATOM},
            {BOX_stbl, BOX_ATOM},
            {BOX_stsd, BOX_ATOM},
            {BOX_mp4a, BOX_ATOM},
            {BOX_mp4s, BOX_ATOM},
#if MP4D_AVC_SUPPORTED
            {BOX_mp4v, BOX_ATOM},
            {BOX_avc1, BOX_ATOM},
            //{BOX_avc2, BOX_ATOM},
            //{BOX_svc1, BOX_ATOM},
#endif
#if MP4D_HEVC_SUPPORTED
            {BOX_hvc1, BOX_ATOM},
#endif
            {BOX_udta, BOX_ATOM},
            {BOX_meta, BOX_ATOM},
            {BOX_ilst, BOX_ATOM}
        };

        uint32_t FullAtomVersionAndFlags = 0;
        boxsize_t payload_bytes;
        boxsize_t box_bytes;
        uint32_t box_name;
#if MP4D_INFO_SUPPORTED
        unsigned char **ptag = NULL;
#endif
        int read_bytes = 0;

        // Read header box type and it's length
        if (stack[depth].format == BOX_ATOM)
        {
            box_bytes = minimp4_read(mp4, 4, &eof_flag);
#if FIX_BAD_ANDROID_META_BOX
broken_android_meta_hack:
#endif
            if (eof_flag)
                break;  // normal exit

            if (box_bytes >= 2 && box_bytes < 8)
            {
                ERROR("invalid box size (broken file?)");
            }

            box_name  = minimp4_read(mp4, 4, &eof_flag);
            read_bytes = 8;

            // Decode box size
            if (box_bytes == 0 ||                         // standard indication of 'till eof' size
                box_bytes == (boxsize_t)0xFFFFFFFFU       // some files uses non-standard 'till eof' signaling
                )
            {
                box_bytes = ~(boxsize_t)0;
            }

            payload_bytes = box_bytes - 8;

            if (box_bytes == 1)           // 64-bit sizes
            {
                TRACE(("\n64-bit chunk encountered"));

                box_bytes = minimp4_read(mp4, 4, &eof_flag);
#if MP4D_64BIT_SUPPORTED
                box_bytes <<= 32;
                box_bytes |= minimp4_read(mp4, 4, &eof_flag);
#else
                if (box_bytes)
                {
                    ERROR("UNSUPPORTED FEATURE: MP4BoxHeader(): 64-bit boxes not supported!");
                }
                box_bytes = minimp4_read(mp4, 4, &eof_flag);
#endif
                if (box_bytes < 16)
                {
                    ERROR("invalid box size (broken file?)");
                }
                payload_bytes = box_bytes - 16;
            }

            // Read and check box version for some boxes
            for (i = 0; i < NELEM(g_fullbox); i++)
            {
                if (box_name == g_fullbox[i].name)
                {
                    FullAtomVersionAndFlags = READ(4);
                    read_bytes += 4;

#if FIX_BAD_ANDROID_META_BOX
                    // Fix invalid BOX_meta, found in some Android-produced MP4
                    // This branch is optional: bad box would be skipped
                    if (box_name == BOX_meta)
                    {
                        if (FullAtomVersionAndFlags >= 8 &&  FullAtomVersionAndFlags < payload_bytes)
                        {
                            if (box_bytes > stack[depth].bytes)
                            {
                                ERROR("broken file structure!");
                            }
                            stack[depth].bytes -= box_bytes;;
                            depth++;
                            stack[depth].bytes = payload_bytes + 4; // +4 need for missing header
                            stack[depth].format = BOX_ATOM;
                            box_bytes = FullAtomVersionAndFlags;
                            TRACE(("Bad metadata box detected (Android bug?)!\n"));
                            goto broken_android_meta_hack;
                        }
                    }
#endif // FIX_BAD_ANDROID_META_BOX

                    if ((FullAtomVersionAndFlags >> 24) > g_fullbox[i].max_version)
                    {
                        ERROR("unsupported box version!");
                    }
                    if (g_fullbox[i].use_track_flag && !tr)
                    {
                        ERROR("broken file structure!");
                    }
                }
            }
        } else // stack[depth].format == BOX_OD
        {
            int val;
            box_name = OD_BASE + minimp4_read(mp4, 1, &eof_flag);     // 1-byte box type
            read_bytes += 1;
            if (eof_flag)
                break;

            payload_bytes = 0;
            box_bytes = 1;
            do
            {
                val = minimp4_read(mp4, 1, &eof_flag);
                read_bytes += 1;
                if (eof_flag)
                {
                    ERROR("premature EOF!");
                }
                payload_bytes = (payload_bytes << 7) | (val & 0x7F);
                box_bytes++;
            } while (val & 0x80);
            box_bytes += payload_bytes;
        }

#if MP4D_TRACE_SUPPORTED
        box_path[depth] = (box_name >> 24) | (box_name << 24) | ((box_name >> 8) & 0x0000FF00) | ((box_name << 8) & 0x00FF0000);
        TRACE(("%2d  %8d %.*s  (%d bytes remains for sibilings) \n", depth, (int)box_bytes, depth*4, (char*)box_path, (int)stack[depth].bytes));
#endif

        // Check that box size <= parent size
        if (depth)
        {
            // Skip box with bad size
            assert(box_bytes > 0);
            if (box_bytes > stack[depth].bytes)
            {
                TRACE(("Wrong %c%c%c%c box size: broken file?\n", (box_name >> 24)&255, (box_name >> 16)&255, (box_name >> 8)&255, box_name&255));
                box_bytes = stack[depth].bytes;
                box_name = 0;
                payload_bytes = box_bytes - read_bytes;
            }
            stack[depth].bytes -= box_bytes;
        }

        // Read box header
        switch(box_name)
        {
        case BOX_stz2:  //ISO/IEC 14496-1 Page 38. Section 8.17.2 - Sample Size Box.
        case BOX_stsz:
            {
                int size = 0;
                uint32_t sample_size = READ(4);
                tr->sample_count = READ(4);
                MALLOC(unsigned int*, tr->entry_size, tr->sample_count*4);
                for (i = 0; i < tr->sample_count; i++)
                {
                    if (box_name == BOX_stsz)
                    {
                       tr->entry_size[i] = (sample_size?sample_size:READ(4));
                    } else
                    {
                        switch (sample_size & 0xFF)
                        {
                        case 16:
                            tr->entry_size[i] = READ(2);
                            break;
                        case  8:
                            tr->entry_size[i] = READ(1);
                            break;
                        case  4:
                            if (i & 1)
                            {
                                tr->entry_size[i] = size & 15;
                            } else
                            {
                                size = READ(1);
                                tr->entry_size[i] = (size >> 4);
                            }
                            break;
                        }
                    }
                }
            }
            break;

        case BOX_stsc:  //ISO/IEC 14496-12 Page 38. Section 8.18 - Sample To Chunk Box.
            tr->sample_to_chunk_count = READ(4);
            MALLOC(MP4D_sample_to_chunk_t*, tr->sample_to_chunk, tr->sample_to_chunk_count*sizeof(tr->sample_to_chunk[0]));
            for (i = 0; i < tr->sample_to_chunk_count; i++)
            {
                tr->sample_to_chunk[i].first_chunk = READ(4);
                tr->sample_to_chunk[i].samples_per_chunk = READ(4);
                SKIP(4);    // sample_description_index
            }
            break;
#if MP4D_TRACE_TIMESTAMPS || MP4D_TIMESTAMPS_SUPPORTED
        case BOX_stts:
            {
                unsigned count = READ(4);
                unsigned j, k = 0, ts = 0, ts_count = count;
#if MP4D_TIMESTAMPS_SUPPORTED
                MALLOC(unsigned int*, tr->timestamp, ts_count*4);
                MALLOC(unsigned int*, tr->duration, ts_count*4);
#endif

                for (i = 0; i < count; i++)
                {
                    unsigned sc = READ(4);
                    int d =  READ(4);
                    TRACE(("sample %8d count %8d duration %8d\n", i, sc, d));
#if MP4D_TIMESTAMPS_SUPPORTED
                    if (k + sc > ts_count)
                    {
                        ts_count = k + sc;
                        tr->timestamp = (unsigned int*)realloc(tr->timestamp, ts_count * sizeof(unsigned));
                        tr->duration  = (unsigned int*)realloc(tr->duration,  ts_count * sizeof(unsigned));
                    }
                    for (j = 0; j < sc; j++)
                    {
                        tr->duration[k] = d;
                        tr->timestamp[k++] = ts;
                        ts += d;
                    }
#endif
                }
            }
            break;
        case BOX_ctts:
            {
                unsigned count = READ(4);
                for (i = 0; i < count; i++)
                {
                    int sc = READ(4);
                    int d =  READ(4);
                    (void)sc;
                    (void)d;
                    TRACE(("sample %8d count %8d decoding to composition offset %8d\n", i, sc, d));
                }
            }
            break;
#endif
        case BOX_stco:  //ISO/IEC 14496-12 Page 39. Section 8.19 - Chunk Offset Box.
        case BOX_co64:
            tr->chunk_count = READ(4);
            MALLOC(MP4D_file_offset_t*, tr->chunk_offset, tr->chunk_count*sizeof(MP4D_file_offset_t));
            for (i = 0; i < tr->chunk_count; i++)
            {
                tr->chunk_offset[i] = READ(4);
                if (box_name == BOX_co64)
                {
#if !MP4D_64BIT_SUPPORTED
                    if (tr->chunk_offset[i])
                    {
                        ERROR("UNSUPPORTED FEATURE: 64-bit chunk_offset not supported!");
                    }
#endif
                    tr->chunk_offset[i] <<= 32;
                    tr->chunk_offset[i] |= READ(4);
                }
            }
            break;

#if MP4D_INFO_SUPPORTED
        case BOX_mvhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8 + 8 : 4 + 4);
            mp4->timescale = READ(4);
            mp4->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? READ(4) : 0;
            mp4->duration_lo = READ(4);
            SKIP(4 + 2 + 2 + 4*2 + 4*9 + 4*6 + 4);
            break;

        case BOX_mdhd:
            SKIP(((FullAtomVersionAndFlags >> 24) == 1) ? 8 + 8 : 4 + 4);
            tr->timescale = READ(4);
            tr->duration_hi = ((FullAtomVersionAndFlags >> 24) == 1) ? READ(4) : 0;
            tr->duration_lo = READ(4);

            {
                int ISO_639_2_T = READ(2);
                tr->language[2] = (ISO_639_2_T & 31) + 0x60; ISO_639_2_T >>= 5;
                tr->language[1] = (ISO_639_2_T & 31) + 0x60; ISO_639_2_T >>= 5;
                tr->language[0] = (ISO_639_2_T & 31) + 0x60;
            }
            // the rest of this box is skipped by default ...
            break;

        case BOX_hdlr:
            if (tr) // When this box is within 'meta' box, the track may not be avaialable
            {
                SKIP(4); // pre_defined
                tr->handler_type = READ(4);
            }
            // typically hdlr box does not contain any useful info.
            // the rest of this box is skipped by default ...
            break;

        case BOX_btrt:
            if (!tr)
            {
                ERROR("broken file structure!");
            }

            SKIP(4 + 4);
            tr->avg_bitrate_bps = READ(4);
            break;

            // Set pointer to tag to be read...
        case BOX_calb: ptag = &mp4->tag.album;   break;
        case BOX_cART: ptag = &mp4->tag.artist;  break;
        case BOX_cnam: ptag = &mp4->tag.title;   break;
        case BOX_cday: ptag = &mp4->tag.year;    break;
        case BOX_ccmt: ptag = &mp4->tag.comment; break;
        case BOX_cgen: ptag = &mp4->tag.genre;   break;

#endif

        case BOX_stsd:
            SKIP(4); // entry_count, BOX_mp4a & BOX_mp4v boxes follows immediately
            break;

        case BOX_mp4s:  // private stream
            if (!tr)
            {
                ERROR("broken file structure!");
            }
            SKIP(6*1 + 2/*Base SampleEntry*/);
            break;

        case BOX_mp4a:
            if (!tr)
            {
                ERROR("broken file structure!");
            }
#if MP4D_INFO_SUPPORTED
            SKIP(6*1+2/*Base SampleEntry*/  + 4*2);
            tr->SampleDescription.audio.channelcount = READ(2);
            SKIP(2/*samplesize*/ + 2 + 2);
            tr->SampleDescription.audio.samplerate_hz = READ(4) >> 16;
#else
            SKIP(28);
#endif
            break;

#if MP4D_AVC_SUPPORTED
        case BOX_avc1:  // AVCSampleEntry extends VisualSampleEntry
//         case BOX_avc2:   - no test
//         case BOX_svc1:   - no test
        case BOX_mp4v:
            if (!tr)
            {
                ERROR("broken file structure!");
            }
#if MP4D_INFO_SUPPORTED
            SKIP(6*1 + 2/*Base SampleEntry*/ + 2 + 2 + 4*3);
            tr->SampleDescription.video.width  = READ(2);
            tr->SampleDescription.video.height = READ(2);
            // frame_count is always 1
            // compressorname is rarely set..
            SKIP(4 + 4 + 4 + 2/*frame_count*/ + 32/*compressorname*/ + 2 + 2);
#else
            SKIP(78);
#endif
            // ^^^ end of VisualSampleEntry
            // now follows for BOX_avc1:
            //      BOX_avcC
            //      BOX_btrt (optional)
            //      BOX_m4ds (optional)
            // for BOX_mp4v:
            //      BOX_esds
            break;

        case BOX_avcC:  // AVCDecoderConfigurationRecord()
            // hack: AAC-specific DSI field reused (for it have same purpoose as sps/pps)
            // TODO: check this hack if BOX_esds co-exist with BOX_avcC
            tr->object_type_indication = MP4_OBJECT_TYPE_AVC;
            tr->dsi = (unsigned char*)malloc((size_t)box_bytes);
            tr->dsi_bytes = (unsigned)box_bytes;
            {
                int spspps;
                unsigned char *p = tr->dsi;
                unsigned int configurationVersion = READ(1);
                unsigned int AVCProfileIndication = READ(1);
                unsigned int profile_compatibility = READ(1);
                unsigned int AVCLevelIndication = READ(1);
                //bit(6) reserved =
                unsigned int lengthSizeMinusOne = READ(1) & 3;

                (void)configurationVersion;
                (void)AVCProfileIndication;
                (void)profile_compatibility;
                (void)AVCLevelIndication;
                (void)lengthSizeMinusOne;

                for (spspps = 0; spspps < 2; spspps++)
                {
                    unsigned int numOfSequenceParameterSets= READ(1);
                    if (!spspps)
                    {
                         numOfSequenceParameterSets &= 31;  // clears 3 msb for SPS
                    }
                    *p++ = numOfSequenceParameterSets;
                    for (i = 0; i < numOfSequenceParameterSets; i++)
                    {
                        unsigned k, sequenceParameterSetLength = READ(2);
                        *p++ = sequenceParameterSetLength >> 8;
                        *p++ = sequenceParameterSetLength ;
                        for (k = 0; k < sequenceParameterSetLength; k++)
                        {
                            *p++ = READ(1);
                        }
                    }
                }
            }
            break;
#endif  // MP4D_AVC_SUPPORTED

        case OD_ESD:
            {
                unsigned flags = READ(3);   // ES_ID(2) + flags(1)

                if (flags & 0x80)       // steamdependflag
                {
                    SKIP(2);            // dependsOnESID
                }
                if (flags & 0x40)       // urlflag
                {
                    unsigned bytecount = READ(1);
                    SKIP(bytecount);    // skip URL
                }
                if (flags & 0x20)       // ocrflag (was reserved in MPEG-4 v.1)
                {
                    SKIP(2);            // OCRESID
                }
                break;
            }

        case OD_DCD:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            tr->object_type_indication = READ(1);
#if MP4D_INFO_SUPPORTED
            tr->stream_type = READ(1) >> 2;
            SKIP(3/*bufferSizeDB*/ + 4/*maxBitrate*/);
            tr->avg_bitrate_bps = READ(4);
#else
            SKIP(1+3+4+4);
#endif
            break;

        case OD_DSI:        //ISO/IEC 14496-1 Page 28. Section 8.6.5 - DecoderConfigDescriptor.
            assert(tr);     // ensured by g_fullbox[] check
            if (!tr->dsi && payload_bytes)
            {
                MALLOC(unsigned char*, tr->dsi, (int)payload_bytes);
                for (i = 0; i < payload_bytes; i++)
                {
                    tr->dsi[i] = minimp4_read(mp4, 1, &eof_flag);    // These bytes available due to check above
                }
                tr->dsi_bytes = i;
                payload_bytes -= i;
                break;
            }

        default:
            TRACE(("[%c%c%c%c]  %d\n", box_name >> 24, box_name >> 16, box_name >> 8, box_name, (int)payload_bytes));
        }

#if MP4D_INFO_SUPPORTED
        // Read tag is tag pointer is set
        if (ptag && !*ptag && payload_bytes > 16)
        {
#if 0
            uint32_t size = READ(4);
            uint32_t data = READ(4);
            uint32_t class = READ(4);
            uint32_t x1 = READ(4);
            TRACE(("%2d  %2d %2d ", size, class, x1));
#else
            SKIP(4 + 4 + 4 + 4);
#endif
            MALLOC(unsigned char*, *ptag, (unsigned)payload_bytes + 1);
            for (i = 0; payload_bytes != 0; i++)
            {
                (*ptag)[i] = READ(1);
            }
            (*ptag)[i] = 0; // zero-terminated string
        }
#endif

        if (box_name == BOX_trak)
        {
            // New track found: allocate memory using realloc()
            // Typically there are 1 audio track for AAC audio file,
            // 4 tracks for movie file,
            // 3-5 tracks for scalable audio (CELP+AAC)
            // and up to 50 tracks for BSAC scalable audio
            void *mem = realloc(mp4->track, (mp4->track_count + 1)*sizeof(MP4D_track_t));
            if (!mem)
            {
                // if realloc fails, it does not deallocate old pointer!
                ERROR("out of memory");
            }
            mp4->track = (MP4D_track_t*)mem;
            tr = mp4->track + mp4->track_count++;
            memset(tr, 0, sizeof(MP4D_track_t));
        } else if (box_name == BOX_meta)
        {
            tr = NULL;  // Avoid update of 'hdlr' box, which may contains in the 'meta' box
        }

        // If this box is envelope, save it's size in box stack
        for (i = 0; i < NELEM(g_envelope_box); i++)
        {
            if (box_name == g_envelope_box[i].name)
            {
                if (++depth >= MAX_CHUNKS_DEPTH)
                {
                    ERROR("too deep atoms nesting!");
                }
                stack[depth].bytes = payload_bytes;
                stack[depth].format = g_envelope_box[i].type;
                break;
            }
        }

        // if box is not envelope, just skip it
        if (i == NELEM(g_envelope_box))
        {
            if (payload_bytes > file_size)
            {
                eof_flag = 1;
            } else
            {
                SKIP(payload_bytes);
            }
        }

        // remove empty boxes from stack
        // don't touch box with index 0 (which indicates whole file)
        while (depth > 0 && !stack[depth].bytes)
        {
            depth--;
        }

    } while(!eof_flag);

    if (!mp4->track_count)
    {
        RETURN_ERROR("no tracks found");
    }
    return 1;
}

/**
*   Find chunk, containing given sample.
*   Returns chunk number, and first sample in this chunk.
*/
static int sample_to_chunk(MP4D_track_t *tr, unsigned nsample, unsigned *nfirst_sample_in_chunk)
{
    unsigned chunk_group = 0, nc;
    unsigned sum = 0;
    *nfirst_sample_in_chunk = 0;
    if (tr->chunk_count <= 1)
    {
        return 0;
    }
    for (nc = 0; nc < tr->chunk_count; nc++)
    {
        if (chunk_group + 1 < tr->sample_to_chunk_count     // stuck at last entry till EOF
            && nc + 1 ==    // Chunks counted starting with '1'
            tr->sample_to_chunk[chunk_group + 1].first_chunk)    // next group?
        {
            chunk_group++;
        }

        sum += tr->sample_to_chunk[chunk_group].samples_per_chunk;
        if (nsample < sum)
            return nc;

        // TODO: this can be calculated once per file
        *nfirst_sample_in_chunk = sum;
    }
    return -1;
}

// Exported API function
MP4D_file_offset_t MP4D_frame_offset(const MP4D_demux_t *mp4, unsigned ntrack, unsigned nsample, unsigned *frame_bytes, unsigned *timestamp, unsigned *duration)
{
    MP4D_track_t *tr = mp4->track + ntrack;
    unsigned ns;
    int nchunk = sample_to_chunk(tr, nsample, &ns);
    MP4D_file_offset_t offset;

    if (nchunk < 0)
    {
        *frame_bytes = 0;
        return 0;
    }

    offset = tr->chunk_offset[nchunk];
    for (; ns < nsample; ns++)
    {
        offset += tr->entry_size[ns];
    }

    *frame_bytes = tr->entry_size[ns];

    if (timestamp)
    {
#if MP4D_TIMESTAMPS_SUPPORTED
        *timestamp = tr->timestamp[ns];
#else
        *timestamp = 0;
#endif
    }
    if (duration)
    {
#if MP4D_TIMESTAMPS_SUPPORTED
        *duration = tr->duration[ns];
#else
        *duration = 0;
#endif
    }

    return offset;
}

#define FREE(x) if (x) {free(x); x = NULL;}

// Exported API function
void MP4D_close(MP4D_demux_t *mp4)
{
    while (mp4->track_count)
    {
        MP4D_track_t *tr = mp4->track + --mp4->track_count;
        FREE(tr->entry_size);
#if MP4D_TIMESTAMPS_SUPPORTED
        FREE(tr->timestamp);
        FREE(tr->duration);
#endif
        FREE(tr->sample_to_chunk);
        FREE(tr->chunk_offset);
        FREE(tr->dsi);
    }
    FREE(mp4->track);
#if MP4D_INFO_SUPPORTED
    FREE(mp4->tag.title);
    FREE(mp4->tag.artist);
    FREE(mp4->tag.album);
    FREE(mp4->tag.year);
    FREE(mp4->tag.comment);
    FREE(mp4->tag.genre);
#endif
}

static int skip_spspps(const unsigned char *p, int nbytes, int nskip)
{
    int i, k = 0;
    for (i = 0; i < nskip; i++)
    {
        unsigned segmbytes;
        if (k > nbytes - 2)
            return -1;
        segmbytes = p[k]*256 + p[k+1];
        k += 2 + segmbytes;
    }
    return k;
}

static const void *MP4D_read_spspps(const MP4D_demux_t *mp4, unsigned int ntrack, int pps_flag, int nsps, int *sps_bytes)
{
    int sps_count, skip_bytes;
    int bytepos = 0;
    unsigned char *p = mp4->track[ntrack].dsi;
    if (ntrack >= mp4->track_count)
        return NULL;
    if (mp4->track[ntrack].object_type_indication != MP4_OBJECT_TYPE_AVC)
        return NULL;    // SPS/PPS are specific for AVC format only

    if (pps_flag)
    {
        // Skip all SPS
        sps_count = p[bytepos++];
        skip_bytes = skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, sps_count);
        if (skip_bytes < 0)
            return NULL;
        bytepos += skip_bytes;
    }

    // Skip sps/pps before the given target
    sps_count = p[bytepos++];
    if (nsps >= sps_count)
        return NULL;
    skip_bytes = skip_spspps(p+bytepos, mp4->track[ntrack].dsi_bytes - bytepos, nsps);
    if (skip_bytes < 0)
        return NULL;
    bytepos += skip_bytes;
    *sps_bytes = p[bytepos]*256 + p[bytepos+1];
    return p + bytepos + 2;
}


const void *MP4D_read_sps(const MP4D_demux_t *mp4, unsigned int ntrack, int nsps, int *sps_bytes)
{
    return MP4D_read_spspps(mp4, ntrack, 0, nsps, sps_bytes);
}

const void *MP4D_read_pps(const MP4D_demux_t *mp4, unsigned int ntrack, int npps, int *pps_bytes)
{
    return MP4D_read_spspps(mp4, ntrack, 1, npps, pps_bytes);
}

#if MP4D_PRINT_INFO_SUPPORTED
/************************************************************************/
/*  Purely informational part, may be removed for embedded applications */
/************************************************************************/

//
// Decodes ISO/IEC 14496 MP4 stream type to ASCII string
//
static const char *GetMP4StreamTypeName(int streamType)
{
    switch (streamType)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "ObjectDescriptorStream";
    case 0x02: return "ClockReferenceStream";
    case 0x03: return "SceneDescriptionStream";
    case 0x04: return "VisualStream";
    case 0x05: return "AudioStream";
    case 0x06: return "MPEG7Stream";
    case 0x07: return "IPMPStream";
    case 0x08: return "ObjectContentInfoStream";
    case 0x09: return "MPEGJStream";
    default:
        if (streamType >= 0x20 && streamType <= 0x3F)
        {
            return "User private";
        } else
        {
            return "Reserved for ISO use";
        }
    }
}

//
// Decodes ISO/IEC 14496 MP4 object type to ASCII string
//
static const char *GetMP4ObjectTypeName(int objectTypeIndication)
{
    switch (objectTypeIndication)
    {
    case 0x00: return "Forbidden";
    case 0x01: return "Systems ISO/IEC 14496-1";
    case 0x02: return "Systems ISO/IEC 14496-1";
    case 0x20: return "Visual ISO/IEC 14496-2";
    case 0x40: return "Audio ISO/IEC 14496-3";
    case 0x60: return "Visual ISO/IEC 13818-2 Simple Profile";
    case 0x61: return "Visual ISO/IEC 13818-2 Main Profile";
    case 0x62: return "Visual ISO/IEC 13818-2 SNR Profile";
    case 0x63: return "Visual ISO/IEC 13818-2 Spatial Profile";
    case 0x64: return "Visual ISO/IEC 13818-2 High Profile";
    case 0x65: return "Visual ISO/IEC 13818-2 422 Profile";
    case 0x66: return "Audio ISO/IEC 13818-7 Main Profile";
    case 0x67: return "Audio ISO/IEC 13818-7 LC Profile";
    case 0x68: return "Audio ISO/IEC 13818-7 SSR Profile";
    case 0x69: return "Audio ISO/IEC 13818-3";
    case 0x6A: return "Visual ISO/IEC 11172-2";
    case 0x6B: return "Audio ISO/IEC 11172-3";
    case 0x6C: return "Visual ISO/IEC 10918-1";
    case 0xFF: return "no object type specified";
    default:
        if (objectTypeIndication >= 0xC0 && objectTypeIndication <= 0xFE)
            return "User private";
        else
            return "Reserved for ISO use";
    }
}

/**
*   Print MP4 information to stdout.
*   Subject for customization to particular application

Output Example #1: movie file

MP4 FILE: 7 tracks found. Movie time 104.12 sec

No|type|lng| duration           | bitrate| Stream type            | Object type
 0|odsm|fre|   0.00 s      1 frm|       0| Forbidden              | Forbidden
 1|sdsm|fre|   0.00 s      1 frm|       0| Forbidden              | Forbidden
 2|vide|```| 104.12 s   2603 frm| 1960559| VisualStream           | Visual ISO/IEC 14496-2   -  720x304
 3|soun|ger| 104.06 s   2439 frm|  191242| AudioStream            | Audio ISO/IEC 14496-3    -  6 ch 24000 hz
 4|soun|eng| 104.06 s   2439 frm|  194171| AudioStream            | Audio ISO/IEC 14496-3    -  6 ch 24000 hz
 5|subp|ger|  71.08 s     25 frm|       0| Forbidden              | Forbidden
 6|subp|eng|  71.08 s     25 frm|       0| Forbidden              | Forbidden

Output Example #2: audio file with tags

MP4 FILE: 1 tracks found. Movie time 92.42 sec
title = 86-Second Blowout
artist = Yo La Tengo
album = May I Sing With Me
year = 1992

No|type|lng| duration           | bitrate| Stream type            | Object type
 0|mdir|und|  92.42 s   3980 frm|  128000| AudioStream            | Audio ISO/IEC 14496-3MP4 FILE: 1 tracks found. Movie time 92.42 sec

*/
void MP4D_printf_info(const MP4D_demux_t *mp4)
{
    unsigned i;
    printf("\nMP4 FILE: %d tracks found. Movie time %.2f sec\n", mp4->track_count, (4294967296.0*mp4->duration_hi + mp4->duration_lo) / mp4->timescale);
#define STR_TAG(name) if (mp4->tag.name)  printf("%10s = %s\n", #name, mp4->tag.name)
    STR_TAG(title);
    STR_TAG(artist);
    STR_TAG(album);
    STR_TAG(year);
    STR_TAG(comment);
    STR_TAG(genre);
    printf("\nNo|type|lng| duration           | bitrate| %-23s| Object type", "Stream type");
    for (i = 0; i < mp4->track_count; i++)
    {
        MP4D_track_t *tr = mp4->track + i;

        printf("\n%2d|%c%c%c%c|%c%c%c|%7.2f s %6d frm| %7d|", i,
            (tr->handler_type >> 24), (tr->handler_type >> 16), (tr->handler_type >> 8), (tr->handler_type >> 0),
            tr->language[0], tr->language[1], tr->language[2],
            (65536.0*65536.0*tr->duration_hi + tr->duration_lo) / tr->timescale,
            tr->sample_count,
            tr->avg_bitrate_bps);

        printf(" %-23s|", GetMP4StreamTypeName(tr->stream_type));
        printf(" %-23s", GetMP4ObjectTypeName(tr->object_type_indication));

        if (tr->handler_type == MP4D_HANDLER_TYPE_SOUN)
        {
            printf("  -  %d ch %d hz", tr->SampleDescription.audio.channelcount, tr->SampleDescription.audio.samplerate_hz);
        } else if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE)
        {
            printf("  -  %dx%d", tr->SampleDescription.video.width, tr->SampleDescription.video.height);
        }
    }
    printf("\n");
}

#endif // MP4D_PRINT_INFO_SUPPORTED
#endif
