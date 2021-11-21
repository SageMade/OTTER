#pragma once
#include "AudioFramework/SampleFormat.h"

class IAudioEncoder;
struct AudioInStreamConfig;

class Resampler {
public:
	struct StreamConfig {
		uint8_t      NumChannels;
		SampleFormat Format;
		uint32_t     SampleRate;
		uint32_t     FrameSampleCount;
		// Pointer to buffers to read from or output to
		uint8_t*     ChannelBuffs[8];
	};

	Resampler();
	virtual ~Resampler();

	Resampler(const Resampler& other) = delete;
	Resampler(Resampler&& other) = delete;
	Resampler& operator=(const Resampler& other) = delete;
	Resampler& operator=(Resampler&& other) = delete;

	/**
	 * Sets the output buffer for the given channel, the size of which should be FrameSampleCount * sizeof(Format) bytes
	 * Note for interleaved buffers, should be a single channel bound to slot zero, with size FrameSampleCount * sizeof(Format) * numChannels
	 * 
	 * @param channel The ID of the channel to set
	 * @param backingField A pointer to the backing buffer to store data
	*/
	void SetOutputChannelPtr(uint8_t channel, uint8_t* backingField);
	/**
	 * Sets the input buffer for the given channel, the size of which should be FrameSampleCount * sizeof(Format) bytes
	 * Note for interleaved buffers, should be a single channel bound to slot zero, with size FrameSampleCount * sizeof(Format) * numChannels
	 * 
	 * @param channel The ID of the channel to set
	 * @param backingField A pointer to the backing buffer to store data
	*/
	void SetInputChannelPtr(uint8_t channel, uint8_t* backingField);

	/**
	 * Sets the number of samples that each input channel stores. This is used to determine the size
	 * of the input buffers
	 * 
	 * Cannot be called after initialization 
	 * 
	 * @param frameCount The number of samples in a single frame for re-sampling, per channel
	 */
	void SetInputFrameSampleCount(uint32_t frameCount);
	/**
	 * Sets the number of samples that each output channel stores. This is used to determine the size
	 * of the output buffers
	 *
	 * Cannot be called after initialization
	 * 
	 * @param frameCount The number of samples in a single frame for re-sampling, per channel
	 */
	void SetOutputFrameSampleCount(uint32_t sampleCount);

	/**
	 * Matches the input format to the given audio stream config.
	 * 
	 * Note that this will detach all input buffers that have been assigned
	 *
	 * Cannot be called after initialization
	 * 
	 * @param inputStream The input audio stream configuration parameters
	 */
	void MatchSourceEncoding(const AudioInStreamConfig& inputStream);
	/**
	 * Matches the output format to the given audio encoder, and attaches it's
	 * input buffers as our output buffers
	 *  
	 * Cannot be called after initialization
	 * 
	 * @param encoder The encoder to attach the output stream to
	 */
	void MatchDestEncoding(const IAudioEncoder* encoder);

	/**
	 * Overrides the input configuration for this resampler.
	 * 
	 * Note that this will detach all input buffers that have been assigned
	 * 
	 * @param config The new config to use for data input
	 */
	void SetInputConfig(const StreamConfig& config);
	/**
	 * Gets the resampler's input configuration
	 */
	const StreamConfig& GetInputConfig() const;

	/**
	 * Overrides the output configuration for this resampler.
	 * 
	 * Note that this will detach all output buffers that have been assigned
	 * 
	 * @param config The new config to use for data output
	 */
	void SetOutputConfig(const StreamConfig& config);
	/**
	 * Gets the resampler's output configuration
	 */
	const StreamConfig& GetOutputConfig() const;

	/**
	 * Initializes the resampler and configures it's internal state. Calling this 
	 * disallows further modification of parameters (aside from buffer pointers)
	 */
	void Init();

	/**
	 * Re-samples a single frame of data from the input buffers, storing result
	 * in the output buffers
	 * 
	 * @param sampleCount The number of samples to re-sample, default is the entire
	 * input sample frame count
	 */
	uint32_t EncodeFrame(uint32_t sampleCount = 0);

protected:
	StreamConfig _inConfig;
	StreamConfig _outConfig;

	struct ResamplerContext;

	ResamplerContext* _context;
};