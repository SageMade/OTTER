#include "AudioEncoder.h"
#include <Logging.h>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libswresample/swresample.h>
	#include <libavutil/opt.h>
	#include <libavutil/log.h>
	#include <libavutil/error.h>
	#include <libavutil/channel_layout.h>
}

#define VALIDATE_ALLOC(param, message, returnCode) if (param == nullptr) { std::cout << message << std::endl; return returnCode; }
#define VALIDATE_FFMPEG(ret, message, returnCode) if (ret < 0) { std::cout << message << std::endl; return returnCode; }

struct AudioEncoderContext {
	AVCodec* Codec;
	AVCodecContext* Ctx;
	AVFrame* InputFrame;
	AVPacket* SyncEncodePacket;

	AudioEncoderContext() :
		Codec(nullptr),
		Ctx(nullptr),
		InputFrame(nullptr),
		SyncEncodePacket(nullptr) { }
};

inline const AVCodecID GetCodecID(EncodingFormat format) {
	switch (format)
	{
		case EncodingFormat::AAC: return AV_CODEC_ID_AAC;
		case EncodingFormat::FLAC: return AV_CODEC_ID_FLAC;
		default: return AV_CODEC_ID_NONE;
	}
}

AudioEncoder::AudioEncoder() :
	_encoderContext(nullptr)
{
	Config.Format = EncodingFormat::AAC;
}

AudioEncoder::~AudioEncoder()
{
	if (_encoderContext != nullptr) {
		if (_encoderContext->Codec) {
			av_frame_free(&_encoderContext->InputFrame);
			avcodec_free_context(&_encoderContext->Ctx);
			av_free_packet(_encoderContext->SyncEncodePacket);
		}
		delete _encoderContext;
		delete _syncResult;
	}
}

void AudioEncoder::SetEncodingFormat(EncodingFormat format) {
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");
	Config.Format = format;
}

void AudioEncoder::SetBitRate(uint32_t bitrate) {
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");
	Config.BitRate = bitrate;
}

void AudioEncoder::SetTargetSampleRate(uint32_t sampleRate) {
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");
	Config.SampleRate = sampleRate;
}

void AudioEncoder::SetTargetChannels(uint8_t numChannels) {
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");
	Config.NumChannels = numChannels;
}

void AudioEncoder::SetTargetSampleFormat(SampleFormat format)
{
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");
	Config.SampleInFormat = format;
}

void AudioEncoder::Init() {
	LOG_ASSERT(_encoderContext == nullptr, "This encoder has already been initialized!");

	_encoderContext = new AudioEncoderContext();
	_syncResult = new EncoderResult();

	// Find the AAC encoder
	_encoderContext->Codec = avcodec_find_encoder(GetCodecID(Config.Format));
	LOG_ASSERT(_encoderContext->Codec, "Failed to find codec");

	_ValidateSampleFormat();

	// Create the context for our codec
	_encoderContext->Ctx = avcodec_alloc_context3(_encoderContext->Codec);
	LOG_ASSERT(_encoderContext->Ctx, "Failed to allocate decoder context");

	_encoderContext->Ctx->sample_rate = _SelectOptimalSampleRate(Config.SampleRate); // Target 44.1k/hz by default
	_encoderContext->Ctx->bit_rate = Config.BitRate; // 64kbps
	_encoderContext->Ctx->channels = Config.NumChannels; // Mono track
	_encoderContext->Ctx->sample_fmt = ToFfmpeg(Config.SampleInFormat); // planar floats |  AV_SAMPLE_FMT_S16; // Signed 16 bits (PCM)
	_encoderContext->Ctx->channel_layout = av_get_default_channel_layout(Config.NumChannels); // Make sure our channel layout is identified as mono

	// Open the codec for conversion;
	int ret = avcodec_open2(_encoderContext->Ctx, _encoderContext->Codec, nullptr);
	LOG_ASSERT(ret >= 0, "Failed to initialize FFMPEG codec");

	// Create the frame
	_encoderContext->InputFrame = av_frame_alloc();
	LOG_ASSERT(_encoderContext->InputFrame, "Failed to allocate AV frame");
	_encoderContext->InputFrame->nb_samples = _encoderContext->Ctx->frame_size;
	_encoderContext->InputFrame->format = _encoderContext->Ctx->sample_fmt;
	_encoderContext->InputFrame->channel_layout = _encoderContext->Ctx->channel_layout;

	// Allocate the buffers within the AV frame
	ret = av_frame_get_buffer(_encoderContext->InputFrame, 0);
	LOG_ASSERT(ret >= 0, "Failed to allocate AV frame buffers for encoding!");

	// Allote a packet to be used when decoding frames in syncronous mode
	_encoderContext->SyncEncodePacket = av_packet_alloc();
	LOG_ASSERT(_encoderContext->InputFrame, "Failed to allocate output packet!");

	// Allocate the Buffer fillers, using a single buffer if interleaved and multi buffer if planar
	/*int numBuffs = av_sample_fmt_is_planar(_encoderContext->Ctx->sample_fmt) ? _encoderContext->Ctx->channels : 1;
	uint8_t** buffers = new uint8_t*[numBuffs];
	for (int ix = 0; numBuffs; ix++) {
		buffers[ix] = _encoderContext->InputFrame->data[ix];
	}
	size_t frameByteSize = _encoderContext->InputFrame->nb_samples * av_get_bytes_per_sample(_encoderContext->Ctx->sample_fmt) * (av_sample_fmt_is_planar(_encoderContext->Ctx->sample_fmt) ? _encoderContext->Ctx->channels : 1);
	_inputBuffer = new BufferFiller(buffers, numBuffs, frameByteSize);*/
}

