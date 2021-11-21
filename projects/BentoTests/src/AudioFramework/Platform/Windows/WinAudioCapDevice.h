#pragma once

#include <AudioFramework/IAudioCapDevice.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

/**
 * Implements the IAudioCapDevice for Windows using Windows Media Foundation
 */
class WinAudioCapDevice final: public IAudioCapDevice {
public:
	virtual ~WinAudioCapDevice();

	// Inherited via IAudioCapDevice

	virtual void Init(const AudioInStreamConfig* targetConfig = nullptr) override;
	virtual void PollDevice(DataCallback callback) override;
	virtual void Stop() override;
	virtual IAudioCapDevice* Clone() const override;

private:
	// Allow WinAudioCapDeviceEnumerator to access the private fields
	friend class WinAudioCapDeviceEnumerator;

	// We need to store the whole array of devices since
	// Windows is dumb and we can't copy the activate passed
	// into the CTOR
	IMFActivate**    _devicesPtr;
	// This is our ACTUAL activator for this device
	IMFActivate*     _attributes;
	IMFMediaSource*  _device;
	IMFSourceReader* _reader;
	IMFMediaType*    _mediaType;

	WinAudioCapDevice(IMFActivate* wmfDevice);


};