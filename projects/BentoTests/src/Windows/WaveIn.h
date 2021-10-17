#pragma once
#include <functional>

void TestWaveAudio(int sampleRate, std::function<void(uint8_t*, size_t)> callback);
void TestWaveAudio2(std::function<void(uint8_t*, size_t)> callback);