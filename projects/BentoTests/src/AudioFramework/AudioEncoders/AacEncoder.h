#pragma once
#include <AudioFramework/AudioEncoders/IAudioEncoder.h>

/**
 * Represents an MPEG-4 audio object type
 */
enum class AacMediaType {
	Unknown = 0,
	// Main format
	Main    = 1, 
	// Low Complexity <- use as default
	LC      = 2, 
	// Scalable Sample Rate
	SSR     = 3, 
	// Long Term Prediction
	LTP     = 4, 
	// Parametric Stereo Best quality, DOES NOT WORK WITH ADTS TRANSPORT or mono streams
	PS      = 29 
};

/**
 * Wraps around an FDK TRANSPORT_TYPE, determines what data
 * stream format will be output by the encoder
 */
enum class AacTransportType {
	Unknown = -1,
	Raw     = 0, // Raw AAC frames
	ADIF    = 1, // Audio Data Interchange Format
	ADTS    = 2, // Audio Data Transport Stream
	LOAS    = 10 // Low Overhead Audio Stream
};

/**
 * Wraps around the Fraunhofer AAC encoder, and extends the 
 * IAudioEncoder interface to allow converting raw audio input
 * into AAC encoded data frames
 */
class AacEncoder : public IAudioEncoder {
public:
	AacEncoder();
	virtual ~AacEncoder();

	/**
	 * Gets the output format for this encoder
	 * 
	 * @returns EncodingFormat::AAC
	 */
	virtual EncodingFormat GetEncoderFormat() const noexcept override;
	/**
	 * Gets the input sample format for this encoder
	 * 
	 * @returns SampleFormat::PCM
	 */
	virtual SampleFormat GetInputFormat() const noexcept override;
	/**
	 * Gets whether this encoder has been designed for asynchronous encode/read
	 * 
	 * @returns false
	 */
	virtual bool GetAsyncSupported() const noexcept override;

	/**
	 * Sets whether to enable the FDK afterburner algorithm.
	 * 
	 * FDK Afterburner is a tool in the FDK encoder which can improve audio quality
	 * at the cost of processing speed and memory usage (default false)
	 * 
	 * This function cannot be called after the encoder is initialized
	 * 
	 * @param isEnabled True if afterburner should be used, false otherwise
	 */
	void SetAfterburnerEnabled(bool isEnabled);
	/**
	 * Returns true if the FDK afterburner algorithm is enabled for this encoder
	 */
	bool GetAfterburnerEnabled() const { return _aacConfig.UseAfterburner; }

	/**
	 * Sets the output stream transport type for this encoder. Defaults to ADTS
	 * 
	 * ADIF appears to have the most support for decoding.
	 * ADTS seems to also work well with some MP4 readers (ex: VLC). 
	 * Raw should be used if planning to implement custom transport layers.
	 * LOAS requires stereo audio, which does not work with spacial audio without
	 * additional processing, and should generally not be used
	 *
	 * This function cannot be called after the encoder is initialized
	 * 
	 * @param type The transport type to use.
	 */
	void SetTransportType(AacTransportType type);
	/**
	 * Returns the transport type that this encoder will output using
	 */
	AacTransportType GetTransportType() const { return _aacConfig.TransportType; }

	/**
	 * Sets whether the encoder should generate CRC checksums for output frames. 
	 * Only applicable to ADTS streams
	 * 
	 * This function cannot be called after the encoder is initialized
	 * 
	 * @param isEnabled True if CRC encoding should be enabled, false if otherwise
	 */
	void SetCrcEnabled(bool isEnabled);
	/**
	 * Returns true if this encoder is generating CRC checksums for output frames
	 */
	bool GetCrcEnabled() const { return _aacConfig.IsCrcEnabled && _aacConfig.TransportType == AacTransportType::ADTS; }

	/**
	 * Sets the MPEG-4 Audio Object Type for this encoder. Defaults to AAC_LC
	 *
	 * This function cannot be called after the encoder is initialized
	 * 
	 * @param mediaType The MPEG-4 media type (ex: AAC_LC, AAC_PS, etc...)
	 */
	void SetAOT(AacMediaType mediaType);
	/**
	 * Gets the MPEG-4 Audio Object Type this encoder is configured to output
	 */
	AacMediaType GetAOT() const { return _aacConfig.MediaType; }

	/** 
	 * Gets the number of samples that this encoder requires to process a single frame
	 * 
	 * This function be called before the encoder is initialized
	 * 
	 * @returns The number of samples required to encode a single output frame
	 */
	uint32_t GetInputSampleCapacity() const;

	// Inherited from IAudioEncoder

	virtual int Init() override;
	virtual size_t GetSamplesPerInputFrame() const override;
	virtual uint8_t* GetInputBuffer(uint8_t channelIx) const override;
	virtual size_t NotifyNewDataInBuffer(size_t numSamples) override;
	virtual size_t GetPendingSampleCount() const override;
	virtual size_t GetInputBufferSizeBytes() const override;
	virtual size_t GetInputBufferSizeSamples() const override;
	virtual void EncodeFrame(const uint8_t** data, size_t numBytes, bool async = false);
	virtual void GetEncodedResults() override;
	virtual void Flush(bool async = false) override;

protected:
	// Encoder specific config
	struct AacConfig {
		// Whether this encoder should use afterburner (increase quality at a processing cost)
		bool             UseAfterburner;
		// Whether this encoder should include CRC checks in ADTS headers
		bool             IsCrcEnabled;
		// The transport type to use, by default this will be TT_MP4_ADTS
		AacTransportType TransportType;
		// The size of the input buffer to allocate for input samples, will
		// be set by the encoder upon initialization
		uint32_t         InputSampleCapacity;
		// The Audio Object Type (AOT) to use for encoding
		AacMediaType     MediaType;
	};
	AacConfig _aacConfig;

	// Pre-declare so internal structure is not exposed in headers
	struct AacEncoderContext;
	// Stores the AacEncoder context, which in turn stores the state of an FDK encoder
	AacEncoderContext* _context;

	// Helper function to handle allocating and configuring input and output buffers
    // for use with the FDK encoder
	void _ConfigureInOutBuffers();

	// Helper function for handling output data from the encoder
	void _HandleOutputDataFrame(bool async, int numInSamples, int numOutBytes);
};