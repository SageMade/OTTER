#include <AudioFramework/Platform/Windows/WinAudioCapDevice.h>
#include <Logging.h>
#include <AudioFramework/SampleFormat.h>

#include <locale>
#include <codecvt>

#include <dshow.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <Audioclient.h>
#include <Functiondiscoverykeys_devpkey.h> // For prop IDS????
#include <comdef.h>

inline SampleFormat GetSampleFormat(GUID format) {
	if (format == MFAudioFormat_PCM)
		return SampleFormat::PCM;
	else if (format == MFAudioFormat_Float)
		return SampleFormat::Float;
	else
		return SampleFormat::Unknown;
}
inline GUID ToMfFormat(SampleFormat format) {
	switch (format) {
		case SampleFormat::Float: return MFAudioFormat_Float;
		case SampleFormat::PCM: return MFAudioFormat_PCM;
		case SampleFormat::PlanarFloat:
			LOG_WARN("WMF Output does not support planar floats, switching to interleaved floats");
			return MFAudioFormat_Float;
		case SampleFormat::Unknown:
		default:
			return MFAudioFormat_Float; // Default to float for unknown data types
			break;
	}
}

WinAudioCapDevice::WinAudioCapDevice(IMFActivate* wmfDevice) : IAudioCapDevice(),
_attributes(wmfDevice), _device(nullptr), _reader(nullptr), _mediaType(nullptr)
{
	IMFMediaType* audioFormat = nullptr;
	WAVEFORMATEXTENSIBLE* waveFormatEX = nullptr; // The extended wave format for the recording device
	// Extract wide name from Windows
	LPWSTR name;
	uint32_t size;
	_attributes->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &size);

	// Name from pointer, convert from wide to short chars
	std::wstring wideName = name;
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	_humanReadableName = converter.to_bytes(wideName);

	// Activate the object and create a source reader so we can extract some info about the stream
	_attributes->ActivateObject(IID_PPV_ARGS(&_device));

	// Create a temporary reader (make sure it won't kill the device on disconnect
	//_attributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);
	MFCreateSourceReaderFromMediaSource(_device, _attributes, &_reader);

	// Get the native media type of the stream
	_reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, MF_SOURCE_READER_CURRENT_TYPE_INDEX, &audioFormat);

	// Get the media representation so we can pull out it's configuration
	void* rawRepresentation = nullptr;
	audioFormat->GetRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &rawRepresentation);
	AM_MEDIA_TYPE* format = reinterpret_cast<AM_MEDIA_TYPE*>(rawRepresentation);

	// Ensure base format is something we understand
	if (format->formattype == FORMAT_WaveFormatEx) {
		waveFormatEX = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format->pbFormat);
	} else {
		throw std::runtime_error("Failed to initialize audio stream! Unknown format type!");
	}

	// Copy data to default device config
	_config.Format = GetSampleFormat(waveFormatEX->SubFormat);
	_config.NumChannels = waveFormatEX->Format.nChannels;
	_config.SampleRate = waveFormatEX->Format.nSamplesPerSec;
}

WinAudioCapDevice::~WinAudioCapDevice() {
	// Release reader and device so we don't waste memory
	if (_reader != nullptr) _reader->Release();
	if (_device != nullptr) _device->Release();
	if (_attributes != nullptr) _attributes->Release();
	if (_mediaType != nullptr) _mediaType->Release();

	// Null out reader and device so we don't accidentally attempt to use them
	_reader = nullptr;
	_device = nullptr;
	_attributes = nullptr;
	_mediaType = nullptr;
}

void WinAudioCapDevice::Init(const AudioInStreamConfig* targetConfig) {
	// If we received a target config, copy paramters that have been set
	if (targetConfig != nullptr) {
		if (targetConfig->Format != SampleFormat::Unknown) {
			_config.Format = targetConfig->Format;
		}
		if (targetConfig->NumChannels != 0) {
			_config.NumChannels = targetConfig->NumChannels;
		}
		if (targetConfig->SampleRate != 0) {
			_config.SampleRate = targetConfig->SampleRate;
		}
	}

	// Create a media type for the device, pass it the configuration parameters
	MFCreateMediaType(&_mediaType);
	_mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	_mediaType->SetGUID(MF_MT_SUBTYPE, ToMfFormat(_config.Format));
	_mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, _config.NumChannels);
	_mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, _config.SampleRate);

	// Tell the audio reader to use that media type
	_reader->SetCurrentMediaType(0, nullptr, _mediaType);
}

void WinAudioCapDevice::PollDevice(DataCallback callback) {
	// Only poll if the device has been initialized
	if (_reader != nullptr) {
		IMFSample* soundSample = nullptr;
		DWORD streamIX, streamFlags;
		int64_t timestamp;
		_reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIX, &streamFlags, &timestamp, &soundSample);

		// We got data!
		if (soundSample != nullptr) {
			// Convert the samples to a contiguous buffer
			IMFMediaBuffer* buffer = nullptr;
			soundSample->ConvertToContiguousBuffer(&buffer);
			// Lock the buffer for reading
			DWORD length = 0;
			buffer->GetCurrentLength(&length);
			uint8_t* rawData = nullptr;
			buffer->Lock(&rawData, nullptr, &length);
			uint8_t* buffers[] = {
				rawData
			};
			// Invoke the callback with the data
			callback((const uint8_t**)buffers, length);
			// Unlock buffer
			buffer->Unlock();
		}
	}
}

void WinAudioCapDevice::Stop() {
	// Release reader and device so we don't waste memory
	if (_reader != nullptr) _reader->Release();
	if (_mediaType != nullptr) _mediaType->Release();

	// Null out reader and device so we don't accidentally attempt to use them
	_reader = nullptr;
	_mediaType = nullptr;
}

