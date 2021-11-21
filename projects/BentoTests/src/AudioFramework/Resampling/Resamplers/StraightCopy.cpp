#include "AudioFramework/Resampling/Resamplers/StraightCopy.h"
#include <Logging.h>

uint32_t ResampleStraightCopy::Resample(const uint8_t ** inBuffers, size_t inSamples, uint8_t ** outBufffers, size_t outSamples) const {
	LOG_ASSERT(InputFormat == OutputFormat, "Input and output format do not match, select another sampling strategy");
	LOG_ASSERT(NumInputChannels == NumOutputChannels, "Input and output channel count does not match, select another sampling strategy");
	LOG_ASSERT(InputSampleRate == OutputSampleRate, "Input and output sample rate does not match, select another sampling strategy");

	if (IsFormatPlanar(InputFormat)) {
		for (int ix = 0; ix < NumInputChannels; ix++) {
			memcpy(outBufffers[ix], inBuffers[ix], inSamples * GetSampleFormatSize(InputFormat));
		}
	} else {
		memcpy(outBufffers[0], inBuffers[0], inSamples * GetSampleFormatSize(InputFormat) * NumInputChannels);
	}
	return inSamples;
}