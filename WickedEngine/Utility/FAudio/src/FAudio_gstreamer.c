/* FAudio - XAudio Reimplementation for FNA
 *
 * Copyright (c) 2011-2021 Ethan Lee, Luigi Auriemma, and the MonoGame Team
 * Copyright (c) 2019-2021 Andrew Eikum for CodeWeavers, Inc
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ethan "flibitijibibo" Lee <flibitijibibo@flibitijibibo.com>
 *
 */

#ifdef HAVE_GSTREAMER

#include "FAudio_internal.h"

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/app/gstappsink.h>

typedef struct FAudioWMADEC
{
	GstPad *srcpad;
	GstElement *pipeline;
	GstElement *dst;
	GstElement *resampler;
	GstSegment segment;
	uint8_t *convertCache, *prevConvertCache;
	size_t convertCacheLen, prevConvertCacheLen;
	uint32_t curBlock, prevBlock;
	size_t *blockSizes;
	uint32_t blockAlign;
	uint32_t blockCount;
	size_t maxBytes;
} FAudioWMADEC;

#define SIZE_FROM_DST(sample) \
	((sample) * voice->src.format->nChannels * sizeof(float))
#define SIZE_FROM_SRC(sample) \
	((sample) * voice->src.format->nChannels * (voice->src.format->wBitsPerSample / 8))
#define SAMPLES_FROM_SRC(len) \
	((len) / voice->src.format->nChannels / (voice->src.format->wBitsPerSample / 8))
#define SAMPLES_FROM_DST(len) \
	((len) / voice->src.format->nChannels / sizeof(float))
#define SIZE_SRC_TO_DST(len) \
	((len) * (sizeof(float) / (voice->src.format->wBitsPerSample / 8)))
#define SIZE_DST_TO_SRC(len) \
	((len) / (sizeof(float) / (voice->src.format->wBitsPerSample / 8)))

