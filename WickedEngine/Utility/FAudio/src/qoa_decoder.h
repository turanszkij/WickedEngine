/* QOA decoder API for FAudio

Copyright (c) 2024 Ethan Lee.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>

Original version:

Copyright (c) 2024, Dominic Szablewski - https://phoboslab.org
SPDX-License-Identifier: MIT

QOA - The "Quite OK Audio" format for fast, lossy audio compression


-- Data Format

QOA encodes pulse-code modulated (PCM) audio data with up to 255 channels,
sample rates from 1 up to 16777215 hertz and a bit depth of 16 bits.

The compression method employed in QOA is lossy; it discards some information
from the uncompressed PCM data. For many types of audio signals this compression
is "transparent", i.e. the difference from the original file is often not
audible.

QOA encodes 20 samples of 16 bit PCM data into slices of 64 bits. A single
sample therefore requires 3.2 bits of storage space, resulting in a 5x
compression (16 / 3.2).

A QOA file consists of an 8 byte file header, followed by a number of frames.
Each frame contains an 8 byte frame header, the current 16 byte en-/decoder
state per channel and 256 slices per channel. Each slice is 8 bytes wide and
encodes 20 samples of audio data.

All values, including the slices, are big endian. The file layout is as follows:

struct {
	struct {
		char     magic[4];         // magic bytes "qoaf"
		uint32_t samples;          // samples per channel in this file
	} file_header;

	struct {
		struct {
			uint8_t  num_channels; // no. of channels
			uint24_t samplerate;   // samplerate in hz
			uint16_t fsamples;     // samples per channel in this frame
			uint16_t fsize;        // frame size (includes this header)
		} frame_header;

		struct {
			int16_t history[4];    // most recent last
			int16_t weights[4];    // most recent last
		} lms_state[num_channels];

		qoa_slice_t slices[256][num_channels];

	} frames[ceil(samples / (256 * 20))];
} qoa_file_t;

Each `qoa_slice_t` contains a quantized scalefactor `sf_quant` and 20 quantized
residuals `qrNN`:

.- QOA_SLICE -- 64 bits, 20 samples --------------------------/  /------------.
|        Byte[0]         |        Byte[1]         |  Byte[2]  \  \  Byte[7]   |
| 7  6  5  4  3  2  1  0 | 7  6  5  4  3  2  1  0 | 7  6  5   /  /    2  1  0 |
|------------+--------+--------+--------+---------+---------+-\  \--+---------|
|  sf_quant  |  qr00  |  qr01  |  qr02  |  qr03   |  qr04   | /  /  |  qr19   |
`-------------------------------------------------------------\  \------------`

Each frame except the last must contain exactly 256 slices per channel. The last
frame may contain between 1 .. 256 (inclusive) slices per channel. The last
slice (for each channel) in the last frame may contain less than 20 samples; the
slice still must be 8 bytes wide, with the unused samples zeroed out.

Channels are interleaved per slice. E.g. for 2 channel stereo:
slice[0] = L, slice[1] = R, slice[2] = L, slice[3] = R ...

A valid QOA file or stream must have at least one frame. Each frame must contain
at least one channel and one sample with a samplerate between 1 .. 16777215
(inclusive).

If the total number of samples is not known by the encoder, the samples in the
file header may be set to 0x00000000 to indicate that the encoder is
"streaming". In a streaming context, the samplerate and number of channels may
differ from frame to frame. For static files (those with samples set to a
non-zero value), each frame must have the same number of channels and same
samplerate.

Note that this implementation of QOA only handles files with a known total
number of samples.

A decoder should support at least 8 channels. The channel layout for channel
counts 1 .. 8 is:

	1. Mono
	2. L, R
	3. L, R, C
	4. FL, FR, B/SL, B/SR
	5. FL, FR, C, B/SL, B/SR
	6. FL, FR, C, LFE, B/SL, B/SR
	7. FL, FR, C, LFE, B, SL, SR
	8. FL, FR, C, LFE, BL, BR, SL, SR

QOA predicts each audio sample based on the previously decoded ones using a
"Sign-Sign Least Mean Squares Filter" (LMS). This prediction plus the
dequantized residual forms the final output sample.

*/

