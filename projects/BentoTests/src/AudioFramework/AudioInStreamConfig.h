#pragma once
#include <cstdint>
#include <AudioFramework/SampleFormat.h>


struct AudioInStreamConfig {
	uint8_t      NumChannels;
	SampleFormat Format;
	uint32_t     SampleRate;
};