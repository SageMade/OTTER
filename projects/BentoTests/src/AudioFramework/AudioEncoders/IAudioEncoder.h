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
	uint8_t  MaxAsyncFrames = 1;
	bool     AsyncSupport   = false;
};

/// <summary>
/// Base class for encoders which convert an input stream of data to an encoded format
/// for later decoding (ex: AAC, FLAC)
/// </summary>
class IAudioEncoder {
public:
	/// <summary>
	/// Typedef for a function that can handle a resultant frame from an encode
	/// </summary>
	typedef std::function<void(EncoderResult*)> SyncDataCallback;

	virtual ~IAudioEncoder() = default;

	// Delete move and copy operators
	IAudioEncoder(const IAudioEncoder& other) = delete;
	IAudioEncoder(IAudioEncoder&& other) = delete;
	IAudioEncoder& operator=(const IAudioEncoder& other) = delete;
	IAudioEncoder& operator=(IAudioEncoder&& other) = delete;

	/// <summary>
	/// Gets the Encoding Format that this encoder can output
	/// </summary>
	virtual EncodingFormat GetEncoderFormat() const = 0;
	/// <summary>
	/// Returns the input format that this encoding schema requires
	/// </summary>
	virtual SampleFormat GetInputFormat() const = 0;

	/// <summary>
	/// Sets all basic encoder config settings, overriding existing ones
	/// Cannot be called after initialization
	/// </summary>
	/// <param name="config">The new config to override the existing one with</param>
	virtual void SetConfig(const EncoderConfig& config);
	/// <summary>
	/// Returns the current config of the audio encoder
	/// </summary>
	virtual const EncoderConfig& GetConfig() const;

	/// <summary>
	/// Gets the mutex to lock this audio encoder's input buffer
	/// </summary>
	virtual std::mutex& GetInputBufferLock() { return _inputBufferLock; }

	/// <summary>
	/// Sets the data callback to use for this encoder, this function will receive
	/// encoded data frames when ready. Note that only a single data callback may
	/// be bound to an encoder at a time
	/// </summary>
	/// <param name="callback">The callback to bind</param>
	virtual void SetDataCallback(SyncDataCallback callback) { _dataCallback = callback; }

	/// <summary>
	/// Sets the output bitrate of this encoder
	/// Cannot be called after initialization
	/// </summary>
	/// <param name="bitrate">The bitrate for the encoder to use</param>
	virtual void SetBitRate(uint32_t bitrate);
	/// <summary>
	/// Sets the sampling rate for input to this encoder
	/// Cannot be called after initialization
	/// </summary>
	/// <param name="sampleRate">The sample rate for the encoder to use</param>
	virtual void SetSampleRate(uint32_t sampleRate);
	/// <summary>
	/// Sets the number of channels that this encoder is handling
	/// Cannot be called after initialization
	/// </summary>
	/// <param name="numChannels">The number of channels for the encoder to use</param>
	virtual void SetNumChannels(uint8_t numChannels);
	/// <summary>
	/// Sets the number of frames for the encoder to allocate for
	/// asynchronous output handling, only used if AsyncSupported is true
	/// Cannot be called after initialization
	/// </summary>
	/// <param name="numFrames">The output frames for the encoder to allocate</param>
	virtual void SetMaxAsyncFrames(uint8_t numFrames);
	/// <summary>
	/// Set whether asynchronous support should be enabled for this encoder, if true,
	/// a rotary buffer of data frames will be allocated for use by a separate data
	/// handling thread
	/// </summary>
	/// <param name="isSupported">True if async encode/decode is supported, false if not</param>
	virtual void SetAsyncSupported(bool isSupported);

