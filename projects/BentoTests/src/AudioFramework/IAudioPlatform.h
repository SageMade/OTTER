#pragma once
#include <AudioFramework/IAudioCapDeviceEnumerator.h>

/**
 * Interface for audio platform support. Audio platform wrappers (ex: WMF, Jack Audio)
 * should derive from this interface
 * 
 * This interface provides a place for performing library initialization and cleanup, as
 * well as device enumeration
 *
 * These platforms only need to be supported on those platforms that the capture suite will be ported to
 */
class IAudioPlatform {
public:
	virtual ~IAudioPlatform() = default;

	//Delete copy and move
	IAudioPlatform(const IAudioPlatform& other) = delete;
	IAudioPlatform(IAudioPlatform&& other) = delete;
	IAudioPlatform& operator=(const IAudioPlatform& other) = delete;
	IAudioPlatform& operator=(IAudioPlatform&& other) = delete;

	/**
	 * Handles initializing shared resources for this audio platform, 
	 * and performs any other required library initialization
	 */
	virtual void Init() = 0;
	/**
	 * Handles cleaning up shared resources and uninitializing any library
	 * resources created in Init
	 */
	virtual void Cleanup() = 0;

	/**
	 * Returns a new device enumerator for this platform
	 *
	 * The resulting enumerator should be allocated via new for all platforms. 
	 * 
	 * Ownership of the enumerator belongs to the caller, which can free the
	 * enumerator using the delete keyword (NO MALLOC!)
	 * 
	 * @returns A new audio capture device enumerator for the platform
	 */
	virtual IAudioCapDeviceEnumerator* GetDeviceEnumerator() = 0;

	/**
	 * Returns a human-readable prefix for this platform
	 * 
	 * Can be used to generate names for device selection (ex: "WMF: Microphone in", "JACK: Input A")
	 */
	virtual std::string GetPrefix() = 0;

protected:
	IAudioPlatform() = default;
};