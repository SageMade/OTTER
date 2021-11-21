#include "PlanarPack.h"
#include "Logging.h"

uint32_t ResamplePlanarPack::Resample(const uint8_t** inBuffers, size_t inSamples, uint8_t** outBufffers, size_t outSamples) const {
	LOG_ASSERT(NumInputChannels == NumOutputChannels, "Input and output channel count does not match, select another sampling strategy");
	LOG_ASSERT(InputSampleRate == OutputSampleRate, "Input and output sample rate does not match, select another sampling strategy");

	// TODO: This is dumb and slow, can we optimize it?

	uint8_t* resultBuffer = outBufffers[0];
	uint32_t ix = 0;
	for (; ix < outSamples && ix < inSamples; ix++) {
		size_t offset = ix * GetSampleFormatSize(InputFormat);
		for (int ic = 0; ic < NumInputChannels; ic++) {
			memcpy(resultBuffer, inBuffers + offset, GetSampleFormatSize(InputFormat));
			resultBuffer += GetSampleFormatSize(InputFormat);
		}
	}

	return ix;
}
