#include <AudioFramework/AudioEncoders/AacEncoder.h>
#include <queue>
#include <aacenc_lib.h>
#include <Logging.h>

#include "AudioFramework/BufferFiller.h"
#include "FDK_audio.h" // for TRANSPORT_TYPE

/// <summary>
/// Gets an error string for a given AACENC_Error
/// </summary>
/// <param name="err"></param>
/// <returns></returns>
inline std::string GetErrorString(AACENC_ERROR err) {
	switch (err) {
		case AACENC_ERROR::AACENC_OK:                    return "OK";
		case AACENC_ERROR::AACENC_INVALID_HANDLE:        return "Handle passed to function call was invalid";
		case AACENC_ERROR::AACENC_MEMORY_ERROR:          return "Memory allocation failed";
		case AACENC_ERROR::AACENC_UNSUPPORTED_PARAMETER: return "Parameter not available";
		case AACENC_ERROR::AACENC_INVALID_CONFIG:        return "Config is invalid or not provided";
		case AACENC_ERROR::AACENC_INIT_ERROR:            return "General initialization error";
		case AACENC_ERROR::AACENC_INIT_AAC_ERROR:        return "AAC library initialization error";
		case AACENC_ERROR::AACENC_INIT_SBR_ERROR:        return "SBR library initialization error";
		case AACENC_ERROR::AACENC_INIT_TP_ERROR:         return "Transport library initialization error";
		case AACENC_ERROR::AACENC_INIT_META_ERROR:       return "Meta data library initialization error";
		case AACENC_ERROR::AACENC_INIT_MPS_ERROR:        return "MPS library initialization error";
		case AACENC_ERROR::AACENC_ENCODE_ERROR:          return "The encoding process was interrupted by an unexpected error";
		case AACENC_ERROR::AACENC_ENCODE_EOF:            return "End of file reached";
		default:
			return "Unknown error code!";
	}
}

/// <summary>
/// Maps a channel count to a channel mode for the FDK lib
/// </summary>
/// <param name="numChannels">The number of channels</param>
/// <returns>A CHANNEL_MODE for the number of channels</returns>
inline CHANNEL_MODE MapChannelMode(int numChannels) {
	switch (numChannels) {
		case 1: return  CHANNEL_MODE::MODE_1;
		case 2: return  CHANNEL_MODE::MODE_2;
		case 3: return  CHANNEL_MODE::MODE_1_2;
		case 4: return  CHANNEL_MODE::MODE_1_2_1;
		case 5: return  CHANNEL_MODE::MODE_1_2_2;
		case 6: return  CHANNEL_MODE::MODE_1_2_2_1;
		case 7: return  CHANNEL_MODE::MODE_6_1;
		case 8: return  CHANNEL_MODE::MODE_7_1_BACK;
		default: return CHANNEL_MODE::MODE_INVALID;
	}
}

#define AACENC_CALL(x) {AACENC_ERROR err = (x); if (err != AACENC_OK && err != AACENC_ENCODE_EOF) { throw std::runtime_error(GetErrorString(err)); }}
#define AACENC_CHECK(x) {if (x != AACENC_OK && x != AACENC_ENCODE_EOF) { throw std::runtime_error(GetErrorString(err)); }}

// This constant is derived from the FDK documentation for recommended output buffer size
#define BUFF_SIZE_PER_CHANNEL 6144u

/// <summary>
/// The state parameters for the encoder once it has been initialized
/// </summary>
struct AacEncoder::AacEncoderContext {
	// The FDK encoder pointer
	AACENCODER*     Encoder; 
	
	// Pointer to the input data buffer
	INT_PCM*        InputBuffer; 
	// Size of InputBuffer in bytes
	size_t          InputBufferSizeBytes; 

	// Pointer to the output data buffer
	uint8_t*        OutputBuffer;
	// Size of OutputBuffer in bytes
	size_t          OutputBufferSizeBytes;
	// Stores the output result for synchronous access
	EncoderResult   OutputResult;

	// Config parameters for InputBufferDesc
	void* InBufferConfig[1];
	int   InBufferConfigIds[1];
	int   InBufferConfigSizes[1];
	int   InBufferConfigSizesEl[1];

	// Config parameters for OutputBufferDesc
	void* OutBufferConfig[1];
	int   OutBufferConfigIds[1];
	int   OutBufferConfigSizes[1];
	int   OutBufferConfigSizesEl[1];

	// Describes the input buffers
	AACENC_BufDesc InputBufferDesc;
	// Describes the output buffers
	AACENC_BufDesc OutputBufferDesc;

