#include <AudioFramework/Resampler.h>
#include <Logging.h>

extern "C" {
	#include <libswresample/swresample.h>
	#include <libavutil/opt.h>
	#include <libavutil/log.h>
	#include <libavutil/error.h>
	#include <libavutil/channel_layout.h>
}
#include <AudioFramework/AudioEncoders/IAudioEncoder.h>

Resampler::Resampler() :
	_ffmegResample(nullptr),
	_inConfig(StreamConfig()),
	_outConfig(StreamConfig())
{
	// Set our input and output configs to something reasonable
	_inConfig.Format = SampleFormat::Float;
	_inConfig.NumChannels = 2;
	_inConfig.SampleRate = 44100;
	memset(&_inConfig.ChannelBuffs, 0, sizeof(void*) * 8);

	_outConfig.Format = SampleFormat::PlanarFloat;
	_outConfig.NumChannels = 2;
	_outConfig.SampleRate = 44100;
	memset(&_outConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

Resampler::~Resampler() {
	if (_ffmegResample != nullptr) {
		swr_free(&_ffmegResample);
	}
}


void Resampler::SetOutputFrameSampleCount(uint32_t sampleCount) {
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_outConfig.FrameSampleCount = sampleCount;
}


void Resampler::SetOutputChannelPtr(uint8_t channel, uint8_t* backingField) {
	LOG_ASSERT(channel >= 0 && channel < _outConfig.NumChannels && channel < 8, "Channel outside the range for this resampler!");
	_outConfig.ChannelBuffs[channel] = backingField;
}

void Resampler::SetInputChannelPtr(uint8_t channel, uint8_t* backingField) {
	LOG_ASSERT(channel >= 0 && channel < _outConfig.NumChannels&& channel < 8, "Channel outside the range for this resampler!");
	_inConfig.ChannelBuffs[channel] = backingField;
}

void Resampler::SetInputFrameSampleCount(uint32_t frameCount) {
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig.FrameSampleCount = frameCount;
}

void Resampler::MatchSourceEncoding(const AudioInStreamConfig& inputStream)
{
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig.Format = inputStream.Format;
	_inConfig.SampleRate = inputStream.SampleRate;
	_inConfig.NumChannels = inputStream.NumChannels;
	memset(&_inConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

void Resampler::MatchDestEncoding(const IAudioEncoder* encoder)
{
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_outConfig.Format = encoder->GetInputFormat();
	_outConfig.SampleRate = encoder->GetSampleRate();
	_outConfig.NumChannels = encoder->GetNumChannels();
	_outConfig.FrameSampleCount = encoder->GetSamplesPerInputFrame();
	memset(&_outConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

void Resampler::SetInputConfig(const StreamConfig& config)
{
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig = config;
	memset(&_inConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

void Resampler::SetOutputConfig(const StreamConfig& config)
{
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");
	_outConfig = config;
	memset(&_outConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

void Resampler::Init()
{
	LOG_ASSERT(_ffmegResample == nullptr, "This resampler has already been initialized! Check your logic!");

	_ffmegResample = swr_alloc();
	av_opt_set_int(_ffmegResample, "in_channel_layout", av_get_default_channel_layout(_inConfig.NumChannels), 0);
	av_opt_set_int(_ffmegResample, "in_sample_rate", _inConfig.SampleRate, 0);
	av_opt_set_sample_fmt(_ffmegResample, "in_sample_fmt", (AVSampleFormat)ToFfmpeg(_inConfig.Format), 0);

	av_opt_set_int(_ffmegResample, "out_channel_layout", av_get_default_channel_layout(_outConfig.NumChannels), 0);
	av_opt_set_int(_ffmegResample, "out_sample_rate", _outConfig.SampleRate, 0);
	av_opt_set_sample_fmt(_ffmegResample, "out_sample_fmt", (AVSampleFormat)ToFfmpeg(_outConfig.Format), 0);
	swr_init(_ffmegResample);
}

void Resampler::EncodeFrame(uint32_t sampleCount) {
	LOG_ASSERT(_ffmegResample != nullptr, "This resampler has not been initialized!");
	LOG_ASSERT(sampleCount <= _inConfig.FrameSampleCount, "Buffer overflow!");
	if (sampleCount == 0) {
		sampleCount = _inConfig.FrameSampleCount;  
	}
	
	int outSamples = swr_convert(_ffmegResample, (uint8_t**)_outConfig.ChannelBuffs, _outConfig.FrameSampleCount, (const uint8_t**)_inConfig.ChannelBuffs, sampleCount);
}
