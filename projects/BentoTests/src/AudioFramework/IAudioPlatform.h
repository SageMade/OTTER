#pragma once
#include <AudioFramework/IAudioCapDeviceEnumerator.h>

class IAudioPlatform {
public:
	virtual ~IAudioPlatform() = default;

	IAudioPlatform(const IAudioPlatform& other) = delete;
	IAudioPlatform(IAudioPlatform&& other) = delete;
	IAudioPlatform& operator=(const IAudioPlatform& other) = delete;
	IAudioPlatform& operator=(IAudioPlatform&& other) = delete;

	virtual void Init() = 0;
	virtual void Cleanup() = 0;

	virtual IAudioCapDeviceEnumerator* GetDeviceEnumerator() = 0;

protected:
	IAudioPlatform() = default;
};