#pragma once
#include <AudioFramework/IAudioCapDeviceEnumerator.h>

class WinAudioCapDeviceEnumerator : public IAudioCapDeviceEnumerator {
public:
	WinAudioCapDeviceEnumerator();
	virtual ~WinAudioCapDeviceEnumerator();

	// Inherited from IAudioCapDeviceEnumerator

	virtual size_t DeviceCount() const override;
	virtual IAudioCapDevice**const GetDevices() const override;
	virtual IAudioCapDevice* GetDefaultDevice() const override;
	virtual IAudioCapDevice* GetDevice(uint32_t index) const override;
	virtual IAudioCapDevice* GetDevice(const std::string& name) const override;

protected:
	size_t         _numDevices;
	IAudioCapDevice** _devices;
};