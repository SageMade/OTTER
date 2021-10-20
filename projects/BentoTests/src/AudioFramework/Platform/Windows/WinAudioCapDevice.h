#pragma once

#include <AudioFramework/IAudioCapDevice.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

class WinAudioCapDevice : public IAudioCapDevice {
public:
	virtual ~WinAudioCapDevice();

	// Inherited via IAudioCapDevice
	virtual void Init(const AudioInStreamConfig* targetConfig = nullptr) override;
	virtual void PollDevice(DataCallback callback) override;
	virtual void Stop() override;

protected:
	friend class WinAudioCapDeviceEnumerator;

	IMFActivate*     _attributes;
	IMFMediaSource*  _device;
	IMFSourceReader* _reader;
	IMFMediaType*    _mediaType;

	WinAudioCapDevice(IMFActivate* wmfDevice);
};