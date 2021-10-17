#include <Bento4C.h>
#include <Ap4.h>
#include <string>
#include <iostream>
#include <math.h>
#include <unordered_map>
#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
}
#include "CustomAtom.h"
#include <BinaryWriter.h>
#include <BinaryFileWriter.h>
#include <Windows/WaveIn.h>

#define VALIDATE_ALLOC(param, message, returnCode) if (param == nullptr) { std::cout << message << std::endl; return returnCode; }
#define VALIDATE_FFMPEG(ret, message, returnCode) if (ret < 0) { std::cout << message << std::endl; return returnCode; }

std::unordered_map<AVSampleFormat, std::string> FormatMap {
	{AV_SAMPLE_FMT_NONE, "AV_SAMPLE_FMT_NONE"},
	{AV_SAMPLE_FMT_U8,	 "AV_SAMPLE_FMT_U8"},
	{AV_SAMPLE_FMT_S16,	 "AV_SAMPLE_FMT_S16"},
	{AV_SAMPLE_FMT_S32,	 "AV_SAMPLE_FMT_S32"},
	{AV_SAMPLE_FMT_FLT,	 "AV_SAMPLE_FMT_FLT"},
	{AV_SAMPLE_FMT_DBL,	 "AV_SAMPLE_FMT_DBL"},

	{AV_SAMPLE_FMT_U8P,	 "AV_SAMPLE_FMT_U8P"},
	{AV_SAMPLE_FMT_S16P, "AV_SAMPLE_FMT_S16P"},
	{AV_SAMPLE_FMT_S32P, "AV_SAMPLE_FMT_S32P"},
	{AV_SAMPLE_FMT_FLTP, "AV_SAMPLE_FMT_FLTP"},
	{AV_SAMPLE_FMT_DBLP, "AV_SAMPLE_FMT_DBLP"},
	{AV_SAMPLE_FMT_S64,  "AV_SAMPLE_FMT_S64"},
	{AV_SAMPLE_FMT_S64P, "AV_SAMPLE_FMT_S64P"},

	{AV_SAMPLE_FMT_NB,   "AV_SAMPLE_FMT_NB"}
};

void Save(const std::string& path) {
	// The sample table for the output stream

}

static int select_sample_rate(const AVCodec* codec, int targetSampleRate = 44100)
{
	const int* p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return targetSampleRate;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(targetSampleRate - *p) < abs(targetSampleRate - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

static int encode(AVCodecContext* ctx, AVFrame* frame, AVPacket* packet, IBinaryStream& stream) {
	int ret;
	ret = avcodec_send_frame(ctx, frame);
	VALIDATE_FFMPEG(ret, "Failed to encode frame", -1);
	
	while (ret >= 0) {
		// Read a packet of data from the encoder
		ret = avcodec_receive_packet(ctx, packet);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return 0;
		} else if (ret < 0) {
			return -1;
		}
		stream.WriteBytes(packet->data, packet->size);
		stream.Flush();

		// Decrement reference count now that we're done with the packet
		av_packet_unref(packet); 
	}
}

int main() {
	int ret; // return codes for avcodec commands
	AVCodec* codec;
	AVCodecContext* ctx;
	AVPacket* packet;
	AVFrame* frame;
	uint16_t* samples;
	{
		BinaryFileWriter fWriter("test.flac");
	
		// Find the AAC encoder
		codec = avcodec_find_encoder(AV_CODEC_ID_FLAC);
		VALIDATE_ALLOC(codec, "Failed to find codec", 1);

		// Create the context for our codec
		ctx = avcodec_alloc_context3(codec);
		VALIDATE_ALLOC(ctx, "Failed to allocate context", 2);
		ctx->bit_rate = 64000; // 64kbps
		ctx->channels = 1; // Mono track
		ctx->sample_rate = select_sample_rate(codec); // Target 44.1k/hz by default
		ctx->sample_fmt = AV_SAMPLE_FMT_S16; // Signed 16 bits (PCM)
		ctx->channel_layout = AV_CH_LAYOUT_MONO; // Make sure our channel layout is identified as mono

		// Open the codec for conversion;
		ret = avcodec_open2(ctx, codec, nullptr);
		VALIDATE_FFMPEG(ret, "Failed to open the codec!", 3);



		// Allocate an AV packet to store results
		packet = av_packet_alloc();
		VALIDATE_ALLOC(packet, "Failed to allocate AV packet", 4);

		// Allocate an AV frame
		frame = av_frame_alloc();
		VALIDATE_ALLOC(packet, "Failed to allocate AV frame", 5);
		frame->nb_samples = ctx->frame_size;
		frame->format = ctx->sample_fmt;
		frame->channel_layout = ctx->channel_layout;

		// Allocate the buffers within the AV frame
		ret = av_frame_get_buffer(frame, 0);
		VALIDATE_FFMPEG(ret, "Failed to allocate AV buffers", 6);

		size_t buffOffset = 0;
		size_t frameSize = (ctx->bits_per_raw_sample / 8) * frame->nb_samples;
		TestWaveAudio(ctx->sample_rate, [&](uint8_t* data, size_t len) {
			// We use remaining and an offset so we don't have to spend the time 
			size_t remaining = len;
			size_t offset = 0;

			// If we have a partial frame, we should try to complete it
			if (buffOffset > 0) {
				// Determine the number of bytes to copy, then copy into partial frame
				size_t count = glm::min(frameSize - buffOffset, len);
				memcpy(frame->data[0] + buffOffset, data, count);
				// If the frame is now complete, encode it into our FFMPEG stream
				if (buffOffset + count == frameSize) {
					encode(ctx, frame, packet, fWriter);
					buffOffset = 0;
				}
				// If the frame is still incomplete, increase the offset by the number of bytes we've added
				if (buffOffset + count < frameSize) {
					buffOffset += count;
				}
				// Modify our counters based on number of bytes read from input
				remaining -= count;
				offset = count;
			}

			// Read full frames from input to FFMPEG
			while (remaining >= frameSize) {
				memcpy(frame->data[0], data + offset, frameSize);
				encode(ctx, frame, packet, fWriter);
				remaining -= frameSize;
				offset += frameSize;
			}
			// Read any remaining data into a partial frame
			if (remaining > 0) {
				memcpy(frame->data[0], data + offset, remaining);
				buffOffset = remaining;
			}
		});

		// Fill rest of frame with silence and encode full frame at end of the stream
		memset(frame->data[0] + buffOffset, 0, frameSize - buffOffset);
		encode(ctx, frame, packet, fWriter);

		// Flush encoder
		encode(ctx, NULL, packet, fWriter);
	}

	// Cleanup
	av_frame_free(&frame);
	av_packet_free(&packet);
	avcodec_free_context(&ctx);


	return 0; 
} 
