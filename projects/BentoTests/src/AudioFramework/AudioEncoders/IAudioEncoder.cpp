#include <AudioFramework/AudioEncoders/IAudioEncoder.h>
#include <Logging.h>

void IAudioEncoder::SetConfig(const EncoderConfig& config) {
	LOG_ASSERT(_rawContext == nullptr, "Cannot modify config after encoder has been initialized");
	_config = config;
}
const EncoderConfig& IAudioEncoder::GetConfig() const noexcept {
	return _config;
}

std::mutex& IAudioEncoder::GetInputBufferLock() noexcept {
	return _inputBufferLock;
}

void IAudioEncoder::SetDataCallback(SyncDataCallback callback) noexcept {
	_dataCallback = callback;
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

uint32_t IAudioEncoder::GetBitRate() const noexcept { return _config.BitRate; }
uint32_t IAudioEncoder::GetSampleRate() const noexcept { return _config.SampleRate; }
uint32_t IAudioEncoder::GetNumChannels() const noexcept { return _config.NumChannels; }
uint8_t IAudioEncoder::GetMaxAsyncFrames() const noexcept { return _config.MaxAsyncFrames; }

IAudioEncoder::IAudioEncoder(EncoderConfig config /*= EncoderConfig()*/) :
	_config(config),
	_rawContext(nullptr),
	_inputBufferLock(std::mutex()),
	_outputBufferLock(std::mutex()),
	_dataCallback(nullptr)
{

}
