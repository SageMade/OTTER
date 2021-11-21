#include "AudioFramework/Resampling/Resampler.h"
#include <Logging.h>

#include "AudioFramework/AudioEncoders/IAudioEncoder.h"
#include "AudioFramework/IAudioCapDevice.h"

#include "AudioFramework/Resampling/Resamplers/IResamplerMethod.h"
#include "AudioFramework/Resampling/Resamplers/StraightCopy.h"

struct Resampler::ResamplerContext {
	IResamplerMethod* _method;
};

Resampler::Resampler() :
	_inConfig(StreamConfig()),
	_outConfig(StreamConfig()),
	_context(nullptr)
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
	if (_context != nullptr) {
		delete _context;
	}
}


void Resampler::SetOutputFrameSampleCount(uint32_t sampleCount) {
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
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
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig.FrameSampleCount = frameCount;
}

void Resampler::MatchSourceEncoding(const AudioInStreamConfig& config)
{
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig.Format = config.Format;
	_inConfig.SampleRate = config.SampleRate;
	_inConfig.NumChannels = config.NumChannels;
	memset(&_inConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

void Resampler::MatchDestEncoding(const IAudioEncoder* encoder)
{
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
	_outConfig.Format = encoder->GetInputFormat();
	_outConfig.SampleRate = encoder->GetSampleRate();
	_outConfig.NumChannels = encoder->GetNumChannels();
	_outConfig.FrameSampleCount = encoder->GetSamplesPerInputFrame();

	// Remove any existing buffer handles
	memset(&_outConfig.ChannelBuffs, 0, sizeof(void*) * 8);
	// Attach our encoder input buffers as the target for this resampler
	if (IsFormatPlanar(_outConfig.Format)) {
		for (int ix = 0; ix < _outConfig.NumChannels; ix++) {
			_outConfig.ChannelBuffs[ix] = encoder->GetInputBuffer(ix);
		}
	} else {
		_outConfig.ChannelBuffs[0] = encoder->GetInputBuffer(0);
	}
}

void Resampler::SetInputConfig(const StreamConfig& config)
{
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
	_inConfig = config;
	memset(&_inConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

const Resampler::Resampler::StreamConfig& Resampler::GetInputConfig() const
{
	return _inConfig;
}

void Resampler::SetOutputConfig(const StreamConfig& config) {
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");
	_outConfig = config;
	memset(&_outConfig.ChannelBuffs, 0, sizeof(void*) * 8);
}

const Resampler::Resampler::StreamConfig& Resampler::GetOutputConfig() const {
	return _outConfig;
}

void Resampler::Init()
{
	LOG_ASSERT(_context == nullptr, "This resampler has already been initialized! Check your logic!");

	if (_inConfig.SampleRate != _outConfig.SampleRate) {
		throw std::runtime_error("Cannot initialize audio resampler, sample rate conversion has not been implemented");
	}

	_context = new Resampler::ResamplerContext();

	if (_inConfig.Format == _outConfig.Format && 
		_inConfig.NumChannels == _outConfig.NumChannels && 
		_inConfig.SampleRate == _outConfig.SampleRate) {
		_context->_method = new ResampleStraightCopy();
	}

	if (_context->_method != nullptr) {
		_context->_method->InputFormat = _inConfig.Format;
		_context->_method->NumInputChannels = _inConfig.NumChannels;
		_context->_method->InputSampleRate = _inConfig.SampleRate;

		_context->_method->OutputFormat = _outConfig.Format;
		_context->_method->NumOutputChannels = _outConfig.NumChannels;
		_context->_method->OutputSampleRate = _outConfig.SampleRate;
	} else {
		delete _context;
		throw std::runtime_error("Failed to initialize audio resampler, could not select a supported method");
	}
}

uint32_t Resampler::EncodeFrame(uint32_t sampleCount) {
	LOG_ASSERT(_context != nullptr, "This resampler has not been initialized!");
	LOG_ASSERT(sampleCount >= _inConfig.FrameSampleCount, "Buffer overflow, cannot encode more samples than are stored in output");
	if (sampleCount == 0) {
		sampleCount = _inConfig.FrameSampleCount;  
	}
	
	return _context->_method->Resample((const uint8_t**)_inConfig.ChannelBuffs, sampleCount, (uint8_t**)_outConfig.ChannelBuffs, _outConfig.FrameSampleCount);
}
