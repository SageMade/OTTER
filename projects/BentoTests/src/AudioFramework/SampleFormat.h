#pragma once
#include <cstdint>
#include <string>
extern "C" {
	#include "libavutil/samplefmt.h"
}

/// <summary>
/// Represents an audio sample format for a stream of audio data
/// </summary>
enum class SampleFormat {
	Unknown     = 0,
	Float       = 1, // Interleaved floats
	PCM         = 2, // PCM
	PlanarFloat = 4, // Float, but each float is on it's own data plane 
};

/// <summary>
/// Gets the size of the given sample format, in bytes.
/// For planar formats, this returns the size of a single element
/// within the data plane
/// </summary>
/// <param name="format">The format to examine</param>
/// <returns>The size of format, in bytes</returns>
inline size_t GetSampleFormatSize(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float:
		case SampleFormat::PlanarFloat:
			return sizeof(float);
		case SampleFormat::PCM:
			return sizeof(uint16_t);
		case SampleFormat::Unknown:
		default:
			return 0;
	}
}

/// <summary>
/// Returns whether the given sample format is planar or interleaved
/// </summary>
/// <param name="format">The format to examine</param>
/// <returns>True for planar samples, false for interleaved samples</returns>
inline bool IsFormatPlanar(SampleFormat format) {
	switch (format) {
		switch (format) {
			case SampleFormat::PlanarFloat:
				return true;
			case SampleFormat::Float:
			case SampleFormat::PCM:
			case SampleFormat::Unknown:
			default:
				return false;
		}
	}
}

/// <summary>
/// Returns a human-readable name for the given format
/// </summary>
/// <param name="format">The format to return a name for</param>
inline std::string GetSampleFormatName(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float: return "Float";
		case SampleFormat::PlanarFloat: return "Planar Floats";
		case SampleFormat::PCM:   return "PCM";
		case SampleFormat::Unknown: return "Unknown";
	}
}

/// <summary>
/// Converts a format to it's FFMPEG equivalent, or AV_SAMPLE_FMT_NONE if no match is found
/// </summary>
/// <param name="format">The format to convert</param>
/// <returns>An AVSampleFormat that corresponds to the given format</returns>
inline AVSampleFormat ToFfmpeg(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float: return AV_SAMPLE_FMT_FLT;
		case SampleFormat::PlanarFloat: return AV_SAMPLE_FMT_FLTP;
		case SampleFormat::PCM:   return AV_SAMPLE_FMT_S16;
		case SampleFormat::Unknown: 
		default:
			return AV_SAMPLE_FMT_NONE;
	}
}

/// <summary>
/// Converts an FFMPEG format into it's equivalent SampleFormat, or SampleFormat::Unknown if 
/// no match is found
/// </summary>
/// <param name="format">The format to convert</param>
/// <returns>A SampleFormat that corresponds to the given format</returns>
inline SampleFormat FromFfmpeg(AVSampleFormat format) {
	switch (format) {
		case AV_SAMPLE_FMT_FLT: return SampleFormat::Float;
		case AV_SAMPLE_FMT_FLTP: return SampleFormat::PlanarFloat;
		case AV_SAMPLE_FMT_S16: return SampleFormat::PCM;
		default: return SampleFormat::Unknown;
	}
}