static int FAudio_GSTREAMER_RestartDecoder(FAudioWMADEC *wmadec)
{
	GstEvent *event;

	event = gst_event_new_flush_start();
	gst_pad_push_event(wmadec->srcpad, event);

	event = gst_event_new_flush_stop(TRUE);
	gst_pad_push_event(wmadec->srcpad, event);

	event = gst_event_new_segment(&wmadec->segment);
	gst_pad_push_event(wmadec->srcpad, event);

	wmadec->curBlock = ~0u;
	wmadec->prevBlock = ~0u;

	if (gst_element_set_state(wmadec->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
	{
		return 0;
	}

	return 1;
}

static int FAudio_GSTREAMER_CheckForBusErrors(FAudioVoice *voice)
{
	GstBus *bus;
	GstMessage *msg;
	GError *err = NULL;
	gchar *debug_info = NULL;
	int ret = 0;

	bus = gst_pipeline_get_bus(GST_PIPELINE(voice->src.wmadec->pipeline));

	while((msg = gst_bus_pop_filtered(bus, GST_MESSAGE_ERROR)))
	{
		switch(GST_MESSAGE_TYPE(msg))
		{
		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, &debug_info);
			LOG_ERROR(
				voice->audio,
				"Got gstreamer bus error from %s: %s (%s)",
				GST_OBJECT_NAME(msg->src),
				err->message,
				debug_info ? debug_info : "none"
			)
			g_clear_error(&err);
			g_free(debug_info);
			gst_message_unref(msg);
			ret = 1;
			break;
		default:
			gst_message_unref(msg);
			break;
		}
	}

	gst_object_unref(bus);

	return ret;
}

static size_t FAudio_GSTREAMER_FillConvertCache(
	FAudioVoice *voice,
	FAudioBuffer *buffer,
	size_t availableBytes
) {
	GstBuffer *src, *dst;
	GstSample *sample;
	GstMapInfo info;
	size_t pulled, clipStartBytes, clipEndBytes, toCopyBytes;
	GstAudioClippingMeta *cmeta;
	GstFlowReturn push_ret;

	LOG_FUNC_ENTER(voice->audio)

	/* Write current block to input buffer, push to pipeline */

	clipStartBytes = (
		(size_t) voice->src.wmadec->blockAlign *
		voice->src.wmadec->curBlock
	);
	toCopyBytes = voice->src.wmadec->blockAlign;
	clipEndBytes = clipStartBytes + toCopyBytes;
	if (buffer->AudioBytes < clipEndBytes)
	{
		/* XMA2 assets can ship with undersized ends.
		 * If your first instinct is to "pad with 0xFF, that's what game data trails with,"
		 * sorry to disappoint but it inserts extra silence in loops.
		 * Instead, push an undersized buffer and pray that the decoder handles it properly.
		 * At the time of writing, it's FFmpeg, and it's handling it well.
		 * For everything else, uh, let's assume the same. If you're reading this: sorry.
		 * -ade
		 */
		toCopyBytes = buffer->AudioBytes - clipStartBytes;
	}
	clipStartBytes += (size_t) buffer->pAudioData;

	src = gst_buffer_new_allocate(
		NULL,
		toCopyBytes,
		NULL
	);

	if (	gst_buffer_fill(
			src,
			0,
			(gconstpointer*) clipStartBytes,
			toCopyBytes
		) != toCopyBytes	)
	{
		LOG_ERROR(
			voice->audio,
			"for voice %p, failed to copy whole chunk into buffer",
			voice
		);
		gst_buffer_unref(src);
		return (size_t) -1;
	}

	push_ret = gst_pad_push(voice->src.wmadec->srcpad, src);
	if(	push_ret != GST_FLOW_OK &&
		push_ret != GST_FLOW_EOS	)
	{
		LOG_ERROR(
			voice->audio,
			"for voice %p, pushing buffer failed: 0x%x",
			voice,
			push_ret
		);
		return (size_t) -1;
	}

	pulled = 0;
	while (1)
	{
		/* Pull frames one into cache */
		sample = gst_app_sink_try_pull_sample(
			GST_APP_SINK(voice->src.wmadec->dst),
			0
		);

		if (!sample)
		{
			/* done decoding */
			break;
		}
		dst = gst_sample_get_buffer(sample);
		gst_buffer_map(dst, &info, GST_MAP_READ);

		cmeta = gst_buffer_get_audio_clipping_meta(dst);
		if (cmeta)
		{
			FAudio_assert(cmeta->format == GST_FORMAT_DEFAULT /* == samples */);
			clipStartBytes = SIZE_FROM_DST(cmeta->start);
			clipEndBytes = SIZE_FROM_DST(cmeta->end);
		}
		else
		{
			clipStartBytes = 0;
			clipEndBytes = 0;
		}

		toCopyBytes = FAudio_min(info.size - (clipStartBytes + clipEndBytes), availableBytes - pulled);

		if (voice->src.wmadec->convertCacheLen < pulled + toCopyBytes)
		{
			voice->src.wmadec->convertCacheLen = pulled + toCopyBytes;
			voice->src.wmadec->convertCache = (uint8_t*) voice->audio->pRealloc(
				voice->src.wmadec->convertCache,
				pulled + toCopyBytes
			);
		}


		FAudio_memcpy(voice->src.wmadec->convertCache + pulled,
			info.data + clipStartBytes,
			toCopyBytes
		);

		gst_buffer_unmap(dst, &info);
		gst_sample_unref(sample);

		pulled += toCopyBytes;
	}

	LOG_FUNC_EXIT(voice->audio)

	return pulled;
}

static int FAudio_WMADEC_DecodeBlock(FAudioVoice *voice, FAudioBuffer *buffer, uint32_t block, size_t availableBytes)
{
	FAudioWMADEC *wmadec = voice->src.wmadec;
	uint8_t *tmpBuf;
	size_t tmpLen;

	if (wmadec->curBlock != ~0u && block != wmadec->curBlock + 1)
	{
		/* XAudio2 allows looping back to start of XMA buffers, but nothing else */
		if (block != 0)
		{
			LOG_ERROR(
				voice->audio,
				"for voice %p, out of sequence block: %u (cur: %d)\n",
				voice,
				block,
				wmadec->curBlock
			);
		}
		FAudio_assert(block == 0);
		if (!FAudio_GSTREAMER_RestartDecoder(wmadec))
		{
			LOG_WARNING(
				voice->audio,
				"%s",
				"Restarting decoder failed!"
			)
		}
	}

	/* store previous block to allow for minor rewinds (FAudio quirk) */
	tmpBuf = wmadec->prevConvertCache;
	tmpLen = wmadec->prevConvertCacheLen;
	wmadec->prevConvertCache = wmadec->convertCache;
	wmadec->prevConvertCacheLen = wmadec->convertCacheLen;
	wmadec->convertCache = tmpBuf;
	wmadec->convertCacheLen = tmpLen;

	wmadec->prevBlock = wmadec->curBlock;
	wmadec->curBlock = block;

	wmadec->blockSizes[block] = FAudio_GSTREAMER_FillConvertCache(
		voice,
		buffer,
		availableBytes
	);

	return wmadec->blockSizes[block] != (size_t) -1;
}

static void FAudio_INTERNAL_DecodeGSTREAMER(
	FAudioVoice *voice,
	FAudioBuffer *buffer,
	float *decodeCache,
	uint32_t samples
) {
	size_t byteOffset, siz, availableBytes, blockCount, maxBytes;
	uint32_t curBlock, curBufferOffset;
	uint8_t *convertCache;
	int error = 0;
	FAudioWMADEC *wmadec = voice->src.wmadec;

	if (wmadec->blockCount != 0)
	{
		blockCount = wmadec->blockCount;
		maxBytes = wmadec->maxBytes;
	}
	else
	{
		blockCount = voice->src.bufferList->bufferWMA.PacketCount;
		maxBytes = voice->src.bufferList->bufferWMA.pDecodedPacketCumulativeBytes[blockCount - 1];
	}

	LOG_FUNC_ENTER(voice->audio)

	if (!wmadec->blockSizes)
	{
		size_t sz = blockCount * sizeof(*wmadec->blockSizes);
		wmadec->blockSizes = (size_t *) voice->audio->pMalloc(sz);
		memset(wmadec->blockSizes, 0xff, sz);
	}

	curBufferOffset = voice->src.curBufferOffset;
decode:
	byteOffset = SIZE_FROM_DST(curBufferOffset);

	/* the last block size can truncate the length of the buffer */
	availableBytes = SIZE_SRC_TO_DST(maxBytes);
	for (curBlock = 0; curBlock < blockCount; curBlock += 1)
	{
		/* decode to get real size */
		if (wmadec->blockSizes[curBlock] == (size_t)-1)
		{
			if (!FAudio_WMADEC_DecodeBlock(voice, buffer, curBlock, availableBytes))
			{
				error = 1;
				goto done;
			}
		}

		if (wmadec->blockSizes[curBlock] > byteOffset)
		{
			/* ensure curBlock is decoded in either slot */
			if (wmadec->curBlock != curBlock && wmadec->prevBlock != curBlock)
			{
				if (!FAudio_WMADEC_DecodeBlock(voice, buffer, curBlock, availableBytes))
				{
					error = 1;
					goto done;
				}
			}
			break;
		}

		byteOffset -= wmadec->blockSizes[curBlock];
		availableBytes -= wmadec->blockSizes[curBlock];
		if (availableBytes == 0)
			break;
	}

	if (curBlock >= blockCount || availableBytes == 0)
	{
		goto done;
	}

	/* If we're in a different block from last time, decode! */
	if (curBlock == wmadec->curBlock)
	{
		convertCache = wmadec->convertCache;
	}
	else if (curBlock == wmadec->prevBlock)
	{
		convertCache = wmadec->prevConvertCache;
	}
	else
	{
		convertCache = NULL;
		FAudio_assert(0 && "Somehow got an undecoded curBlock!");
	}

	/* Copy to decodeCache, finally. */
	siz = FAudio_min(SIZE_FROM_DST(samples), wmadec->blockSizes[curBlock] - byteOffset);
	if (convertCache)
	{
		FAudio_memcpy(
			decodeCache,
			convertCache + byteOffset,
			siz
		);
	}
	else
	{
		FAudio_memset(
			decodeCache,
			0,
			siz
		);
	}

	/* Increment pointer, decrement remaining sample count */
	decodeCache += siz / sizeof(float);
	samples -= SAMPLES_FROM_DST(siz);
	curBufferOffset += SAMPLES_FROM_DST(siz);

done:
	if (FAudio_GSTREAMER_CheckForBusErrors(voice))
	{
		LOG_ERROR(
			voice->audio,
			"%s",
			"Got a bus error after decoding!"
		)

		error = 1;
	}

	/* If the cache isn't filled yet, keep decoding! */
	if (samples > 0)
	{
		if (!error && curBlock < blockCount - 1)
		{
			goto decode;
		}

		/* out of stuff to decode, write blank and exit */
		FAudio_memset(decodeCache, 0, SIZE_FROM_DST(samples));
	}

	LOG_FUNC_EXIT(voice->audio)
}

void FAudio_WMADEC_end_buffer(FAudioSourceVoice *pSourceVoice)
{
	FAudioWMADEC *wmadec = pSourceVoice->src.wmadec;

	LOG_FUNC_ENTER(pSourceVoice->audio)

	pSourceVoice->audio->pFree(wmadec->blockSizes);
	wmadec->blockSizes = NULL;

	wmadec->curBlock = ~0u;
	wmadec->prevBlock = ~0u;

	LOG_FUNC_EXIT(pSourceVoice->audio)
}

static void FAudio_WMADEC_NewDecodebinPad(GstElement *decodebin,
		GstPad *pad, gpointer user)
{
	FAudioWMADEC *wmadec = user;
	GstPad *ac_sink;

	ac_sink = gst_element_get_static_pad(wmadec->resampler, "sink");
	if (GST_PAD_IS_LINKED(ac_sink))
	{
		gst_object_unref(ac_sink);
		return;
	}

	gst_pad_link(pad, ac_sink);

	gst_object_unref(ac_sink);
}

/* XMA2 doesn't have a recognized mimetype and so far only libav's avdec_xma2 exists for XMA2.
 * Things can change and as of writing this patch, things are actively changing.
 * As for this being a possible leak: using gstreamer is a static endeavour elsewhere already~
 * -ade
 */
/* #define FAUDIO_GST_LIBAV_EXPOSES_XMA2_CAPS_IN_CURRENT_YEAR */
#ifndef FAUDIO_GST_LIBAV_EXPOSES_XMA2_CAPS_IN_CURRENT_YEAR
static const char *FAudio_WMADEC_XMA2_Mimetype = NULL;
static GstElementFactory *FAudio_WMADEC_XMA2_DecoderFactory = NULL;
static GValueArray *FAudio_WMADEC_XMA2_DecodebinAutoplugFactories(
	GstElement *decodebin,
	GstPad *pad,
	GstCaps *caps,
	gpointer user
) {
	FAudio *audio = user;
	GValueArray *result;
	gchar *capsText;

	if (!gst_structure_has_name(gst_caps_get_structure(caps, 0), FAudio_WMADEC_XMA2_Mimetype))
	{
		capsText = gst_caps_to_string(caps);
		LOG_ERROR(
			audio,
			"Expected %s (XMA2) caps not %s",
			FAudio_WMADEC_XMA2_Mimetype,
			capsText
		)
		g_free(capsText);
		/*
		 * From the docs:
		 *
		 * If this function returns NULL, @pad will be exposed as a final caps.
		 *
		 * If this function returns an empty array, the pad will be considered as
		 * having an unhandled type media type.
		 */
		return g_value_array_new(0);
	}

	result = g_value_array_new(1);
	GValue val = G_VALUE_INIT;
	g_value_init(&val, G_TYPE_OBJECT);
	g_value_set_object(&val, FAudio_WMADEC_XMA2_DecoderFactory);
	g_value_array_append(result, &val);
	g_value_unset(&val);
	return result;
}
#endif


uint32_t FAudio_WMADEC_init(FAudioSourceVoice *pSourceVoice, uint32_t type)
{
	FAudioWMADEC *result;
	GstElement *decoder = NULL, *converter = NULL;
	const GList *tmpls;
	GstStaticPadTemplate *tmpl;
	GstCaps *caps;
	const char *mimetype;
	const char *versiontype;
	int version;
	GstBuffer *codec_data;
	size_t codec_data_size;
	uint8_t *extradata;
	uint8_t fakeextradata[16];
	GstPad *decoder_sink;
	GstEvent *event;

	LOG_FUNC_ENTER(pSourceVoice->audio)

	/* Init GStreamer statically. The docs tell us not to exit, so I guess
	 * we're supposed to just leak!
	 */
	if (!gst_is_initialized())
	{
		/* Apparently they ask for this to leak... */
		gst_init(NULL, NULL);
	}

#ifndef FAUDIO_GST_LIBAV_EXPOSES_XMA2_CAPS_IN_CURRENT_YEAR
	if (type == FAUDIO_FORMAT_XMAUDIO2 && !FAudio_WMADEC_XMA2_Mimetype)
	{
		/* Old versions ship with unknown/unknown as the sink caps mimetype.
		 * A patch has been submitted (and merged!) to expose avdec_xma2 as audio/x-xma with xmaversion 2:
		 * https://gitlab.freedesktop.org/gstreamer/gst-libav/-/merge_requests/124
		 * For now, try to find avdec_xma2 if it's found, match the mimetype on the fly.
		 * This should also take future alternative XMA2 decoders into account if avdec_xma2 is missing.
		 * -ade
		 */
		FAudio_WMADEC_XMA2_Mimetype = "audio/x-xma";
		FAudio_WMADEC_XMA2_DecoderFactory = gst_element_factory_find("avdec_xma2");
		if (FAudio_WMADEC_XMA2_DecoderFactory)
		{
			for (tmpls = gst_element_factory_get_static_pad_templates(FAudio_WMADEC_XMA2_DecoderFactory); tmpls; tmpls = tmpls->next)
			{
				tmpl = (GstStaticPadTemplate*) tmpls->data;
				if (tmpl->direction == GST_PAD_SINK)
				{
					caps = gst_static_pad_template_get_caps(tmpl);
					FAudio_WMADEC_XMA2_Mimetype = gst_structure_get_name(gst_caps_get_structure(caps, 0));
					gst_caps_unref(caps);
					break;
				}
			}
		}
	}
#endif

	/* Match the format with the codec */
	switch (type)
	{
	#define GSTTYPE(fmt, mt, vt, ver) \
		case FAUDIO_FORMAT_##fmt: mimetype = mt; versiontype = vt; version = ver; break;
	GSTTYPE(WMAUDIO2, "audio/x-wma", "wmaversion", 2)
	GSTTYPE(WMAUDIO3, "audio/x-wma", "wmaversion", 3)
	GSTTYPE(WMAUDIO_LOSSLESS, "audio/x-wma", "wmaversion", 4)
#ifndef FAUDIO_GST_LIBAV_EXPOSES_XMA2_CAPS_IN_CURRENT_YEAR
	GSTTYPE(XMAUDIO2, FAudio_WMADEC_XMA2_Mimetype, "xmaversion", 2)
#else
	GSTTYPE(XMAUDIO2, "audio/x-xma", "xmaversion", 2)
#endif
	#undef GSTTYPE
	default:
		LOG_ERROR(
			pSourceVoice->audio,
			"%X codec not supported!",
			type
		)
		LOG_FUNC_EXIT(pSourceVoice->audio)
		return FAUDIO_E_UNSUPPORTED_FORMAT;
	}

	/* Set up the GStreamer pipeline.
	 * Note that we're not assigning names, since many pipelines will exist
	 * at the same time (one per source voice).
	 */
	result = (FAudioWMADEC*) pSourceVoice->audio->pMalloc(sizeof(FAudioWMADEC));
	FAudio_zero(result, sizeof(FAudioWMADEC));

	result->pipeline = gst_pipeline_new(NULL);

	decoder = gst_element_factory_make("decodebin", NULL);
	if (!decoder)
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"Unable to create gstreamer decodebin; is %zu-bit gst-plugins-base installed?",
			sizeof(void *) * 8
		)
		goto free_without_bin;
	}

