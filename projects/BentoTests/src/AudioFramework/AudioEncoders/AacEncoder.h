#pragma once
#include <AudioFramework/AudioEncoders/IAudioEncoder.h>
#include <FDK_audio.h> // for TRANSPORT_TYPE

struct AacEncoderContext;

/// <summary>
/// Represents an MPEG-4 audio object type
/// </summary>
enum class AacMediaType {
	Unknown = 0,
	Main    = 1, // Main format
	LC      = 2, // Low Complexity <- use as default
	SSR     = 3, // Scalable Sample Rate
	LTP     = 4, // Long Term Prediction
	PS      = 29 // DOES NOT WORK WITH ADTS TRANSPORT
};

/// <summary>
/// Wraps around an FDK TRANSPORT_TYPE, determines what data
/// stream format will be output by the encoder
/// </summary>
enum class AacTransportType {
	Unknown = -1,
	Raw     = 0, // Raw AAC frames
	ADIF    = 1, // Audio Data Interchange Format
	ADTS    = 2, // Audio Data Transport Stream
	LOAS    = 10 // Low Overhead Audio Stream
};

/// <summary>
/// Wraps around the Fraunhofer AAC encoder, providing an AAC audio encoder
/// </summary>
class AacEncoder : public IAudioEncoder {
public:
	AacEncoder();
	virtual ~AacEncoder();

	/// <summary>
	/// Gets the encoder format for this encoder
	/// </summary>
	/// <returns>EncodingFormat::AAC</returns>
	virtual EncodingFormat GetEncoderFormat() const override;
	/// <summary>
	/// Gets the input sample format for this encoder
	/// </summary>
	/// <returns>SampleFormat::PCM</returns>
	virtual SampleFormat GetInputFormat() const override;

	/// <summary>
	/// Sets whether to enable the FDK afterburner, which can improve audio quality
	/// at the cost of processing speed and memory usage (default false)
	/// Cannot be called after the encoder is initialized
	/// </summary>
	/// <param name="isEnabled">True if afterburner should be used, false otherwise</param>
	void SetAfterburnerEnabled(bool isEnabled);
	/// <summary>
	/// Returns true if FDK afterburner is enabled for this encoder
	/// </summary>
	bool GetAfterburnerEnabled() const { return _aacConfig.UseAfterburner; }

	/// <summary>
	/// Sets the transport type that this encoder should use, for instance ADTS, ADIF, etc...
	/// Cannot be called after the encoder is initialized
	/// </summary>
	/// <param name="type">The transport type for the output bitstream</param>
	void SetTransportType(AacTransportType type);
	/// <summary>
	/// Gets the transport type that this encoder is using
	/// </summary>
	AacTransportType GetTransportType() const { return _aacConfig.TransportType; }

	/// <summary>
	/// Sets whether the encoder should generate CRC frame validations. Only applicable to
	/// ADTS streams
	/// Cannot be called after the encoder is initialized
	/// </summary>
	/// <param name="isEnabled">True if CRC encoding should be enabled, false if otherwise</param>
	void SetCrcEnabled(bool isEnabled);
	/// <summary>
	/// Gets whether the encoder should generate CRC frame validations
	/// </summary>
	bool GetCrcEnabled() const { return _aacConfig.IsCrcEnabled; }

	/// <summary>
	/// Sets the MPEG-4 Audio Object Type for this encoder (ie what encoding format to use)
	/// Cannot be called after the encoder is initialized
	/// </summary>
	/// <param name="mediaType">The MPEG-4 media type</param>
	void SetAOT(AacMediaType mediaType);
	/// <summary>
	/// Gets the MPEG-4 Audio Object Type for this encoder (ie what encoding format to use)
	/// </summary>
	AacMediaType GetAOT() const { return _aacConfig.MediaType; }

	/// <summary>
	/// Gets the number of samples that this encoder can handle
	/// Cannot be called before the encoder is initialized
	/// </summary>
	/// <returns></returns>
	uint32_t GetInputSampleCapacity() const;

	// Inherited from IAudioEncoder
	virtual size_t GetSamplesPerInputFrame() const override;
	virtual int Init() override;
	virtual uint8_t* GetInputBuffer(uint8_t channelIx) const override;
	virtual size_t NotifyNewDataInBuffer(size_t numSamples) override;
	virtual size_t GetPendingSampleCount() const override;
	virtual size_t GetInputBufferSizeBytes() const override;
	virtual size_t GetInputBufferSizeSamples() const override;
	virtual void EncodeFrame(const uint8_t** data, size_t numBytes, bool async = false);
	virtual void GetEncodedResults() override;
	virtual void Flush(bool async = true) override;

protected:
	// Encoder specific config
	struct AacConfig {
		/// <summary>
		/// Whether this encoder should use afterburner (increase quality at a processing cost)
		/// </summary>
		bool             UseAfterburner;
		/// <summary>
		/// Whether this encoder should include CRC checks in ADTS headers
		/// </summary>
		bool             IsCrcEnabled;
		/// <summary>
		/// The transport type to use, by default this will be TT_MP4_ADTS
		/// </summary>
		AacTransportType TransportType;
		/// <summary>
		/// The size of the input buffer to allocate for input samples, will
		/// be set by the encoder upon initialization;
		/// </summary>
		uint32_t         InputSampleCapacity;
		/// <summary>
		/// The Audio Object Type (AOT) to use for encoding
		/// </summary>
		AacMediaType     MediaType;
	};
	AacConfig _aacConfig;

	// Stores the AacEncoder context, which in turn stores the state of an AacEncoder
	AacEncoderContext* _context;

	void _ConfigureInOutBuffers();
};