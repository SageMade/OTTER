#pragma once
#include <functional>
#include <AudioFramework/IAudioCapDevice.h>

void TestWaveAudio(const AudioInStreamConfig& config, std::function<void(const uint8_t*, size_t)> callback);
void TestWaveAudio2(std::function<void(const uint8_t*, size_t)> callback);