#pragma once
#include <string>
#include <functional>
#include <AudioFramework/SampleFormat.h>

/**
 * Basic configuration data for an audio capture stream
 */
struct AudioInStreamConfig {
	/**
	 * The number of channels the stream is outputting
	 */
	uint8_t      NumChannels;
	/**
	 *  The stream's output format
	 */
	SampleFormat Format;
	/**
	 *  The stream's sampling rate (samples per second)
	 */
	uint32_t     SampleRate;
};

/**
 * Interface for an audio capture stream (ex a WMF port or Jack Audio stream)
 *
 * These should only need to be implemented for platforms that the capture suite will be ported to
 */
class IAudioCapDevice {
public:
	/**
	 * Typedef for a function that can receive data from an audio capture device
	 * 
	 * The first parameter will receive the data planes from the capture device
	 * The second parameter will receive the size of the data plane in bytes
	 * 
	 * Note that for interleaved data, only a single buffer should be returned
	 * For planar data, the number of buffers needs to = number of channels
	 */
	typedef std::function<void(const uint8_t**, size_t)> DataCallback;

	virtual ~IAudioCapDevice() = default;

	// Delete copy and move
	IAudioCapDevice(const IAudioCapDevice& other) = delete;
	IAudioCapDevice(IAudioCapDevice&& other) = delete;
	IAudioCapDevice& operator=(const IAudioCapDevice& other) = delete;
	IAudioCapDevice& operator=(IAudioCapDevice&& other) = delete;


	/**
	 * Gets the stream in configuration for this audio device
	 */
	const AudioInStreamConfig& GetConfig() const { return _config; };

	/**
	 * Initializes the audio stream and attempts to match the given stream config if applicable
	 * 
	 * Note that this does not guarantee that the stream will contain these configuration parameters,
	 * these are simply hints for the audio capture device to try and match
	 * 
	 * @param targetConfig The target config for the device, or nullptr to use defaults
	 */
	virtual void Init(const AudioInStreamConfig* targetConfig = nullptr) = 0;
	/**
	 * Polls the audio device, dispatching the data handling callback if data is present
	 * 
	 * @param callback The method to invoke when this device has received audio data
	 */
	virtual void PollDevice(DataCallback callback) = 0;
	/**
	 * Stops this audio stream, must be re-initialized before use
	 */
	virtual void Stop() = 0;

	/**
	 * Creates a copy of the audio capture device
	 * 
	 * This may be used to create a copy of the device that persists beyond the life span of 
	 * the IAudioCapDeviceEnumerator that created it
	 */
	virtual IAudioCapDevice* Clone() const = 0;

	/**
	 * Gets the human readable name of this device
	 */
	virtual const std::string& GetName() const { return _humanReadableName; }

protected:
	IAudioCapDevice() = default;

	std::string         _humanReadableName;
	AudioInStreamConfig _config;
};