EncoderResult* AudioEncoder::CreatePacket() const {
	return new EncoderResult();
}

void AudioEncoder::FreePacket(EncoderResult** packet) { 
	if (packet != nullptr && *packet != nullptr) {
		delete packet;
		packet = nullptr;
	}
}

size_t AudioEncoder::FrameSampleCount() const {
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, format has not been determined!");
	return _encoderContext->InputFrame->nb_samples;
}

uint8_t AudioEncoder::GetNumChannels() const {
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, format has not been determined!");
	return _encoderContext->Ctx->channels;
}

uint32_t AudioEncoder::GetSampleRate() const {
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, format has not been determined!");
	return _encoderContext->Ctx->sample_rate;
}

uint32_t AudioEncoder::GetBitRate() const
{
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, format has not been determined!");
	return _encoderContext->Ctx->bit_rate;
}

size_t AudioEncoder::GetFrameBufferSize() const {
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, buffers not yet allocated!");
	return _encoderContext->InputFrame->nb_samples* av_get_bytes_per_sample(_encoderContext->Ctx->sample_fmt)* (av_sample_fmt_is_planar(_encoderContext->Ctx->sample_fmt) ? 1 : (uint32_t)_encoderContext->Ctx->channels);
}

size_t AudioEncoder::CalcFrameBufferSize(SampleFormat format) const
{
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, buffers not yet allocated!");
	return _encoderContext->InputFrame->nb_samples * av_get_bytes_per_sample(ToFfmpeg(format)) * (av_sample_fmt_is_planar(ToFfmpeg(format)) ? 1 : (uint32_t)_encoderContext->Ctx->channels);
}

uint8_t* AudioEncoder::GetInputBufferPtr(uint8_t channel) const
{
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, buffers not yet allocated!");
	return _encoderContext->InputFrame->data[channel];
}

//void AudioEncoder::FeedDataSync(uint8_t** data, size_t len, PacketEncodedCallback callback) {
//	_inputBuffer->FeedData(data, len, [&](uint8_t** data, size_t bufferSize) {
//		_EncodeFrame(_encoderContext->SyncEncodePacket, callback);
//	});
//}

SampleFormat AudioEncoder::GetActualSampleFormat() const {
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized, format has not been determined!");
	return Config.SampleInFormat;
}

void AudioEncoder::EncodeFrame(PacketEncodedCallback callback, EncoderResult* packet)
{
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized!");

	packet = packet == nullptr ? _syncResult : packet;
	int ret = 0;
	// Send the data frame to the encoder
	ret = avcodec_send_frame(_encoderContext->Ctx, _encoderContext->InputFrame);
	LOG_ASSERT(ret >= 0, "Failed to encode frame!");

	// TODO: Split this into another thread? 
	while (ret >= 0) {
		// Read a packet of data from the encoder
		ret = avcodec_receive_packet(_encoderContext->Ctx, _encoderContext->SyncEncodePacket);

		// Early abort if EOF
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		} else if (ret < 0) {
			char buffer[256];
			av_make_error_string(buffer, 256, ret);
			LOG_WARN("Failed when encoding packet: {}", (const char*)buffer);
		}

		packet->Data = _encoderContext->SyncEncodePacket->data;
		packet->DataSize = _encoderContext->SyncEncodePacket->size;
		packet->Duration = _encoderContext->SyncEncodePacket->duration;
		callback(packet);

		// Decrement reference count now that we're done with the packet
		av_packet_unref(_encoderContext->SyncEncodePacket);
	}
}

void AudioEncoder::Flush(PacketEncodedCallback frameCallback, EncoderResult* packet)
{
	LOG_ASSERT(_encoderContext != nullptr, "Not initialized!");
	packet = packet == nullptr ? _syncResult : packet;
	int ret = 0;
	// Flush the encoder
	ret = avcodec_send_frame(_encoderContext->Ctx, nullptr);
	LOG_ASSERT(ret >= 0, "Failed to encode frame!");

	// TODO: Split this into another thread? 
	while (ret >= 0) {
		// Read a packet of data from the encoder
		ret = avcodec_receive_packet(_encoderContext->Ctx, _encoderContext->SyncEncodePacket);

		// Early abort if EOF
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		} else if (ret < 0) {
			char buffer[256];
			av_make_error_string(buffer, 256, ret);
			LOG_WARN("Failed when encoding packet: {}", (const char*)buffer);
		}

		packet->Data     = _encoderContext->SyncEncodePacket->data;
		packet->DataSize = _encoderContext->SyncEncodePacket->size;
		packet->Duration = _encoderContext->SyncEncodePacket->duration;
		frameCallback(packet);

		// Decrement reference count now that we're done with the packet
		av_packet_unref(_encoderContext->SyncEncodePacket);
	}
}

int AudioEncoder::_SelectOptimalSampleRate(int target /*= 44100*/) {
	const int* p;
	int best_samplerate = 0;

	if (!_encoderContext->Codec->supported_samplerates)
		return target;

	p = _encoderContext->Codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(target - *p) < abs(target - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

void AudioEncoder::_ValidateSampleFormat() {
	const AVSampleFormat* p;
	if (!_encoderContext->Codec->sample_fmts)
		return;
	
	if (Config.SampleInFormat != SampleFormat::Unknown) {
		p = _encoderContext->Codec->sample_fmts;
		while (*p) {
			if (*p == ToFfmpeg(Config.SampleInFormat));
			return;
			p++;
		}
		LOG_WARN("Targeted a sample format, but the codec does not support it! Falling back to codec support");
	}
	Config.SampleInFormat = FromFfmpeg(_encoderContext->Codec->sample_fmts[0]);
}

