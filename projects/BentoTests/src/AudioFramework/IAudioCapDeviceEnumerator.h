#pragma once
#include <AudioFramework/IAudioCapDevice.h>
#include <assert.h>

// TODOs:
// [ ] Provide a callback interface for when capture devices have been added/removed
// [ ] Method for storing selected input device between runs
//      - Human readable name?
//      - Provide ability for IAudioCap devices to return a GUID?

/**
 * Base class for listing audio capture devices, should be overridden to provide support for audio 
 * device enumeration on other platforms (ex: WMF, Jack Audio, OSX, etc...)
 * 
 * Note that the devices returned by the enumerator are owned by the enumerator itself, thus need to
 * be cloned if you wish to preserve a device beyond the enumerators lifespan
 * 
 * These platforms only need to be supported on those platforms that the capture suite will be ported to
 */
class IAudioCapDeviceEnumerator {
public:
	virtual ~IAudioCapDeviceEnumerator() = default;

	// Delete copy and assignment operators
	IAudioCapDeviceEnumerator(const IAudioCapDeviceEnumerator& other) = delete;
	IAudioCapDeviceEnumerator(IAudioCapDeviceEnumerator&& other) = delete;
	IAudioCapDeviceEnumerator& operator=(const IAudioCapDeviceEnumerator& other) = delete;
	IAudioCapDeviceEnumerator& operator=(IAudioCapDeviceEnumerator&& other) = delete;

	/**
	 * Gets the number of audio capture devices currently enabled on the system
	 */
	virtual size_t DeviceCount() const = 0;
	/**
	 * Gets an array of all audio capture devices enabled on the system
	 */
	virtual  IAudioCapDevice**const GetDevices() const = 0;

	/**
	 * Gets a pointer to the default audio recording device if applicable
	 * 
	 * @returns The default recording device, or nullptr if not available/applicable
	 */
	virtual IAudioCapDevice* GetDefaultDevice() const = 0;

	/**
	 * Gets the recording device with the given index, or nullptr if the index is not a valid device index
	 * 
	 * @param index The index of the device to get (must be < the DeviceCount)
	 * @returns The Audio Capture Device at that index, or nullptr if the index is out of range
	 */
	virtual IAudioCapDevice* GetDevice(uint32_t index) const = 0;
	
	/**
	 * Gets the recording device with the given human readable name, or nullptr if no such device exists
	 *
	 * @param name The human readable name of the device to select
	 * @returns The Audio Capture Device with that name, or nullptr
	 */
	virtual IAudioCapDevice* GetDevice(const std::string& name) const = 0;

protected:
	IAudioCapDeviceEnumerator() = default;
};