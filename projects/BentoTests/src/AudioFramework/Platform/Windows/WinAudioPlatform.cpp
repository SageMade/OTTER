#include "WinAudioPlatform.h"

#include <mfapi.h>
#include <AudioFramework/Platform/Windows/WinAudioCapDeviceEnumerator.h>
#include <Logging.h>

WinAudioPlatform::WinAudioPlatform() : IAudioPlatform() {
}

WinAudioPlatform::~WinAudioPlatform() {
}

void WinAudioPlatform::Init() {
	MFStartup(MF_VERSION);
	LOG_INFO("Initializing Windows Media Foundation audio subsystem");
}

void WinAudioPlatform::Cleanup() {
	MFShutdown();
	LOG_INFO("Cleaning up Windows Media Foundation audio subsystem");
}

IAudioCapDeviceEnumerator* WinAudioPlatform::GetDeviceEnumerator() {
	return new WinAudioCapDeviceEnumerator();
}

std::string WinAudioPlatform::GetPrefix() {
	return "WIN";
}