#define QOA_MIN_FILESIZE 16
#define QOA_MAX_CHANNELS 8

#define QOA_SLICE_LEN 20
#define QOA_SLICES_PER_FRAME 256
#define QOA_FRAME_LEN (QOA_SLICES_PER_FRAME * QOA_SLICE_LEN)
#define QOA_LMS_LEN 4
#define QOA_MAGIC 0x716f6166 /* 'qoaf' */

#define QOA_FRAME_SIZE(channels, slices) \
	(8 + QOA_LMS_LEN * 4 * channels + 8 * slices * channels)

typedef struct {
	int history[QOA_LMS_LEN];
	int weights[QOA_LMS_LEN];
} qoa_lms_t;

typedef struct {
	unsigned int channels;
	unsigned int samplerate;
	unsigned int samples;
	qoa_lms_t lms[QOA_MAX_CHANNELS];
	#ifdef QOA_RECORD_TOTAL_ERROR
		double error;
	#endif
} qoa_desc;

typedef struct {
	unsigned char *bytes;
	unsigned int size;
	unsigned int frame_index;
	unsigned int frame_size;
	unsigned short samples_per_channel_per_frame;
	int free_on_close;
	qoa_desc qoa;
} qoa_data;

typedef unsigned long long qoa_uint64_t;

typedef struct qoa qoa;

/* NOTE: this API only supports "static" type QOA files. "streaming" type files are not supported!! */
FAUDIOAPI qoa *qoa_open_from_memory(unsigned char *bytes, unsigned int size, int free_on_close);
FAUDIOAPI qoa *qoa_open_from_filename(const char *filename);
FAUDIOAPI void qoa_attributes(qoa *qoa, unsigned int *channels, unsigned int *samplerate, unsigned int *samples_per_channel_per_frame, unsigned int *total_samples_per_channel);
FAUDIOAPI unsigned int qoa_decode_next_frame(qoa *qoa, short *sample_data); /* decode the next frame into a preallocated buffer */
FAUDIOAPI void qoa_seek_frame(qoa *qoa, int frame_index);
FAUDIOAPI void qoa_decode_entire(qoa *qoa, short *sample_data); /* fill a buffer with the entire qoa data decoded */
FAUDIOAPI void qoa_close(qoa *qoa);

/* The quant_tab provides an index into the dequant_tab for residuals in the
range of -8 .. 8. It maps this range to just 3bits and becomes less accurate at
the higher end. Note that the residual zero is identical to the lowest positive
value. This is mostly fine, since the qoa_div() function always rounds away
from zero. */

static const int qoa_quant_tab[17] = {
	7, 7, 7, 5, 5, 3, 3, 1, /* -8..-1 */
	0,                      /*  0     */
	0, 2, 2, 4, 4, 6, 6, 6  /*  1.. 8 */
};


/* We have 16 different scalefactors. Like the quantized residuals these become
less accurate at the higher end. In theory, the highest scalefactor that we
would need to encode the highest 16bit residual is (2**16)/8 = 8192. However we
rely on the LMS filter to predict samples accurately enough that a maximum
residual of one quarter of the 16 bit range is sufficient. I.e. with the
scalefactor 2048 times the quant range of 8 we can encode residuals up to 2**14.

The scalefactor values are computed as:
scalefactor_tab[s] <- round(pow(s + 1, 2.75)) */

static const int qoa_scalefactor_tab[16] = {
	1, 7, 21, 45, 84, 138, 211, 304, 421, 562, 731, 928, 1157, 1419, 1715, 2048
};


/* The reciprocal_tab maps each of the 16 scalefactors to their rounded
reciprocals 1/scalefactor. This allows us to calculate the scaled residuals in
the encoder with just one multiplication instead of an expensive division. We
do this in .16 fixed point with integers, instead of floats.

The reciprocal_tab is computed as:
reciprocal_tab[s] <- ((1<<16) + scalefactor_tab[s] - 1) / scalefactor_tab[s] */

static const int qoa_reciprocal_tab[16] = {
	65536, 9363, 3121, 1457, 781, 475, 311, 216, 156, 117, 90, 71, 57, 47, 39, 32
};


