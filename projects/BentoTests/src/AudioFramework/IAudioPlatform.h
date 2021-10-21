#pragma once
#include <AudioFramework/IAudioCapDeviceEnumerator.h>

/// <summary>
/// Interface for audio platform support. Audio platform wrappers (ex: WMF, Jack Audio)
/// should derive from this interface
/// </summary>
class IAudioPlatform {
public:
	virtual ~IAudioPlatform() = default;

	IAudioPlatform(const IAudioPlatform& other) = delete;
	IAudioPlatform(IAudioPlatform&& other) = delete;
	IAudioPlatform& operator=(const IAudioPlatform& other) = delete;
	IAudioPlatform& operator=(IAudioPlatform&& other) = delete;

	/// <summary>
	/// Handles initializing shared resources for this audio platform, 
	/// and performs any other required initialization
	/// </summary>
	virtual void Init() = 0;
	/// <summary>
	/// Handles disposing of shared resources and performs any cleanup
	/// required for the platform
	/// </summary>
	virtual void Cleanup() = 0;

	/// <summary>
	/// Returns a new device enumerator for this platform, should be
	/// allocated via new for all platforms. Ownership of the enumerator
	/// belongs to the caller
	/// </summary>
	/// <returns>A new audio capture device enumerator for the platform</returns>
	virtual IAudioCapDeviceEnumerator* GetDeviceEnumerator() = 0;

protected:
	IAudioPlatform() = default;
};