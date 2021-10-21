#pragma once
#include <string>
#include <functional>
#include <AudioFramework/SampleFormat.h>

/// <summary>
/// Basic configuration data for an audio capture stream
/// </summary>
struct AudioInStreamConfig {
	/// <summary>
	/// The number of channels the stream is outputting
	/// </summary>
	uint8_t      NumChannels;
	/// <summary>
	/// The stream's output format
	/// </summary>
	SampleFormat Format;
	/// <summary>
	/// The stream's sampling rate (samples per second)
	/// </summary>
	uint32_t     SampleRate;
};

/// <summary>
/// Interface for an audio capture stream (ex a WMF port or Jack Audio stream)
/// </summary>
class IAudioCapDevice {
public:
	typedef std::function<void(const uint8_t**, size_t)> DataCallback;

	virtual ~IAudioCapDevice() = default;

	IAudioCapDevice(const IAudioCapDevice& other) = delete;
	IAudioCapDevice(IAudioCapDevice&& other) = delete;
	IAudioCapDevice& operator=(const IAudioCapDevice& other) = delete;
	IAudioCapDevice& operator=(IAudioCapDevice&& other) = delete;


	/// <summary>
	/// Gets the stream in configuration for this audio device
	/// </summary>
	const AudioInStreamConfig& GetConfig() const { return _config; };

	/// <summary>
	/// Initializes the audio stream and attempts to match the given stream config if applicable
	/// </summary>
	/// <param name="targetConfig">The target config for the device, if applicable</param>
	virtual void Init(const AudioInStreamConfig* targetConfig = nullptr) = 0;
	/// <summary>
	/// Polls the audio device, dispatching the data handling callback if data is present
	/// </summary>
	/// <param name="callback">The method to invoke when this device has received audio data</param>
	virtual void PollDevice(DataCallback callback) = 0;
	/// <summary>
	/// Stops this audio stream, must be re-initialized before use
	/// </summary>
	virtual void Stop() = 0;

	/// <summary>
	/// Gets the human readable name of this device
	/// </summary>
	virtual const std::string& GetName() const { return _humanReadableName; }

protected:
	IAudioCapDevice() = default;

	std::string         _humanReadableName;
	AudioInStreamConfig _config;
};