/* The dequant_tab maps each of the scalefactors and quantized residuals to
their unscaled & dequantized version.

Since qoa_div rounds away from the zero, the smallest entries are mapped to 3/4
instead of 1. The dequant_tab assumes the following dequantized values for each
of the quant_tab indices and is computed as:
float dqt[8] = {0.75, -0.75, 2.5, -2.5, 4.5, -4.5, 7, -7};
dequant_tab[s][q] <- round_ties_away_from_zero(scalefactor_tab[s] * dqt[q])

The rounding employed here is "to nearest, ties away from zero",  i.e. positive
and negative values are treated symmetrically.
*/

static const int qoa_dequant_tab[16][8] = {
	{   1,    -1,    3,    -3,    5,    -5,     7,     -7},
	{   5,    -5,   18,   -18,   32,   -32,    49,    -49},
	{  16,   -16,   53,   -53,   95,   -95,   147,   -147},
	{  34,   -34,  113,  -113,  203,  -203,   315,   -315},
	{  63,   -63,  210,  -210,  378,  -378,   588,   -588},
	{ 104,  -104,  345,  -345,  621,  -621,   966,   -966},
	{ 158,  -158,  528,  -528,  950,  -950,  1477,  -1477},
	{ 228,  -228,  760,  -760, 1368, -1368,  2128,  -2128},
	{ 316,  -316, 1053, -1053, 1895, -1895,  2947,  -2947},
	{ 422,  -422, 1405, -1405, 2529, -2529,  3934,  -3934},
	{ 548,  -548, 1828, -1828, 3290, -3290,  5117,  -5117},
	{ 696,  -696, 2320, -2320, 4176, -4176,  6496,  -6496},
	{ 868,  -868, 2893, -2893, 5207, -5207,  8099,  -8099},
	{1064, -1064, 3548, -3548, 6386, -6386,  9933,  -9933},
	{1286, -1286, 4288, -4288, 7718, -7718, 12005, -12005},
	{1536, -1536, 5120, -5120, 9216, -9216, 14336, -14336},
};


/* The Least Mean Squares Filter is the heart of QOA. It predicts the next
sample based on the previous 4 reconstructed samples. It does so by continuously
adjusting 4 weights based on the residual of the previous prediction.

The next sample is predicted as the sum of (weight[i] * history[i]).

The adjustment of the weights is done with a "Sign-Sign-LMS" that adds or
subtracts the residual to each weight, based on the corresponding sample from
the history. This, surprisingly, is sufficient to get worthwhile predictions.

This is all done with fixed point integers. Hence the right-shifts when updating
the weights and calculating the prediction. */

static int qoa_lms_predict(qoa_lms_t *lms) {
	int prediction = 0;
	int i;
	for (i = 0; i < QOA_LMS_LEN; i++) {
		prediction += lms->weights[i] * lms->history[i];
	}
	return prediction >> 13;
}

static void qoa_lms_update(qoa_lms_t *lms, int sample, int residual) {
	int delta = residual >> 4;
	int i;
	for (i = 0; i < QOA_LMS_LEN; i++) {
		lms->weights[i] += lms->history[i] < 0 ? -delta : delta;
	}

	for (i = 0; i < QOA_LMS_LEN-1; i++) {
		lms->history[i] = lms->history[i+1];
	}
	lms->history[QOA_LMS_LEN-1] = sample;
}


/* qoa_div() implements a rounding division, but avoids rounding to zero for
small numbers. E.g. 0.1 will be rounded to 1. Note that 0 itself still
returns as 0, which is handled in the qoa_quant_tab[].
qoa_div() takes an index into the .16 fixed point qoa_reciprocal_tab as an
argument, so it can do the division with a cheaper integer multiplication. */

static inline int qoa_div(int v, int scalefactor) {
	int reciprocal = qoa_reciprocal_tab[scalefactor];
	int n = (v * reciprocal + (1 << 15)) >> 16;
	n = n + ((v > 0) - (v < 0)) - ((n > 0) - (n < 0)); /* round away from 0 */
	return n;
}

