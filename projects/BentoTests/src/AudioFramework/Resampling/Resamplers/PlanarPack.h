#pragma once
#include "IResamplerMethod.h"

/**
 * The planar pack method is used when the input is planar but the output is
 * an interleaved buffer of the same type, channel count, etc...
 */
class ResamplePlanarPack final : public IResamplerMethod {
public:
	ResamplePlanarPack() = default;
	virtual ~ResamplePlanarPack() = default;

	virtual uint32_t Resample(const uint8_t** inBuffers, size_t inSamples, uint8_t** outBufffers, size_t outSamples) const override;
};