#include <Bento4C.h>
#include <Ap4.h>
#include <string>
#include <iostream>
#include <math.h>
#include <unordered_map>
#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavutil/mathematics.h>
	#include <libavformat/avformat.h>
	#include <libswresample/swresample.h>
	#include <libavutil/opt.h>
	#include <libavutil/log.h>
	#include <libavutil/error.h>
	#include <libavutil/channel_layout.h>
}

#include "CustomAtom.h"
#include "ConsoleUtils.h"
#include "BufferFiller.h"
#include <BinaryWriter.h>
#include <BinaryFileWriter.h>
#include <Windows/WaveIn.h>
#include <AudioFramework/IAudioCapDeviceEnumerator.h>
#include <AudioFramework/Platform/Windows/WinAudioCapDeviceEnumerator.h>
#include "AudioFramework/IAudioPlatform.h"
#include "AudioFramework/Platform/Windows/WinAudioPlatform.h"
#include "Logging.h"
#include <conio.h>
#include "AudioFramework/Resampler.h"
#include "AudioFramework/AudioEncoder.h"

#define VALIDATE_ALLOC(param, message, returnCode) if (param == nullptr) { std::cout << message << std::endl; return returnCode; }
#define VALIDATE_FFMPEG(ret, message, returnCode) if (ret < 0) { std::cout << message << std::endl; return returnCode; }

std::unordered_map<SampleFormat, AVSampleFormat> InOutFormatMap{
	{SampleFormat::Float, AV_SAMPLE_FMT_FLT},
	{SampleFormat::PCM,   AV_SAMPLE_FMT_S16},
};

static int select_sample_rate(const AVCodec* codec, int targetSampleRate = 44100)
{
	const int* p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return targetSampleRate;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(targetSampleRate - *p) < abs(targetSampleRate - best_samplerate))
			best_samplerate = *p;
		p++;
	}
	return best_samplerate;
}

int FindAdtsSampleIndex(int sample_rate) {
	for (int ix = 0; ix < 16; ix++) {
		if (AP4_AdtsSamplingFrequencyTable[ix] == sample_rate)
			return ix;
	}
	return -1;
}

int sample_description_index = 0;
int sampling_frequency_index = 4;
int total_duration = 0;
int output_channels = 2;
AP4_SyntheticSampleTable* sample_table = nullptr;
BinaryFileWriter* manualOut = nullptr;

/// <summary>
/// An ADTS Header for storing AAC frames, without a CRC check
/// </summary>
/// <see>https://wiki.multimedia.cx/index.php/ADTS</see>
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif // WINDOWS
struct AdtsHeader {
	inline static uint8_t AAC_MAIN = 0;
	inline static uint8_t AAC_LC   = 1;
	inline static uint8_t AAC_SSR  = 2;
	inline static uint8_t AAC_LTP  = 3;

	uint8_t Bytes[7];

	AdtsHeader(){ 
		Bytes[0] = 0b11111111;
		Bytes[1] = 0b11110001;
		Bytes[2] = 0b00010000;
		Bytes[3] = 0b10000000;
		Bytes[4] = 0b00000000;
		Bytes[5] = 0b00011111;
		Bytes[6] = 0b11111100;
	}

	void SetUseMpeg2(bool useMpeg2) {
		Bytes[1] = (Bytes[1] & 0b11110111) | ((useMpeg2 & 0x01) << 3);
	}

	void SetAacProfile(uint8_t profile) {
		LOG_ASSERT(profile < 4, "Profile MUST be one of the available AAC profiles");
		Bytes[2] = (Bytes[2] & 0b00111111) | ((profile & 0b11) << 6);
	}

	void SetSampleFreqIndex(uint8_t index) {
		LOG_ASSERT(index < 15, "Index not valid");
		Bytes[2] = (Bytes[2] & 0b11000011) | ((index & 0b1111) << 2);
	}

