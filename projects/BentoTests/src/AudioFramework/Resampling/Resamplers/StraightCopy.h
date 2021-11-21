#pragma once
#include "AudioFramework/Resampling/Resamplers/IResamplerMethod.h"

/**
 * The straight copy method is used when input and output formats match exactly
 */
class ResampleStraightCopy final : public IResamplerMethod {
public:
	ResampleStraightCopy() = default;
	virtual ~ResampleStraightCopy() = default;

	virtual uint32_t Resample(const uint8_t** inBuffers, size_t inSamples, uint8_t** outBufffers, size_t outSamples) const override;
};