	// Information about the encoder, extracted after init
	AACENC_InfoStruct EncoderInfo;

	// The number of samples pending encoding
	size_t            PendingSamples;

	// The buffer filler that assists in managing InputBuffer
	BufferFiller*     InputFiller;

	// The total number of samples that this encoder has handled
	size_t            TotalSamplesEncoded;


	///////////////////////////////////////////
	//            Aysnc properties           //
	///////////////////////////////////////////

	// How many output buffers we have allocated (for async encode/read)
	uint32_t        NumOutputBuffers;
	// The *megabuffer* that stores all allocated buffer space for async read
	uint8_t*        AsyncOutputBuffer;

	// Stores the list of results that have been allocated
	EncoderResult*  AsyncResultPool;

	uint32_t        CurrentReadTarget = 0;
	uint32_t        CurrentOutputTarget = 0;
};

AacEncoder::AacEncoder() : 
	IAudioEncoder(), 
	_context(nullptr), 
	_aacConfig(AacConfig()) 
{
	_aacConfig.TransportType       = AacTransportType::ADTS;
	_aacConfig.UseAfterburner      = false;
	_aacConfig.IsCrcEnabled        = false;
	_aacConfig.InputSampleCapacity = 0; // Capacity is determined by the encoder
	_aacConfig.MediaType           = AacMediaType::LC; // Default to low complexity for max portability
}

AacEncoder::~AacEncoder() { 
	if (_context != nullptr) {
		aacEncClose(&_context->Encoder);

		delete[] _context->InputBuffer;
		delete[] _context->OutputBuffer;

		if (_context->AsyncResultPool != nullptr) { delete[] _context->AsyncResultPool; }
		if (_context->AsyncOutputBuffer != nullptr) { delete[] _context->AsyncOutputBuffer; }

		delete _context->InputFiller;

		delete _context;
		_context = nullptr;
		_rawContext = nullptr;
	}
}

EncodingFormat AacEncoder::GetEncoderFormat() const noexcept   { 
	return EncodingFormat::AAC; 
}

SampleFormat AacEncoder::GetInputFormat() const noexcept  { 
	return SampleFormat::PCM; 
}

bool AacEncoder::GetAsyncSupported() const noexcept {
	return false;
}

void AacEncoder::SetAfterburnerEnabled(bool isEnabled) {
	LOG_ASSERT(_context == nullptr, "Cannot modify encoder parameters after initialization");
	_aacConfig.UseAfterburner = isEnabled;
}

void AacEncoder::SetTransportType(AacTransportType type) {
	LOG_ASSERT(_context == nullptr, "Cannot modify encoder parameters after initialization");
	_aacConfig.TransportType = type;
}

void AacEncoder::SetCrcEnabled(bool isEnabled) {
	LOG_ASSERT(_context == nullptr, "Cannot modify encoder parameters after initialization");
	_aacConfig.IsCrcEnabled = isEnabled;
}

void AacEncoder::SetAOT(AacMediaType mediaType) {
	LOG_ASSERT(_context == nullptr, "Cannot modify encoder parameters after initialization");
	_aacConfig.MediaType = mediaType;
}

uint32_t AacEncoder::GetInputSampleCapacity() const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return _aacConfig.InputSampleCapacity;
}

size_t AacEncoder::GetSamplesPerInputFrame() const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return _aacConfig.InputSampleCapacity;
}

int AacEncoder::Init() {
	// Ensure that we have not already initialized this encoder
	LOG_ASSERT(_context == nullptr, "This encoder has already been initialized!");
	_context = new AacEncoderContext();
	_rawContext = _context;
	// Zero out the encoder context to ensure we start from a clean slate
	memset(_context, 0, sizeof(AacEncoderContext));

	// Open the audio encoder 
	AACENC_CALL(aacEncOpen(&_context->Encoder, 0, _config.NumChannels));

	// Set our general use encoder settings
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_SAMPLERATE,     _config.SampleRate));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_CHANNELMODE,     MapChannelMode(_config.NumChannels)));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_BITRATE,        _config.BitRate));

	// Set our AAC-specific encoding settings
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_AOT,            (uint32_t)_aacConfig.MediaType));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_TRANSMUX,       (uint32_t)_aacConfig.TransportType));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_AFTERBURNER,    _aacConfig.UseAfterburner));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_PROTECTION,     _aacConfig.IsCrcEnabled));
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_SIGNALING_MODE, 0)); // Default (implicit backwards compat)
	AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_CHANNELORDER,   1)); // use WAV channel ordering (L, R, C...)

	if (_config.NumChannels == 2) {
		if (_aacConfig.MediaType != AacMediaType::Main && _aacConfig.MediaType != AacMediaType::LC) {
			AACENC_CALL(aacEncoder_SetParam(_context->Encoder, AACENC_BITRATEMODE, 3));
		}
	}

	// We call an encode with nulls to initialize the encoder 
	AACENC_CALL(aacEncEncode(_context->Encoder, nullptr, nullptr, nullptr, nullptr));

	// Get our encoding info, we can use this to introspect the encoder, store in headers, etc...
	AACENC_CALL(aacEncInfo(_context->Encoder, &_context->EncoderInfo));

	// Get the number of samples we need to feed in to fill a frame
	_aacConfig.InputSampleCapacity = _context->EncoderInfo.frameLength * _context->EncoderInfo.inputChannels;

	// Set up the input and output buffer descriptors
	_ConfigureInOutBuffers();

	return 0;
}