	void SetChannelConfig(uint8_t config) {
		LOG_ASSERT(config < 8, "Not a valid channel count!");
		Bytes[2] = (Bytes[2] & 0b11111110) | ((config & 0b0111) >> 2);
		Bytes[3] = (Bytes[3] & 0b00111111) | ((config & 0b0111) << 6);
	}

	void SetFrameLength(uint16_t length) {
		LOG_ASSERT(length < 8184, "Frame to large!");
		uint16_t totalLen = length + 7;
		Bytes[3] = (Bytes[3] & 0b11111110) | (totalLen >> 11);
		Bytes[4] = (totalLen >> 3) & 0xFF;
		Bytes[5] = (Bytes[5] & 0b00011111) | (totalLen << 5);
	}

#ifdef _MSC_VER
};
#pragma pack(pop)
#else 
} __attribute__((__packed__));
#endif // WINDOWS
;

void StoreSampleData(uint8_t* data, size_t len, AP4_SyntheticSampleTable* table, int numInputSamples, uint8_t sample_description_index, uint8_t numChannels, bool writeHeader = true)  {
	AP4_MemoryByteStream* sample_data = new AP4_MemoryByteStream(len + (writeHeader ? 7 : 0));

	if (writeHeader) {
		// TODO: This can be cached
		AdtsHeader header      = AdtsHeader();
		header.SetSampleFreqIndex(sampling_frequency_index);
		header.SetChannelConfig(numChannels);
		header.SetFrameLength(len);

		sample_data->Write(header.Bytes, 7);
	}
	memcpy(sample_data->UseData() + (writeHeader ? 7 : 0), data, len);
	table->AddSample(*sample_data, 0, len + (writeHeader ? 7 : 0), numInputSamples, sample_description_index, 0, 0, true);
	sample_data->Release();
}

/// <summary>
/// Callback for handling raw data sent by an ffmpeg formatter
/// </summary>
/// <param name="opaque">A pointer to the user type (unused)</param>
/// <param name="data">The data that the formatter has received</param>
/// <param name="len">The length of data in bytes</param>
/// <returns>0</returns>
static int onDataReceived(void* opaque, uint8_t* data, int len) {
	manualOut->WriteBytes(data, len);
	return 0;
}

/// <summary>
/// Handles encoding a single frame of audio data
/// </summary>
/// <param name="ctx">The codex context to use for conversion</param>
/// <param name="frame">The frame to encode</param>
/// <param name="packet">The packet to store results in</param>
/// <param name="sample_table">The Bento4 SampleTable to output the encoded frame to</param>
/// <param name="outCtx">The FFMPEG formatter to output the encoded frame's data to</param>
/// <returns>0 on success, a negative error code on failure</returns>
static int encode(AVCodecContext* ctx, AVFrame* frame, AVPacket* packet, AP4_SyntheticSampleTable* sample_table, AVFormatContext* outCtx = nullptr) {
	int ret = 0;
	// Send the data frame to the encoder
	ret = avcodec_send_frame(ctx, frame);
	VALIDATE_FFMPEG(ret, "Failed to encode frame", -1);
	
	// TODO: Split this into another thread? 
	while (ret >= 0) {
		// Read a packet of data from the encoder
		ret = avcodec_receive_packet(ctx, packet);

		// Early abort if EOF
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return 0;
		} else if (ret < 0) {
			return ret;
		}

		// Store encoded data into our MP4 container sample table
		if (sample_table != nullptr) {
			StoreSampleData(packet->data, packet->size, sample_table, packet->duration, sample_description_index, ctx->channels);
		}

		// AV write frame for FFMPEG file handle if requested
		if (outCtx != nullptr) {
			ret = av_write_frame(outCtx, packet);
			VALIDATE_FFMPEG(ret, "Failed to store frame in formatter!", -2);
		}

		// Accumulate the packet's duration for our playback timer
		total_duration += packet->duration;

		// Decrement reference count now that we're done with the packet
		av_packet_unref(packet); 
	}

	// Return success
	return 0;
}