#ifndef FAUDIO_GST_LIBAV_EXPOSES_XMA2_CAPS_IN_CURRENT_YEAR
	if (type == FAUDIO_FORMAT_XMAUDIO2 && FAudio_WMADEC_XMA2_DecoderFactory)
	{
		g_signal_connect(decoder, "autoplug-factories", G_CALLBACK(FAudio_WMADEC_XMA2_DecodebinAutoplugFactories), pSourceVoice->audio);
	}
#endif
	g_signal_connect(decoder, "pad-added", G_CALLBACK(FAudio_WMADEC_NewDecodebinPad), result);

	result->srcpad = gst_pad_new(NULL, GST_PAD_SRC);

	result->resampler = gst_element_factory_make("audioresample", NULL);
	if (!result->resampler)
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"Unable to create gstreamer audioresample; is %zu-bit gst-plugins-base installed?",
			sizeof(void *) * 8
		)
		goto free_without_bin;
	}

	converter = gst_element_factory_make("audioconvert", NULL);
	if (!converter)
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"Unable to create gstreamer audioconvert; is %zu-bit gst-plugins-base installed?",
			sizeof(void *) * 8
		)
		goto free_without_bin;
	}

	result->dst = gst_element_factory_make("appsink", NULL);
	if (!result->dst)
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"Unable to create gstreamer appsink; is %zu-bit gst-plugins-base installed?",
			sizeof(void *) * 8
		)
		goto free_without_bin;
	}

	/* turn off sync so we can pull data without waiting for it to "play" in realtime */
	g_object_set(G_OBJECT(result->dst), "sync", FALSE, "async", TRUE, NULL);

	/* Compile the pipeline, finally. */
	if (!gst_pad_set_active(result->srcpad, TRUE))
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"%s",
			"Unable to activate srcpad"
		)
		goto free_without_bin;
	}

	gst_bin_add_many(
		GST_BIN(result->pipeline),
		decoder,
		result->resampler,
		converter,
		result->dst,
		NULL
	);

	decoder_sink = gst_element_get_static_pad(decoder, "sink");

	if (gst_pad_link(result->srcpad, decoder_sink) != GST_PAD_LINK_OK)
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"%s",
			"Unable to get link our src pad to decoder sink pad"
		)
		gst_object_unref(decoder_sink);
		goto free_with_bin;
	}

	gst_object_unref(decoder_sink);

	if (!gst_element_link_many(
		result->resampler,
		converter,
		result->dst,
		NULL))
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"%s",
			"Unable to get link pipeline"
		)
		goto free_with_bin;
	}

	/* send stream-start */
	event = gst_event_new_stream_start("faudio/gstreamer");
	gst_pad_push_event(result->srcpad, event);

	/* Prepare the input format */
	result->blockAlign = (uint32_t) pSourceVoice->src.format->nBlockAlign;
	if (type == FAUDIO_FORMAT_WMAUDIO3)
	{
		const FAudioWaveFormatExtensible *wfx =
			(FAudioWaveFormatExtensible*) pSourceVoice->src.format;
		extradata = (uint8_t*) &wfx->Samples;
		codec_data_size = pSourceVoice->src.format->cbSize;
		/* bufferList (and thus bufferWMA) can't be accessed yet. */
	}
	else if (type == FAUDIO_FORMAT_WMAUDIO2)
	{
		FAudio_zero(fakeextradata, sizeof(fakeextradata));
		fakeextradata[4] = 31;

		extradata = fakeextradata;
		codec_data_size = sizeof(fakeextradata);
		/* bufferList (and thus bufferWMA) can't be accessed yet. */
	}
	else if (type == FAUDIO_FORMAT_XMAUDIO2)
	{
		const FAudioXMA2WaveFormat *wfx =
			(FAudioXMA2WaveFormat*) pSourceVoice->src.format;
		extradata = (uint8_t*) &wfx->wNumStreams;
		codec_data_size = pSourceVoice->src.format->cbSize;
		/* XMA2 block size >= 16-bit limit and it doesn't come with bufferWMA. */
		result->blockAlign = wfx->dwBytesPerBlock;
		result->blockCount = wfx->wBlockCount;
		result->maxBytes = (
			(size_t) wfx->dwSamplesEncoded *
			pSourceVoice->src.format->nChannels *
			(pSourceVoice->src.format->wBitsPerSample / 8)
		);
	}
	else
	{
		extradata = NULL;
		codec_data_size = 0;
		FAudio_assert(0 && "Unrecognized WMA format!");
	}
	codec_data = gst_buffer_new_and_alloc(codec_data_size);
	gst_buffer_fill(codec_data, 0, extradata, codec_data_size);
	caps = gst_caps_new_simple(
		mimetype,
		versiontype,	G_TYPE_INT, version,
		"bitrate",	G_TYPE_INT, pSourceVoice->src.format->nAvgBytesPerSec * 8,
		"channels",	G_TYPE_INT, pSourceVoice->src.format->nChannels,
		"rate",		G_TYPE_INT, pSourceVoice->src.format->nSamplesPerSec,
		"block_align",	G_TYPE_INT, result->blockAlign,
		"depth",	G_TYPE_INT, pSourceVoice->src.format->wBitsPerSample,
		"codec_data",	GST_TYPE_BUFFER, codec_data,
		NULL
	);
	event = gst_event_new_caps(caps);
	gst_pad_push_event(result->srcpad, event);
	gst_caps_unref(caps);
	gst_buffer_unref(codec_data);

	/* Prepare the output format */
	caps = gst_caps_new_simple(
		"audio/x-raw",
		"format",	G_TYPE_STRING, gst_audio_format_to_string(GST_AUDIO_FORMAT_F32),
		"layout",	G_TYPE_STRING, "interleaved",
		"channels",	G_TYPE_INT, pSourceVoice->src.format->nChannels,
		"rate",		G_TYPE_INT, pSourceVoice->src.format->nSamplesPerSec,
		NULL
	);

	gst_app_sink_set_caps(GST_APP_SINK(result->dst), caps);
	gst_caps_unref(caps);

	gst_segment_init(&result->segment, GST_FORMAT_TIME);

	if (!FAudio_GSTREAMER_RestartDecoder(result))
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"%s",
			"Starting decoder failed!"
		)
		goto free_with_bin;
	}

	pSourceVoice->src.wmadec = result;
	pSourceVoice->src.decode = FAudio_INTERNAL_DecodeGSTREAMER;

	if (FAudio_GSTREAMER_CheckForBusErrors(pSourceVoice))
	{
		LOG_ERROR(
			pSourceVoice->audio,
			"%s",
			"Got a bus error after creation!"
		)

		pSourceVoice->src.wmadec = NULL;
		pSourceVoice->src.decode = NULL;

		goto free_with_bin;
	}

	LOG_FUNC_EXIT(pSourceVoice->audio)
	return 0;

