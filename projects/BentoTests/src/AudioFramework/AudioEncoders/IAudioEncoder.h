#pragma once
#include <cstdint>
#include <functional>
#include <AudioFramework/SampleFormat.h>
#include <mutex>

/// <summary>
/// The output format of an encoder or decoder
/// </summary>
enum class EncodingFormat {
	AAC, // FDK AAC
	FLAC // TO BE DETERMINED
};

/// <summary>
/// Represents the encoded output frame, note that in synchronous mode,
/// Data will be a temporary handle valid only until the next EncodeFrame call
/// </summary>
struct EncoderResult {
	/// <summary>
	/// The pointer to the data store for the results
	/// In synchronous mode, only valid until the next EncodeFrame call,
	/// in async mode, valid until the queue of frames overflows
	/// </summary>
	uint8_t* Data;
	/// <summary>
	/// The size of the output data, in bytes
	/// </summary>
	size_t   DataSize;
	/// <summary>
	/// The duration of the result, as a numerator of sample rate
	/// (ex: time in sec = Duration / SampleRate)
	/// </summary>
	size_t   Duration;
	/// <summary>
	/// A mutex to lock the frame for use, can be used by
	/// decoder thread to block encoder thread from overwrites,
	/// not used in synchronous mode
	/// </summary>
	std::mutex Lock;

	EncoderResult() :
		Data(nullptr),
		DataSize(0),
		Duration(0),
		Lock(std::mutex()) { }
};

/// <summary>
/// The shared config settings for all encoders
/// </summary>
struct EncoderConfig {
	uint32_t BitRate        = 64000;
	uint32_t SampleRate     = 44100;
	uint32_t NumChannels    = 2;

	// Future shit
	uint8_t  MaxAsyncFrames = 8;
};

/**
 * Base class for encoders which convert an input stream of data to an encoded format
 * for later decoding (ex: AAC, FLAC)
 */
class IAudioEncoder {
public:
	/**
	 * Typedef for an std::function that can handle a resultant frame from an encoder
	 */
	typedef std::function<void(EncoderResult*)> SyncDataCallback;

	virtual ~IAudioEncoder() = default;

	// Delete move and copy operators
	IAudioEncoder(const IAudioEncoder& other) = delete;
	IAudioEncoder(IAudioEncoder&& other) = delete;
	IAudioEncoder& operator=(const IAudioEncoder& other) = delete;
	IAudioEncoder& operator=(IAudioEncoder&& other) = delete;
	

	/**
	 * Returns the encoding format that this encoder outputs
	 */
	virtual EncodingFormat GetEncoderFormat() const = 0;
	/**
	 * Returns the optimal input format for this encoder
	 */
	virtual SampleFormat GetInputFormat() const = 0;
	/**
	 * Returns true if the encoder has been designed for asynchronous encoding support 
	 */
	virtual bool GetAsyncSupported() const = 0;


	/**
	 * Sets all basic encoder config settings, overriding existing ones
	 *
	 * Cannot be called after initialization
	 *
	 * @param config The new config to override the existing one with
	 */
	virtual void SetConfig(const EncoderConfig& config);
	/**
	 * Returns the current config of the audio encoder
	 */
	virtual const EncoderConfig& GetConfig() const noexcept;

	/**
	 * Gets the mutex to lock this audio encoder's input buffer
	 */
	virtual std::mutex& GetInputBufferLock() noexcept;

	/**
	 * Sets the data callback to use for this encoder
	 *
	 * This callback function will receive encoded data frames when they are ready
	 * for serialization
	 * 
	 * Note that only a single data callback may be bound to an encoder at a time
	 * 
	 * @param callback The callback to bind
	 */
	virtual void SetDataCallback(SyncDataCallback callback) noexcept;

	/**
	 * Sets the target output bitrate of this encoder
	 * 
	 * This function cannot be called after initialization
	 * 
	 * @param bitrate The target bitrate for the encoder to use
     */
	virtual void SetBitRate(uint32_t bitrate);	
	/**
	 * Sets the input sample rate of this encoder
	 * 
	 * Note that not all encoders can support all sample rates.
	 * Prefer 44100hz when available
	 * 
	 * This function cannot be called after initialization
	 * 
	 * @param sampleRate The sample rate for the encoder to use
     */
	virtual void SetSampleRate(uint32_t sampleRate);
	/**	
	 * Sets the number of channels that this encoder is handling
	 *
	 * This function cannot be called after initialization
	 * 
	 * @param numChannels The number of channels for the encoder to read/write
	 */
	virtual void SetNumChannels(uint8_t numChannels);
	/**
	 * Sets the number of frames for the encoder to allocate for asynchronous output handling
	 * 
	 * Note: only used if AsyncSupported is true
	 * 
	 * This function cannot be called after initialization
	 * 
	 * @param numFrames The number of output frames to allocate
	 */
	virtual void SetMaxAsyncFrames(uint8_t numFrames);