/*----------------------------------------------------------------------
|   MakeDsi (Taken from Bento4 samples)
+---------------------------------------------------------------------*/
static void MakeDsi(unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned char* dsi)
{
	unsigned int object_type = AP4_MPEG4_AUDIO_OBJECT_TYPE_AAC_MAIN; // AAC LC by default
	dsi[0] = (object_type << 3) | (sampling_frequency_index >> 1);
	dsi[1] = ((sampling_frequency_index & 1) << 7) | (channel_configuration << 3);
}

int RecordStream(const std::string& path) {
	int ret; // return codes for avcodec commands
	AVCodec* codec;
	AVCodecContext* ctx;
	AVPacket* packet;
	AVFrame* frame;

	AVIOContext* ioContext;
	AVStream* streamOut;
	AVFormatContext* formatContext;
	SwrContext* resamplingContext; 

	uint8_t* memoryStream = reinterpret_cast<uint8_t*>(av_malloc_array(4096, 1)); // Allocate memory for the AAC encoder to store data in

	av_log_set_level(AV_LOG_VERBOSE);
	avcodec_register_all();

	AP4_Result result;

	AP4_ByteStream* output = NULL;
	result = AP4_FileByteStream::Create(path.c_str(), AP4_FileByteStream::STREAM_MODE_WRITE, output);

	// create a sample table
	sample_table = new AP4_SyntheticSampleTable();

	{
		const char* aacPath = "test.aac";
		manualOut = new BinaryFileWriter("test_manual.aac");

		// Find the AAC encoder
		codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		VALIDATE_ALLOC(codec, "Failed to find codec", 1);

		// Create the context for our codec
		ctx = avcodec_alloc_context3(codec);
		VALIDATE_ALLOC(ctx, "Failed to allocate context", 2);

		ctx->sample_rate = select_sample_rate(codec); // Target 44.1k/hz by default
		ctx->bit_rate = 64000; // 64kbps
		ctx->channels = output_channels; // Mono track
		ctx->sample_fmt = AV_SAMPLE_FMT_FLTP; // planar floats |  AV_SAMPLE_FMT_S16; // Signed 16 bits (PCM)
		ctx->channel_layout = av_get_default_channel_layout(ctx->channels); // Make sure our channel layout is identified as mono

		// Our input audio stream can have a different config then encoder, since we are resampling
		AudioInStreamConfig inputConfig;
		inputConfig.Format = SampleFormat::PCM;
		inputConfig.NumChannels = 2;
		inputConfig.SampleRate = ctx->sample_rate;

		// Open the codec for conversion;
		ret = avcodec_open2(ctx, codec, nullptr);
		VALIDATE_FFMPEG(ret, "Failed to open the codec!", 3);

		// Configure Audio Resampler for converting input stream into something the encoder understands
		resamplingContext = swr_alloc();
		av_opt_set_int(resamplingContext, "in_channel_layout", av_get_default_channel_layout(inputConfig.NumChannels), 0);
		av_opt_set_int(resamplingContext, "in_sample_rate", inputConfig.SampleRate, 0);
		av_opt_set_sample_fmt(resamplingContext, "in_sample_fmt", InOutFormatMap[inputConfig.Format], 0);

		av_opt_set_int(resamplingContext, "out_channel_layout", ctx->channel_layout, 0);
		av_opt_set_int(resamplingContext, "out_sample_rate", ctx->sample_rate, 0);
		av_opt_set_sample_fmt(resamplingContext, "out_sample_fmt", ctx->sample_fmt, 0);
		swr_init(resamplingContext);

		//avio_open(&ioContext, aacPath, AVIO_FLAG_WRITE);
		ioContext = avio_alloc_context(memoryStream, 4096, 1, nullptr, NULL, onDataReceived, NULL);
		formatContext = avformat_alloc_context();
		formatContext->pb = ioContext;
		formatContext->oformat = av_guess_format(NULL, "o.aac", NULL);

		streamOut = avformat_new_stream(formatContext, codec);
		streamOut->time_base.den = ctx->sample_rate;
		streamOut->time_base.num = 1;

		avcodec_parameters_from_context(streamOut->codecpar, ctx);

		AP4_DataBuffer dsi;
		unsigned char aac_dsi[2];
		int sample_index = FindAdtsSampleIndex(ctx->sample_rate);
		MakeDsi(sample_index, 1, aac_dsi);
		dsi.SetData(aac_dsi, 2);
		AP4_MpegAudioSampleDescription* sample_description = new AP4_MpegAudioSampleDescription(
			AP4_OTI_MPEG4_AUDIO,   // object type
			ctx->sample_rate,
			16, //ctx->bits_per_coded_sample, // sample size
			ctx->channels,
			&dsi,          // decoder info
			6144,          // buffer size
			128000,        // max bitrate
			128000
		);        // average bitrate
		sample_description_index = sample_table->GetSampleDescriptionCount();
		sample_table->AddSampleDescription(sample_description);

		// Begin ze data stream
		avformat_write_header(formatContext, NULL);

		// Allocate an AV packet to store results
		packet = av_packet_alloc();
		VALIDATE_ALLOC(packet, "Failed to allocate AV packet", 4);

		// Allocate an AV frame
		frame = av_frame_alloc();
		VALIDATE_ALLOC(packet, "Failed to allocate AV frame", 5);
		frame->nb_samples = ctx->frame_size;
		frame->format = ctx->sample_fmt;
		frame->channel_layout = ctx->channel_layout;

		// Allocate the buffers within the AV frame
		ret = av_frame_get_buffer(frame, 0);
		VALIDATE_FFMPEG(ret, "Failed to allocate AV buffers", 6);

		// The packed buffer stores nSamples * sizeof(sample) * channels  bytes, stores all streams tightly packed
		size_t fullPackedInputSize = frame->nb_samples * GetSampleFormatSize(inputConfig.Format) * inputConfig.NumChannels;
		BufferFiller frameBuffer(fullPackedInputSize, 1);

		//
		uint8_t** outputBuffers = new uint8_t * [ctx->channels];
		for (int ix = 0; ix < ctx->channels; ix++) {
			outputBuffers[ix] = frame->data[ix];
		}

		TestWaveAudio(inputConfig, [&](const uint8_t* data, size_t len) {
			//StoreSampleData(data, len, sample_table, sample_count, sample_description_index, ctx->channels);
			const uint8_t* inputData[] = {
				data
			};
			frameBuffer.FeedData((const uint8_t**)inputData, len, [&](uint8_t** data, size_t len) {
				float* dataFloat = reinterpret_cast<float*>(data[0]);
				size_t numInputSamples = len / GetSampleFormatSize(inputConfig.Format) / ctx->channels;
				int outSamples = swr_convert(resamplingContext, outputBuffers, frame->nb_samples, (const uint8_t**)data, numInputSamples);
				if (outSamples != frame->nb_samples) {
					std::cout << "[WARN] Frame size mismatch!" << std::endl;
				}
				encode(ctx, frame, packet, sample_table, formatContext);
			});
		});

		// If we have un-encoded data left in the buffer, we need to flush it
		if (frameBuffer.HasData()) {
			// Flush rest of frame with zeroes
			frameBuffer.Flush();

			// Convert the buffer data
			size_t numInputSamples = frameBuffer.Size() / GetSampleFormatSize(inputConfig.Format) / ctx->channels;
			int outSamples = swr_convert(resamplingContext, outputBuffers, frame->nb_samples, (const uint8_t**)frameBuffer.DataBuffers(), numInputSamples);

			// Store in output file
			encode(ctx, frame, packet, sample_table, formatContext);
		}

		delete[] outputBuffers;

		// Flush encoder
		encode(ctx, NULL, packet, sample_table, formatContext);

		// Write footer and close file
		av_write_trailer(formatContext);
		avio_closep(&ioContext);

	}

	manualOut->Flush();
	delete manualOut;



	// create a movie
	AP4_Movie* movie = new AP4_Movie();

	// create an audio track
	AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
									 sample_table,
									 0,     // track id
									 ctx->sample_rate,  // movie time scale
									 total_duration,    // track duration              
									 ctx->sample_rate,  // media time scale
									 total_duration,    // media duration
									 "eng", // language
									 0, 0); // width, height

	// add the track to the movie
	movie->AddTrack(track);

	// create a multimedia file
	AP4_File* file = new AP4_File(movie);

	// set the file type
	AP4_UI32 compatible_brands[2] = {
		AP4_FILE_BRAND_ISOM,
		AP4_FILE_BRAND_MP42
	};
	file->SetFileType(AP4_FILE_BRAND_M4A_, 0, compatible_brands, 2);

	// write the file to the output
	AP4_FileWriter::Write(*file, *output);

	// Cleanup
	av_frame_free(&frame);
	av_packet_free(&packet);
	avcodec_free_context(&ctx);

	return 0;
}