free_without_bin:
	if (result->dst)
	{
		gst_object_unref(result->dst);
	}
	if (converter)
	{
		gst_object_unref(converter);
	}
	if (result->resampler)
	{
		gst_object_unref(result->resampler);
	}
	if (result->srcpad)
	{
		gst_object_unref(result->srcpad);
	}
	if (decoder)
	{
		gst_object_unref(decoder);
	}
	if (result->pipeline)
	{
		gst_object_unref(result->pipeline);
	}
	pSourceVoice->audio->pFree(result);
	LOG_FUNC_EXIT(pSourceVoice->audio)
	return FAUDIO_E_UNSUPPORTED_FORMAT;

free_with_bin:
	gst_object_unref(result->srcpad);
	gst_object_unref(result->pipeline);
	pSourceVoice->audio->pFree(result);
	LOG_FUNC_EXIT(pSourceVoice->audio)
	return FAUDIO_E_UNSUPPORTED_FORMAT;
}

void FAudio_WMADEC_free(FAudioSourceVoice *voice)
{
	LOG_FUNC_ENTER(voice->audio)
	gst_element_set_state(voice->src.wmadec->pipeline, GST_STATE_NULL);
	gst_object_unref(voice->src.wmadec->pipeline);
	gst_object_unref(voice->src.wmadec->srcpad);
	voice->audio->pFree(voice->src.wmadec->convertCache);
	voice->audio->pFree(voice->src.wmadec->prevConvertCache);
	voice->audio->pFree(voice->src.wmadec->blockSizes);
	voice->audio->pFree(voice->src.wmadec);
	voice->src.wmadec = NULL;
	LOG_FUNC_EXIT(voice->audio)
}

#else

extern int this_tu_is_empty;

#endif /* HAVE_GSTREAMER */

/* vim: set noexpandtab shiftwidth=8 tabstop=8: */