uint8_t* AacEncoder::GetInputBuffer(uint8_t channelIx) const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return reinterpret_cast<uint8_t*>(_context->InputBuffer);
}

size_t AacEncoder::NotifyNewDataInBuffer(size_t numSamples) {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	_context->PendingSamples += numSamples;
	return _context->PendingSamples;
}

size_t AacEncoder::GetPendingSampleCount() const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return _context->PendingSamples;
}

size_t AacEncoder::GetInputBufferSizeBytes() const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return _context->InputBufferSizeBytes;
}

size_t AacEncoder::GetInputBufferSizeSamples() const {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	return _aacConfig.InputSampleCapacity * _config.NumChannels;
}

void AacEncoder::EncodeFrame(const uint8_t** data, size_t numBytes, bool async /*= false*/) {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");

	// Feed data to the input buffers, invoking the handler on buffer full
	_context->InputFiller->FeedData(data, numBytes, [&](uint8_t** dataPlanes, size_t buffSize) {
		// Input arguments to encoder
		AACENC_InArgs inArgs = AACENC_InArgs();
		
		// Pass entire frames worth every time
		inArgs.numInSamples = _context->InputFiller->Size() / sizeof(INT_PCM);
		inArgs.numAncBytes = 0;

		// Output arguments to encoder
		AACENC_OutArgs outArgs = AACENC_OutArgs();
		memset(&outArgs, 0, sizeof(AACENC_OutArgs));

		// Keep track of encoder state
		AACENC_ERROR err = AACENC_ERROR::AACENC_OK;

		// Enter critical section for output buffer
		_outputBufferLock.lock();
		do {
			// Enter critical section for input buffer
			_inputBufferLock.lock();

			// Encode the frame
			err = aacEncEncode(_context->Encoder, &_context->InputBufferDesc, &_context->OutputBufferDesc, &inArgs, &outArgs);

			// Shift input data depending on how many samples the encoder has processed
			memmove(_context->InputBuffer, _context->InputBuffer + outArgs.numInSamples, sizeof(INT_PCM) * (inArgs.numInSamples - outArgs.numInSamples));
			inArgs.numInSamples -= outArgs.numInSamples;

			// exit critical section for input buffer
			_inputBufferLock.unlock();

			// Track our total # of samples encoded, we can use this to skip over the encoder delay
			_context->TotalSamplesEncoded += outArgs.numInSamples;

			_HandleOutputDataFrame(async, outArgs.numInSamples, outArgs.numOutBytes);

		}
		// Run the encoder as long as it is spitting out data frames
		while (err == AACENC_ERROR::AACENC_OK && outArgs.numOutBytes > 0);

		// exit critical section for output buffer
		_outputBufferLock.unlock();
	});
	_context->PendingSamples = _context->InputFiller->InternalBufferOffset();
}

void AacEncoder::GetEncodedResults() {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");
	// TODO
}

void AacEncoder::Flush(bool async /*= false*/) {
	LOG_ASSERT(_context != nullptr, "This encoder has not been initialized!");

	// Flush the input buffer and encode all frames
	_context->InputFiller->Flush();
	EncodeFrame(nullptr, 0);

	AACENC_InArgs inArgs;
	inArgs.numInSamples = -1;
	inArgs.numAncBytes = 0;

	AACENC_OutArgs outArgs;
	outArgs.numOutBytes = 0;

	AACENC_ERROR err = AACENC_ERROR::AACENC_OK;

	// Enter output buffer critical section
	_outputBufferLock.lock();
	do {
		_inputBufferLock.lock();
		err = aacEncEncode(_context->Encoder, &_context->InputBufferDesc, &_context->OutputBufferDesc, &inArgs, &outArgs);
		_inputBufferLock.unlock();

		_HandleOutputDataFrame(async, outArgs.numInSamples, outArgs.numOutBytes);

	} while (err == AACENC_ERROR::AACENC_OK && outArgs.numOutBytes > 0);

	// exit output buffer critical section
	_outputBufferLock.unlock();
}