int RecordStream2(const std::string& bentoPath, const std::string& ffmpegPath = "test_manual.aac") {
	av_log_set_level(AV_LOG_VERBOSE);
	avcodec_register_all();

	IAudioPlatform* audioPlatform = new WinAudioPlatform();
	audioPlatform->Init();

	IAudioCapDeviceEnumerator* devices = audioPlatform->GetDeviceEnumerator();
	int numDevs = devices->DeviceCount();

	for (int ix = 0; ix < numDevs; ix++) {
		std::cout << ix << ": " << devices->GetDevice(ix)->GetName() << std::endl;
	}
	int id = ConsoleUtils::OptionsMenu(numDevs, "Select Audio Device: ");

	// Get the device and display it's default format characteristics
	IAudioCapDevice* selectedDevice = devices->GetDevice(id);
	AudioInStreamConfig config = selectedDevice->GetConfig();
	// Force the stream into PCM format
	config.Format = SampleFormat::PCM;
	
	std::cout << "====== DEFAULT FORMAT ======" << std::endl;
	std::cout << "Channels: " << (int)config.NumChannels << std::endl;
	std::cout << "Sample Rate: " << (int)config.SampleRate << std::endl;
	std::cout << "Sub Format: " << GetSampleFormatName(config.Format) << std::endl;
	std::cout << "============================" << std::endl;

	// Initialize the audio device to begin recording
	selectedDevice->Init(&config);

	// Perform an initial poll to sync up the device
	selectedDevice->PollDevice([](const uint8_t** dat, size_t len) { std::cout << "hi!" << len << std::endl; });

	// Create the audio encoder
	AudioEncoder* encoder = new AudioEncoder();
	encoder->SetBitRate(128000);
	encoder->SetEncodingFormat(EncodingFormat::AAC);
	encoder->Init();

	// Create a resampling context to convert PCM to a format the
	// encoder understands
	// TODO: Only create resampler if input and output types do not match
	Resampler* resampler = new Resampler();
	resampler->MatchSourceEncoding(selectedDevice->GetConfig());
	resampler->SetInputFrameSampleCount(encoder->FrameSampleCount());
	resampler->MatchDestEncoding(encoder);
	resampler->Init();

	AP4_Result result;
	AP4_ByteStream* output = NULL;
	result = AP4_FileByteStream::Create(bentoPath.c_str(), AP4_FileByteStream::STREAM_MODE_WRITE, output);

	// create a sample table
	AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

	AP4_DataBuffer dsi;
	unsigned char aac_dsi[2];
	int sample_index = FindAdtsSampleIndex(encoder->GetSampleRate());
	MakeDsi(sample_index, encoder->GetNumChannels(), aac_dsi);
	dsi.SetData(aac_dsi, 2);
	AP4_MpegAudioSampleDescription* sample_description = new AP4_MpegAudioSampleDescription(
		AP4_OTI_MPEG4_AUDIO,   // object type
		encoder->GetSampleRate(),
		GetSampleFormatSize(encoder->GetActualSampleFormat()) * 8, //ctx->bits_per_coded_sample, // sample size
		encoder->GetNumChannels(),
		&dsi,          // decoder info
		6144,          // buffer size
		encoder->GetBitRate(),        // max bitrate
		encoder->GetBitRate()         // average bitrate
	);        
	sample_description_index = sample_table->GetSampleDescriptionCount();
	sample_table->AddSampleDescription(sample_description);


	// Attach the encoder inputs as the outputs of the resampler
	int numOutBuffers = av_sample_fmt_is_planar(ToFfmpeg(encoder->GetActualSampleFormat())) ? encoder->GetNumChannels() : 1;
	for(int ix = 0; ix < numOutBuffers; ix++) {
		resampler->SetOutputChannelPtr(ix, encoder->GetInputBufferPtr(ix));
	}

	// We need to create temp buffers to fill encoder frames
	// Calculate how many buffers we need, their size, then allocate them
	int numInBuffers = av_sample_fmt_is_planar(ToFfmpeg(resampler->GetInputConfig().Format)) ? resampler->GetInputConfig().NumChannels : 1 ;
	size_t frameByteSize = encoder->CalcFrameBufferSize(resampler->GetInputConfig().Format);
	BufferFiller* bufferFiller = new BufferFiller(frameByteSize, numInBuffers);
	// Attach the buffers from buffer filler to the inputs of the resampler
	for (int ix = 0; ix < numInBuffers; ix++) {
		resampler->SetInputChannelPtr(ix, bufferFiller->DataBuffers()[ix]);
	}

	// Function to handle when data has been encoded by the encoder
	std::function<void(EncoderResult*)> encodedCallback = [&](EncoderResult* output) {
		StoreSampleData(output->Data, output->DataSize, sample_table, output->Duration, sample_description_index, encoder->GetNumChannels());
		total_duration += output->Duration;
	};

	// Function to handle when a complete frame is ready for encoding
	std::function<void(uint8_t**, size_t)> flushBufferCallback = [&](uint8_t** data, size_t len){
		resampler->EncodeFrame();
		encoder->EncodeFrame(encodedCallback);
	};

	// Main encoding loop
	while (true) {
		selectedDevice->PollDevice([&](const uint8_t** dat, size_t len) {
			bufferFiller->FeedData(dat, len, flushBufferCallback);
		});

		if (kbhit()) {
			break;
		}
	}

	// Flush any data left in the buffer
	if (bufferFiller->HasData()) {
		bufferFiller->Flush();
		// Zero out frames 
		for (int ix = 0; ix < numOutBuffers; ix++) {
			memset(encoder->GetInputBufferPtr(ix), 0, encoder->GetFrameBufferSize());
		}
		flushBufferCallback(bufferFiller->DataBuffers(), bufferFiller->Size());
	}
	// Flush the encoder
	encoder->Flush(encodedCallback);


	// create a movie
	AP4_Movie* movie = new AP4_Movie();

	// create an audio track
	AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
									 sample_table,
									 0,     // track id
									 encoder->GetSampleRate(),  // movie time scale
									 total_duration,    // track duration              
									 encoder->GetSampleRate(),  // media time scale
									 total_duration,    // media duration
									 "eng", // language
									 0, 0); // width, height

	// add the track to the movie
	movie->AddTrack(track);

	// create a multimedia file
	AP4_File* file = new AP4_File(movie);

	// set the file type
	AP4_UI32 compatible_brands[2] = {
		AP4_FILE_BRAND_ISOM,
		AP4_FILE_BRAND_MP42
	};
	file->SetFileType(AP4_FILE_BRAND_M4A_, 0, compatible_brands, 2);

	// write the file to the output
	AP4_FileWriter::Write(*file, *output);

	delete devices;
	audioPlatform->Cleanup();
	return 0;
}

int main() {
	Logger::Init();

	int mode = ConsoleUtils::OptionsMenu({
		"Record Type 1",
		"Record Type 2",
		"Playback"
	});

	switch (mode) {
		case 0:
			RecordStream("test_old.m4a");
			break;
		case 1:
			RecordStream2("test.m4a");
			break;
	}

	Logger::Uninitialize();

	return 0;
} 
