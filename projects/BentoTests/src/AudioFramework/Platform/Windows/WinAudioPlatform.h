#pragma once
#include <AudioFramework/IAudioPlatform.h>

/**
 * Extends the IAudioPlatform interface to provide audio capture
 * capabilities for the Windows platform using Windows Media Foundation
 */
class WinAudioPlatform final: public IAudioPlatform {
public:
	WinAudioPlatform();
	virtual ~WinAudioPlatform();

	// Inherited via IAudioPlatform

	virtual void Init() override;
	virtual void Cleanup() override;
	virtual IAudioCapDeviceEnumerator* GetDeviceEnumerator() override;
	virtual std::string GetPrefix() override;
};