static inline int qoa_clamp(int v, int min, int max) {
	if (v < min) { return min; }
	if (v > max) { return max; }
	return v;
}

/* This specialized clamp function for the signed 16 bit range improves decode
performance quite a bit. The extra if() statement works nicely with the CPUs
branch prediction as this branch is rarely taken. */

static inline int qoa_clamp_s16(int v) {
	if ((unsigned int)(v + 32768) > 65535) {
		if (v < -32768) { return -32768; }
		if (v >  32767) { return  32767; }
	}
	return v;
}

static inline qoa_uint64_t qoa_read_u64(const unsigned char *bytes, unsigned int *p) {
	bytes += *p;
	*p += 8;
	return
		((qoa_uint64_t)(bytes[0]) << 56) | ((qoa_uint64_t)(bytes[1]) << 48) |
		((qoa_uint64_t)(bytes[2]) << 40) | ((qoa_uint64_t)(bytes[3]) << 32) |
		((qoa_uint64_t)(bytes[4]) << 24) | ((qoa_uint64_t)(bytes[5]) << 16) |
		((qoa_uint64_t)(bytes[6]) <<  8) | ((qoa_uint64_t)(bytes[7]) <<  0);
}

/* -----------------------------------------------------------------------------
	Decoder */

static unsigned int qoa_decode_header(qoa_data *data) {
	unsigned int p = 0;
	qoa_uint64_t file_header, frame_header;
	if (data->size < QOA_MIN_FILESIZE) {
		return 0;
	}

	/* Read the file header, verify the magic number ('qoaf') and read the
	total number of samples. */
	file_header = qoa_read_u64(data->bytes, &p);

	if ((file_header >> 32) != QOA_MAGIC) {
		return 0;
	}

	data->qoa.samples = file_header & 0xffffffff;
	if (!data->qoa.samples) {
		return 0;
	}

	/* Peek into the first frame header to get the number of channels and
	the samplerate. */
	frame_header = qoa_read_u64(data->bytes, &p);
	data->qoa.channels   = (frame_header >> 56) & 0x0000ff;
	data->qoa.samplerate = (frame_header >> 32) & 0xffffff;
	data->samples_per_channel_per_frame = (frame_header >> 16) & 0x00ffff;

	if (data->qoa.channels == 0 || data->qoa.channels == 0 || data->qoa.samplerate == 0) {
		return 0;
	}

	return 8;
}

qoa *qoa_open_from_memory(unsigned char *bytes, unsigned int size, int free_on_close)
{
	qoa_data *data = (qoa_data*) malloc(sizeof(qoa_data));
	data->bytes = bytes;
	data->size = size;
	data->frame_index = 0;
	data->free_on_close = free_on_close;
	if (qoa_decode_header(data) == 0)
	{
		qoa_close((qoa*) data);
		return NULL;
	}
	data->frame_size = QOA_FRAME_SIZE(data->qoa.channels, QOA_SLICES_PER_FRAME);
	return (qoa*) data;
}

static qoa *qoa_open_from_file(FILE *file, int free_on_close)
{
	unsigned char *bytes;
	unsigned int len, start;
	start = (unsigned int) ftell(file);
	fseek(file, 0, SEEK_END);
	len = (unsigned int) (ftell(file) - start);
	fseek(file, start, SEEK_SET);

	bytes = malloc(len);
	fread(bytes, 1, len, file);
	fclose(file);

	return qoa_open_from_memory(bytes, len, free_on_close);
}

qoa *qoa_open_from_filename(const char *filename)
{
	FILE *f;
	f = fopen(filename, "rb");

	if (f)
		return qoa_open_from_file(f, TRUE);

	return NULL;
}

void qoa_attributes(
	qoa *qoa,
	unsigned int *channels,
	unsigned int *samplerate,
	unsigned int *samples_per_channel_per_frame,
	unsigned int *total_samples_per_channel
) {
	qoa_data *data = (qoa_data*) qoa;
	*channels = data->qoa.channels;
	*samplerate = data->qoa.samplerate;
	*samples_per_channel_per_frame = data->samples_per_channel_per_frame;
	*total_samples_per_channel = data->qoa.samples;
}

