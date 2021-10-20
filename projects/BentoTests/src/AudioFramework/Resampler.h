#pragma once
#include <AudioFramework/AudioInStreamConfig.h>

struct SwrContext;
class AudioEncoder;

class Resampler {
public:
	struct StreamConfig {
		uint8_t      NumChannels;
		SampleFormat Format;
		uint32_t     SampleRate;
		uint32_t     FrameSampleCount;
		uint8_t*     ChannelBuffs[8];
	};

	Resampler();
	virtual ~Resampler();

	Resampler(const Resampler& other) = delete;
	Resampler(Resampler&& other) = delete;
	Resampler& operator=(const Resampler& other) = delete;
	Resampler& operator=(Resampler&& other) = delete;

	/// <summary>
	/// Sets the output buffer for the given channel, the size of which should be FrameSampleCount * sizeof(Format) bytes
	/// Note for packed buffers, should be a single channel bound to slot zero, with size FrameSampleCount * sizeof(Format) * numChannels
	/// </summary>
	/// <param name="channel">The ID of the channel to set</param>
	/// <param name="backingField">A pointer to the backing buffer to store data</param>
	void SetOutputChannelPtr(uint8_t channel, uint8_t* backingField);
	/// <summary>
	/// Sets the input buffer for the given channel, the size of which should be FrameSampleCount * sizeof(Format) bytes
	/// Note for packed buffers, should be a single channel bound to slot zero, with size FrameSampleCount * sizeof(Format) * numChannels
	/// </summary>
	/// <param name="channel">The ID of the channel to set</param>
	/// <param name="backingField">A pointer to the backing buffer to store data</param>
	void SetInputChannelPtr(uint8_t channel, uint8_t* backingField);

	void SetInputFrameSampleCount(uint32_t frameCount);

	/// <summary>
	/// Matches the input format to the given audio stream config.
	/// Note that this will detach all output buffers that have been assigned!
	/// </summary>
	/// <param name="inputStream">The config of the input stream to match</param>
	void MatchSourceEncoding(const AudioInStreamConfig& inputStream);

	/// <summary>
	/// Matches the output format to the given audio encoder
	/// Note that this will detach all output buffers that have been assigned!
	/// </summary>
	/// <param name="encoder">The encoder to match the output stream to</param>
	void MatchDestEncoding(const AudioEncoder* encoder);

	/// <summary>
	/// Overrides the input configuration for this resampler.
	/// Note that this will detach all output buffers that have been assigned!
	/// </summary>
	/// <param name="config">The new config to use for data input</param>
	void SetInputConfig(const StreamConfig& config);
	/// <summary>
	/// Overrides the output configuration for this resampler.
	/// Note that this will detach all output buffers that have been assigned!
	/// </summary>
	/// <param name="config">The new config to use for data output</param>
	void SetOutputConfig(const StreamConfig& config);

	const StreamConfig& GetInputConfig() const { return _inConfig; }
	const StreamConfig& GetOutputConfig() const { return _outConfig; }

	void Init();

	void EncodeFrame(uint32_t sampleCount = 0);

protected:
	StreamConfig _inConfig;
	StreamConfig _outConfig;

	SwrContext* _ffmegResample;
};