	/// <summary>
	/// Gets the bitrate this encoder is configured to handle
	/// </summary>
	virtual uint32_t GetBitRate() const;
	/// <summary>
	/// Gets the sampling rate this encoder is configured to handle
	/// </summary>
	virtual uint32_t GetSampleRate() const;
	/// <summary>
	/// Gets the number of channels this encoder is configured to handle
	/// </summary>
	virtual uint32_t GetNumChannels() const;
	/// <summary>
	/// Gets the number of maximum async frames this encoder is configured to handle
	/// </summary>
	virtual uint8_t GetMaxAsyncFrames() const;
	/// <summary>
	/// Gets whether this encoder is configured for asynchronous read/write
	/// </summary>
	virtual bool GetAsyncSupported() const;

	/// <summary>
	/// Validates configuration and initializes the audio encoder
	/// </summary>
	/// <returns>0 on a sucessful init, any other value indicates an error</returns>
	virtual int Init() = 0;

	/// <summary>
	/// Returns the underlying input buffer for the given channel.
	/// Note that for non-planar input formats, all channels will
	/// point to one interleaved buffer
	/// 
	/// This is useful for minimizing use of memcpy
	/// </summary>
	/// <param name="channelIx">The index of the channel to select the buffer for</param>
	/// <returns>The underlying buffer for the channel, or nullptr if channel does not exist</returns>
	virtual uint8_t* GetInputBuffer(uint8_t channelIx) const = 0;
	/// <summary>
	/// Notify the encoder that there is fresh data in the input buffer
	/// </summary>
	/// <param name="numSamples">The number of new samples in the buffer</param>
	/// <returns>The updated number of samples waiting to be encoded</returns>
	virtual size_t NotifyNewDataInBuffer(size_t numSamples) = 0;
	/// <summary>
	/// Returns the number of pending samples in the encoder input queue
	/// </summary>
	virtual size_t GetPendingSampleCount() const = 0;

	/// <summary>
	/// Returns the size of the input buffers in bytes
	/// For planar data, this returns the size of each individual buffer
	/// For interleaved data, this returns the size of the entire buffer
	/// </summary>
	virtual size_t GetInputBufferSizeBytes() const = 0;
	/// <summary>
	/// Returns the size of the input buffers in samples, returns the
	/// same value for planar and interleaved buffers
	/// </summary>
	virtual size_t GetInputBufferSizeSamples() const = 0;
	/// <summary>
	/// Gets the number of samples per input frame, for a single channel
	/// </summary>
	virtual size_t GetSamplesPerInputFrame() const = 0;

	/// <summary>
	/// Performs encoding on the input data, in either synchronous or asynchronous mode
	/// Note that asynchronous mode may result in dropped frames in the event that the
	/// polling thread cannot keep up with the encoding thread!
	/// 
	/// To encode data that has been populated externally, call EncodeFrame(nullptr, 0);
	/// 
	/// Data will be returned via the DataCallback
	/// </summary>
	/// <param name="data">If not nullptr, this data will be copied to the internal buffer, if null, existing buffer data will be used</param>
	/// <param name="len">The length of the data to pass to the encoder, in bytes</param>
	/// <param name="synchronous">True if the encoder should return results immediately, false to queue results into buffer for GetEncodedResults</param>
	/// <threadsafety instance="true"/>
	virtual void EncodeFrame(const uint8_t** data, size_t len, bool async = false) = 0;

	/// <summary>
	/// Gets any encoded frames from the output, for use in async mode
	/// 
	/// Data will be returned via the DataCallback
	/// </summary>
	/// <threadsafety instance="true"/>
	virtual void GetEncodedResults() = 0;

	/// <summary>
	/// Flushes any remaining data from this encoder to finish an audio stream
	/// 
	/// Data will be returned via the DataCallback in sync mode, or enqued in async mode
	/// </summary>
	/// <param name="synchronous">True if the encoder should return results immediately, false to queue results into buffer for GetEncodedResults</param>
	virtual void Flush(bool async = false) = 0;


protected:
	IAudioEncoder(EncoderConfig config = EncoderConfig()) : 
		_config(config), 
		_rawContext(nullptr),
		_inputBufferLock(std::mutex()),
		_outputBufferLock(std::mutex()),
		_dataCallback(nullptr) { };

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