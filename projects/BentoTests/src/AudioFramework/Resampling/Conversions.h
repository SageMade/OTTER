#pragma once
#include <cstdint>

namespace ResampleConversions {
	const float PCM_TO_FLOAT = (float)0x8000; // int16 max
	const float FLOAT_TO_PCM = (float)0x7FFF; // int16 max - 1

	/**
	 * Converts from signed PCM to floats in the [-1,1] range
	 */
	inline float FromSignedPCM(int16_t value) {
		return value / PCM_TO_FLOAT;
	}
	/**
	 * Coverts a floating point value in the [-1,1] range to a signed
	 * PCM value. Note that this DOES NOT validate that the value
	 */
	inline int16_t ToSignedPCM(float value) {
		return (int16_t)(value * FLOAT_TO_PCM);
	}


};