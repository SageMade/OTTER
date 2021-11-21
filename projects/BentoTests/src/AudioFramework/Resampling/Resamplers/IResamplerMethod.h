#pragma once
#include <cstdint>
#include "AudioFramework/SampleFormat.h"

class IResamplerMethod {
public:
	virtual ~IResamplerMethod() = default;

	uint32_t InputSampleRate;
	uint32_t OutputSampleRate;
	uint8_t  NumInputChannels;
	uint8_t  NumOutputChannels;
	SampleFormat InputFormat;
	SampleFormat OutputFormat;

	virtual uint32_t Resample(const uint8_t** inBuffers, size_t inSamples, uint8_t** outBufffers, size_t outSamples) const = 0;

protected:
	IResamplerMethod() = default;
};