static unsigned int qoa_frame_start(qoa_data *data, int frame_index) {
	return 8 + (data->frame_size * frame_index);
}

unsigned int qoa_decode_next_frame(qoa *qoa, short *sample_data) {
	qoa_data *data = (qoa_data*) qoa;
	unsigned int channels, samplerate, samples, frame_size, data_size,
		num_slices, max_total_samples, sample_index, c, p;
	int scalefactor, slice_start, slice_end, predicted, quantized,
		dequantized, reconstructed, si, i;
	qoa_uint64_t frame_header, history, weights, slice;

	p = qoa_frame_start(data, data->frame_index);

	/* Reached the end */
	if (p >= data->size)
	{
		return 0;
	}

	/* Read and verify the frame header */
	frame_header = qoa_read_u64(data->bytes, &p);
	channels   = (frame_header >> 56) & 0x0000ff;
	samplerate = (frame_header >> 32) & 0xffffff;
	samples    = (frame_header >> 16) & 0x00ffff;
	frame_size = (frame_header      ) & 0x00ffff;

	data_size = frame_size - 8 - QOA_LMS_LEN * 4 * channels;
	num_slices = data_size / 8;
	max_total_samples = num_slices * QOA_SLICE_LEN;

	if (
		channels != data->qoa.channels ||
		samplerate != data->qoa.samplerate ||
		frame_size > data->size ||
		samples * channels > max_total_samples
	) {
		return 0;
	}

	/* Read the LMS state: 4 x 2 bytes history, 4 x 2 bytes weights per channel */
	for (c = 0; c < channels; c++) {
		history = qoa_read_u64(data->bytes, &p);
		weights = qoa_read_u64(data->bytes, &p);
		for (i = 0; i < QOA_LMS_LEN; i++) {
			data->qoa.lms[c].history[i] = ((signed short)(history >> 48));
			history <<= 16;
			data->qoa.lms[c].weights[i] = ((signed short)(weights >> 48));
			weights <<= 16;
		}
	}

	/* Decode all slices for all channels in this frame */
	for (sample_index = 0; sample_index < samples; sample_index += QOA_SLICE_LEN) {
		for (c = 0; c < channels; c++) {
			slice = qoa_read_u64(data->bytes, &p);

			scalefactor = (slice >> 60) & 0xf;
			slice_start = sample_index * channels + c;
			slice_end = qoa_clamp(sample_index + QOA_SLICE_LEN, 0, samples) * channels + c;

			for (si = slice_start; si < slice_end; si += channels) {
				predicted = qoa_lms_predict(&data->qoa.lms[c]);
				quantized = (slice >> 57) & 0x7;
				dequantized = qoa_dequant_tab[scalefactor][quantized];
				reconstructed = qoa_clamp_s16(predicted + dequantized);

				sample_data[si] = reconstructed;
				slice <<= 3;

				qoa_lms_update(&data->qoa.lms[c], reconstructed, dequantized);
			}
		}
	}

	data->frame_index += 1;
	return samples;
}

void qoa_seek_frame(qoa *qoa, int frame_index) {
	qoa_data* data = (qoa_data*) qoa;
	data->frame_index = frame_index;
}

void qoa_decode_entire(qoa *qoa, short *sample_data) {
	qoa_data* data = (qoa_data *) qoa;
	unsigned int frame_count, sample_index, sample_count;
	int total_samples;
	short *sample_ptr;
	uint32_t i;

	/* Calculate the required size of the sample buffer and allocate */
	total_samples = data->qoa.samples * data->qoa.channels;

	frame_count = (data->size - 64) / data->frame_size;
	sample_index = 0;

	for (i = 0; i < frame_count; i += 1)
	{
		sample_ptr = sample_data + sample_index * data->qoa.channels;
		sample_count = qoa_decode_next_frame(qoa, sample_ptr);
		sample_index += sample_count;
	}
}

void qoa_close(qoa *qoa)
{
	qoa_data *data = (qoa_data*) qoa;
	if (data->free_on_close)
	{
		free(data->bytes);
	}
	free(qoa);
}
