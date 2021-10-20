#pragma once
#include <cstdint>
#include <functional>
#include <AudioFramework/AudioInStreamConfig.h>

enum class EncodingFormat {
	AAC,
	FLAC
};

struct AudioEncoderContext;

struct EncoderResult {
	uint8_t* Data;
	size_t   DataSize;
	size_t   Duration;
};

class AudioEncoder {
public:
	typedef std::function<void(EncoderResult* data)> PacketEncodedCallback;

	AudioEncoder();
	virtual ~AudioEncoder();

	void SetEncodingFormat(EncodingFormat format);
	void SetBitRate(uint32_t bitrate);
	void SetTargetSampleRate(uint32_t sampleRate);
	void SetTargetChannels(uint8_t numChannels);


	void SetTargetSampleFormat(SampleFormat format);

	void Init();

	EncoderResult* CreatePacket() const;
	void FreePacket(EncoderResult** packet);
	size_t FrameSampleCount() const;
	uint8_t GetNumChannels() const;
	uint32_t GetSampleRate() const;
	uint32_t GetBitRate() const;
	size_t GetFrameBufferSize() const;
	size_t CalcFrameBufferSize(SampleFormat format) const;

	uint8_t* GetInputBufferPtr(uint8_t channel) const;

	void EncodeFrame(PacketEncodedCallback callback, EncoderResult* packet = nullptr);
	void Flush(PacketEncodedCallback, EncoderResult* packet = nullptr);

	SampleFormat GetActualSampleFormat() const;


protected:
	struct {
		EncodingFormat Format       = EncodingFormat::AAC;
		uint8_t        NumChannels  = 2;
		uint32_t       BitRate      = 64000;
		uint32_t       SampleRate   = 44100;
		SampleFormat   SampleInFormat = SampleFormat::Unknown;
	} Config;

	AudioEncoderContext* _encoderContext;
	EncoderResult*       _syncResult;

	int _SelectOptimalSampleRate(int target = 44100);
	void _ValidateSampleFormat();

};