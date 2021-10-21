#include <AudioFramework/AudioEncoders/IAudioEncoder.h>
#include <Logging.h>

void IAudioEncoder::SetConfig(const EncoderConfig& config) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config = config;
}
const EncoderConfig& IAudioEncoder::GetConfig() const {
	return _config;
}

void IAudioEncoder::SetBitRate(uint32_t bitrate) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config.BitRate = bitrate;
}
void IAudioEncoder::SetSampleRate(uint32_t sampleRate) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config.SampleRate = sampleRate;
}
void IAudioEncoder::SetNumChannels(uint8_t numChannels) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config.NumChannels = numChannels;
}
void IAudioEncoder::SetMaxAsyncFrames(uint8_t numFrames) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config.MaxAsyncFrames = numFrames;
}
void IAudioEncoder::SetAsyncSupported(bool isSupported) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config.AsyncSupport = isSupported;
}

uint32_t IAudioEncoder::GetBitRate() const { return _config.BitRate; }
uint32_t IAudioEncoder::GetSampleRate() const { return _config.SampleRate; }
uint32_t IAudioEncoder::GetNumChannels() const { return _config.NumChannels; }
uint8_t IAudioEncoder::GetMaxAsyncFrames() const { return _config.MaxAsyncFrames; }
bool IAudioEncoder::GetAsyncSupported() const { return _config.AsyncSupport; }
