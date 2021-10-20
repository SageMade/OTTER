#pragma once
#include <AudioFramework/IAudioCapDeviceEnumerator.h>
#include <mfobjects.h>

class WinAudioCapDeviceEnumerator : public IAudioCapDeviceEnumerator {
public:
	WinAudioCapDeviceEnumerator();
	virtual ~WinAudioCapDeviceEnumerator();

protected:
	IMFActivate** _imfDevices;
};