#pragma once
#include <cstdint>
#include <string>
extern "C" {
	#include "libavutil/samplefmt.h"
}

/**
 * Represents an audio sample format for a stream of audio data
 */
enum class SampleFormat {
	Unknown     = 0,
	Float       = 0b00000001, // Interleaved floats
	PCM         = 0b00000010, // int16 PCM, interleaved
	PlanarFloat = 0b10000001, // Float, but each float is on it's own data plane 
	PlanarPCM   = 0b10000010, // int16 PCM, but data is planar
};

/**
 * Gets the size of the given sample format, in bytes.
 * For planar formats, this returns the size of a single element
 * within the data plane
 * 
 * @param format The format to examine
 * @returns The size of format, in bytes
 */
inline size_t GetSampleFormatSize(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float:
		case SampleFormat::PlanarFloat:
			return sizeof(float);
		case SampleFormat::PCM:
		case SampleFormat::PlanarPCM:
			return sizeof(int16_t);
		case SampleFormat::Unknown:
		default:
			return 0;
	}
}

/**
 * Returns whether the given sample format is planar or interleaved
 * 
 * @param format The format to examine
 * @returns True for planar samples, false for interleaved samples
 */
inline bool IsFormatPlanar(SampleFormat format) {
	switch (format) {
		switch (format) {
			case SampleFormat::PlanarFloat:
			case SampleFormat::PlanarPCM:
				return true;
			case SampleFormat::Float:
			case SampleFormat::PCM:
			case SampleFormat::Unknown:
			default:
				return false;
		}
	}
}

/**
 * Returns a human-readable name for the given format
 * 
 * @param format The format to return a name for
 */
inline std::string GetSampleFormatName(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float:       return "Float";
		case SampleFormat::PlanarFloat: return "Planar Floats";
		case SampleFormat::PCM:         return "PCM";
		case SampleFormat::PlanarPCM:   return "Planar PCM";
		case SampleFormat::Unknown:     return "Unknown";
	}
}

#ifdef INCLUDE_FFMPEG
/**
 * Converts a format to it's FFMPEG equivalent, or AV_SAMPLE_FMT_NONE if no match is found
 * 
 * @param format The format to convert
 * @returns An AVSampleFormat that corresponds to the given format
 */
inline int ToFfmpeg(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float: return 3; //AV_SAMPLE_FMT_FLT
		case SampleFormat::PlanarFloat: return 8; // AV_SAMPLE_FMT_FLTP
		case SampleFormat::PCM:   return 1; // AV_SAMPLE_FMT_S16
		case SampleFormat::Unknown: 
		default:
			return -1; //AV_SAMPLE_FMT_NONE
	}
}

/**
 * Converts an FFMPEG format into it's equivalent SampleFormat, or SampleFormat::Unknown if
 * no match is found
 * 
 * @param format The format to convert
 * @returns A SampleFormat that corresponds to the given format
 */
inline SampleFormat FromFfmpeg(int format) {
	switch (format) {
		case 3: return SampleFormat::Float;
		case 8: return SampleFormat::PlanarFloat;
		case 1: return SampleFormat::PCM;
		default: return SampleFormat::Unknown;
	}
}
#endif