void AacEncoder::_ConfigureInOutBuffers() {
	// Allocate memory for the input buffers and track some sizes
	_context->InputBuffer = new INT_PCM[(size_t)_config.NumChannels * _aacConfig.InputSampleCapacity];
	_context->InputBufferSizeBytes = (size_t)_config.NumChannels * _aacConfig.InputSampleCapacity * sizeof(INT_PCM);
	memset(_context->InputBuffer, 0, _context->InputBufferSizeBytes);

	// Allocate memory for output
	_context->OutputBufferSizeBytes = (size_t)_config.NumChannels * BUFF_SIZE_PER_CHANNEL;
	_context->OutputBuffer = new uint8_t[_context->OutputBufferSizeBytes];

	// Determine how many output buffers to allocate based on async settings
	_context->NumOutputBuffers = GetAsyncSupported() ? _config.MaxAsyncFrames : 0;

	// If we support async decoding, we need a few extra output frames. Note an optimization would be to actually pool these in one big 'ol 
	// memory pool
	if (GetAsyncSupported() && _config.MaxAsyncFrames > 0) {
		_context->AsyncResultPool = new EncoderResult[_config.MaxAsyncFrames];
		_context->AsyncOutputBuffer = new uint8_t[(size_t)_context->EncoderInfo.maxOutBufBytes * _config.MaxAsyncFrames];
		for (int ix = 1; ix < _context->NumOutputBuffers; ix++) {
			_context->AsyncResultPool[ix].Data = _context->AsyncOutputBuffer + ((ix - 1) * _context->EncoderInfo.maxOutBufBytes);
		}
	}

	// Configure the input buffers
	_context->InBufferConfig[0]         = _context->InputBuffer;
	_context->InBufferConfigIds[0]      = IN_AUDIO_DATA;
	_context->InBufferConfigSizes[0]    = _context->InputBufferSizeBytes;
	_context->InBufferConfigSizesEl[0]  = sizeof(INT_PCM);

	// Configure the output buffers
	_context->OutBufferConfig[0]        = _context->OutputBuffer;
	_context->OutBufferConfigIds[0]     = OUT_BITSTREAM_DATA;
	_context->OutBufferConfigSizes[0]   = _config.NumChannels * BUFF_SIZE_PER_CHANNEL;
	_context->OutBufferConfigSizesEl[0] = sizeof(uint8_t);

	// Configure input buffer description
	_context->InputBufferDesc.numBufs           = 1;
	_context->InputBufferDesc.bufs              = (void**)&_context->InBufferConfig;
	_context->InputBufferDesc.bufferIdentifiers = _context->InBufferConfigIds;
	_context->InputBufferDesc.bufSizes          = _context->InBufferConfigSizes;
	_context->InputBufferDesc.bufElSizes        = _context->InBufferConfigSizesEl;
	
	// Configure output buffer description
	_context->OutputBufferDesc.numBufs           = 1;
	_context->OutputBufferDesc.bufs              = (void**)&_context->OutBufferConfig;
	_context->OutputBufferDesc.bufferIdentifiers = _context->OutBufferConfigIds;
	_context->OutputBufferDesc.bufSizes          = _context->OutBufferConfigSizes;
	_context->OutputBufferDesc.bufElSizes        = _context->OutBufferConfigSizesEl;

	// Allocate a buffer filler for manipulating the underlying input buffer
	uint8_t* buffs[] = {
		reinterpret_cast<uint8_t*>(_context->InputBuffer)
	};
	_context->InputFiller = new BufferFiller(buffs, 1, _context->InputBufferSizeBytes);
}

void AacEncoder::_HandleOutputDataFrame(bool async, int numInSamples, int numOutBytes) {
	// If we have data and have passed the encoder startup delay
	if (_context->TotalSamplesEncoded > _context->EncoderInfo.nDelay && numOutBytes > 0) {
		if (async) {
			LOG_ASSERT(false, "Not implemented!");
		} else {
			// Get a result, load it full of the output data
			EncoderResult* result = &_context->OutputResult;
			result->Data = _context->OutputBuffer;
			result->DataSize = numOutBytes;
			result->Duration = numInSamples / _config.NumChannels;

			// invoke callback on same thread with data
			if (_dataCallback != nullptr) {
				_dataCallback(result);
			}
		}
	}
}
