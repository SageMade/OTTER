#pragma once
#include <AudioFramework/IAudioPlatform.h>

class WinAudioPlatform : public IAudioPlatform {
public:
	WinAudioPlatform();
	virtual ~WinAudioPlatform();

	// Inherited via IAudioPlatform
	virtual void Init() override;
	virtual void Cleanup() override;
	virtual IAudioCapDeviceEnumerator* GetDeviceEnumerator() override;

protected:
	IAudioCapDeviceEnumerator* _audioCapDevices;
};