	/**
	 * Gets the target bitrate for the encoder
	 */
	virtual uint32_t GetBitRate() const noexcept;
	/**
	 * Gets the sampling rate this encoder is configured to handle
	 */
	virtual uint32_t GetSampleRate() const noexcept;
	/**
	 * Gets the number of channels this encoder is configured to handle
	 */
	virtual uint32_t GetNumChannels() const noexcept;
	/**
	 * Gets the number of output frames that this encoder has allocated
	 */
	virtual uint8_t GetMaxAsyncFrames() const noexcept;

	/** 
	 * Validates configuration and initializes the audio encoder
	 * 
	 * @returns 0 on a successful init, any other value indicates an error
	 */
	virtual int Init() = 0;

	/**
	 * Returns the underlying input buffer for the given channel.
	 *
	 * Note that for non-planar input formats, all channels will
	 * point to one interleaved buffer
	 *
	 * This is useful for minimizing use of memcpy
	 *
	 * This function cannot be called before the encoder is initialized
	 * 
	 * @param channelIx The index of the channel to select the buffer for
	 * @returns The underlying buffer for the channel, or nullptr if channel does not exist
	 */
	virtual uint8_t* GetInputBuffer(uint8_t channelIx) const = 0;
	/**
	 * Notify the encoder that there is fresh data in the input buffer
	 *
	 * This function cannot be called before the encoder is initialized
	 * 
	 * @param numSamples The number of new samples in the buffer
	 * returns The updated number of samples waiting to be encoded
	 */
	virtual size_t NotifyNewDataInBuffer(size_t numSamples) = 0;
	/**
	 * Returns the number of pending samples in the encoder input queue
	 * 
	 * This value should always be less than GetInputBufferSizeSamples()
	 *
	 * This function cannot be called before the encoder is initialized
	 */
	virtual size_t GetPendingSampleCount() const = 0;

	/**
	 * Returns the size of the input buffers in bytes
	 * 
	 * For planar data, this returns the size of each individual buffer
	 * For interleaved data, this returns the size of the entire buffer
	 *
	 * This function cannot be called before the encoder is initialized
	 */
	virtual size_t GetInputBufferSizeBytes() const = 0;
	/**
	 * Returns the size of the input buffers in samples
	 *
	 * For interleaved data, this will be GetSamplesPerInputFrame() * NumChannels
	 * For planar data, this will return the same result as GetSamplesPerInputFrame()
	 *
	 * This function cannot be called before the encoder is initialized
	 */
	virtual size_t GetInputBufferSizeSamples() const = 0;
	/**
	 * Gets the number of samples per input frame, for a single channel
	 *
	 * This function cannot be called before the encoder is initialized
	 */
	virtual size_t GetSamplesPerInputFrame() const = 0;

	/*
	 * Feeds input data to the encoder, in either synchronous or asynchronous mode
	 * 
	 * Note that asynchronous mode may result in dropped frames in the event that the
	 * polling thread cannot keep up with the encoding thread!
	 * 
	 * To encode data that has been populated externally, call EncodeFrame(nullptr, 0);
	 * 
	 * Data will be returned via the DataCallback
	 *
	 * This function cannot be called before the encoder is initialized
	 * 
	 * @param data        If not nullptr, this data will be copied to the internal buffer, if null, existing buffer data will be used
	 * @param len         The length of the data to pass to the encoder, in bytes
	 * @param synchronous True if the encoder should return results immediately, false to queue results into buffer for GetEncodedResults
	 */
	virtual void EncodeFrame(const uint8_t** data, size_t len, bool async = false) = 0;

	/**
	 * Gets any encoded frames from the output, for use in async mode
	 * 
	 * Data will be returned via the DataCallback
	 *
	 * This function cannot be called before the encoder is initialized
	 */
	virtual void GetEncodedResults() = 0;

	/**
	 * Flushes any remaining data from this encoder to finish an audio stream
	 * 
	 * Data will be returned via the DataCallback in sync mode, or enqueued in async mode
	 *
	 * This function cannot be called before the encoder is initialized
	 * 
	 * @param synchronous True if the encoder should return results immediately, false to queue results into buffer for GetEncodedResults
	 */
	virtual void Flush(bool async = false) = 0;

protected:
	// Protected CTOR
	IAudioEncoder(EncoderConfig config = EncoderConfig());;

	// The encoder's common configuration parameters
	EncoderConfig _config;

	// The callback to invoke when data can be processed
	SyncDataCallback _dataCallback;

	// A pointer to the encoder's context, used to tell if initialized or not
	void*           _rawContext;
	// Mutex for locking encoder's input buffers
	std::mutex      _inputBufferLock;
	// Mutex for locking encoder's output buffers
	std::mutex      _outputBufferLock;
};