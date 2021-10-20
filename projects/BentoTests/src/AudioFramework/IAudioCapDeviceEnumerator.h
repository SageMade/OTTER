#pragma once
#include <AudioFramework/IAudioCapDevice.h>
#include <assert.h>

/// <summary>
/// Base class for listing audio capture devices, can be overriden to provide support for audio device enumeration
/// on other platforms
/// </summary>
class IAudioCapDeviceEnumerator {
public:
	virtual ~IAudioCapDeviceEnumerator() = default;

	// Delete copy and assingment operators
	IAudioCapDeviceEnumerator(const IAudioCapDeviceEnumerator& other) = delete;
	IAudioCapDeviceEnumerator(IAudioCapDeviceEnumerator&& other) = delete;
	IAudioCapDeviceEnumerator& operator=(const IAudioCapDeviceEnumerator& other) = delete;
	IAudioCapDeviceEnumerator& operator=(IAudioCapDeviceEnumerator&& other) = delete;

	/// <summary>
	/// Gets the number of audio capture devices currently enabled on the system
	/// </summary>
	virtual size_t DeviceCount() const { return _numDevices; };
	/// <summary>
	/// Gets an array of all audio capture devices enabled on the system
	/// </summary>
	virtual  IAudioCapDevice**const GetDevices() const { return _devices; }

	/// <summary>
	/// Gets the default audio recording device if applicable
	/// </summary>
	/// <returns>The default recording device, or nullptr if not available/applicable</returns>
	virtual const IAudioCapDevice* GetDefaultDevice() const { return nullptr; }
	/// <summary>
	/// Gets the default audio recording device if applicable
	/// </summary>
	/// <returns>The default recording device, or nullptr if not available/applicable</returns>
	virtual IAudioCapDevice* GetDefaultDevice() { return nullptr; }

	/// <summary>
	/// Gets the recording device with the given index, or nullptr if the index is not a valid device index
	/// </summary>
	virtual const IAudioCapDevice* GetDevice(uint32_t index) const { return index < _numDevices ? _devices[index] : nullptr; }
	/// <summary>
	/// Gets the recording device with the given index, or nullptr if the index is not a valid device index
	/// </summary>
	virtual IAudioCapDevice* GetDevice(uint32_t index) { return index < _numDevices ? _devices[index] : nullptr; }

protected:
	IAudioCapDeviceEnumerator() = default;

	size_t         _numDevices;
	IAudioCapDevice** _devices;
};