#include "WaveIn.h"
#include <windows.h>
#include <mmeapi.h>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <unordered_map>
#include <conio.h>
#include <BinaryFileWriter.h>

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

// Specialize std::hash for windows GUID cause apparently MS didn't????
namespace std {
	template<> struct hash<GUID>
	{
		size_t operator()(const GUID& guid) const noexcept {
			const std::uint64_t* p = reinterpret_cast<const std::uint64_t*>(&guid);
			std::hash<std::uint64_t> hash;
			return hash(p[0]) ^ hash(p[1]);
		}
	};
}


std::unordered_map<GUID, std::string> SubTypeMap{
	{ MEDIASUBTYPE_PCM, "PCM"},
	{ MEDIASUBTYPE_MPEG1Packet, "MPEG 1 Packet"},
	{ MEDIASUBTYPE_MPEG1Payload, "MPEG 1 Payload"},
	{ MEDIASUBTYPE_IEEE_FLOAT, "IEEE Float"},
	{ MEDIASUBTYPE_DOLBY_AC3_SPDIF, "Dolby AC3 SPDIF"}
};

std::unordered_map<uint16_t, std::string> WavFormatTags{
	{ WAVE_FORMAT_EXTENSIBLE, "Wave Format"},
	{ WAVE_FORMAT_MPEG, "MPEG Format"}
};

std::unordered_map<HRESULT, std::string> InitErrorMap {
	{ S_OK, "OK"},
	{ S_FALSE, "Failed"},
	{ AUDCLNT_E_UNSUPPORTED_FORMAT, "Unsupported"},
	{ E_POINTER, "Missing Pointer"},
	{ E_INVALIDARG, "Invalid Argument"},
	{ AUDCLNT_E_DEVICE_INVALIDATED, "Invalid Device"},
	{ AUDCLNT_E_SERVICE_NOT_RUNNING, "Service not running"}
};


void TestWaveAudio(int sampleRate, std::function<void(uint8_t*, size_t)> callback)
{
	IMFMediaSource* recordingDevice = nullptr;    // The media source we recording from
	IMFAttributes* attributes = nullptr;          // Attributes of all audio recording devices (?)
	IMFActivate** devices = nullptr;              // List of all audio recording devices
	IMFSourceReader* inputReader = nullptr;       // Handler for reading input from recording devices
	IMFMediaType* audioFormat = nullptr;          // What format the recording device is using
	AM_MEDIA_TYPE* soundRepresentation = nullptr; // What representation the recording device uses for audio frames
	WAVEFORMATEXTENSIBLE* waveFormatEX = nullptr; // The extended wave format for the recording device
	IMFSample* soundSample = nullptr;             // The sound sample for a single frame

	IMFMediaType* decodingMediaType = nullptr;

	uint64_t mediaTypeIndex = 0;
	uint64_t mediaStreamIndex = 0;

	// Initialize Media Foundation
	MFStartup(MF_VERSION);

	// Create an attribute set and set it to only include audio capture devices
	MFCreateAttributes(&attributes, 4);
	attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);

	// List all devices
	UINT numDevs = 0;
	MFEnumDeviceSources(attributes, &devices, &numDevs);

	for (int ix = 0; ix < numDevs; ix++) {
		LPWSTR name;
		uint32_t size;
		devices[ix]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &size);
		std::wcout << ix << ": " << name << std::endl;
	}
	std::cout << "Enter device ID: ";
	int id = 0;
	std::cin >> id;

	// Activate the recording device for use
	devices[id]->ActivateObject(IID_PPV_ARGS(&recordingDevice));

	// Create the input reader for reading data from the device
	MFCreateSourceReaderFromMediaSource(recordingDevice, attributes, &inputReader);

	MFCreateMediaType(&decodingMediaType);
	decodingMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	decodingMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	decodingMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 1);
	decodingMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);

	inputReader->SetCurrentMediaType(mediaStreamIndex, nullptr, decodingMediaType);
	
	// Get the native audio format for the device
	inputReader->GetCurrentMediaType(mediaStreamIndex, &audioFormat);

	// Extract the info about how the data is encoded for the device
	void* rawRepresentation = nullptr;
	audioFormat->GetRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &rawRepresentation);
	soundRepresentation = reinterpret_cast<AM_MEDIA_TYPE*>(rawRepresentation);
	
	// Only input format for audio devices is wave format
	if (soundRepresentation->formattype == FORMAT_WaveFormatEx) {
		waveFormatEX = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(soundRepresentation->pbFormat);
	} else {
		return;
	}

	std::cout << "====== FORMAT ===========" << std::endl;
	std::cout << "Channels: " << waveFormatEX->Format.nChannels << std::endl;
	std::cout << "Block Alignment: " << waveFormatEX->Format.nBlockAlign << std::endl;
	std::cout << "Sample Rate: " << waveFormatEX->Format.nSamplesPerSec << std::endl;
	std::cout << "Bits per Sample: " << waveFormatEX->Format.wBitsPerSample << std::endl;
	std::cout << "Extras Size: " << waveFormatEX->Format.cbSize << std::endl;
	std::cout << "Sub Format: " << SubTypeMap[waveFormatEX->SubFormat] << std::endl;
	std::cout << "========================" << std::endl;

	std::cout << "Press a key to start recording" << std::endl;
	std::cin.ignore();
	std::cout << "Now Recording, press any key to end" << std::endl;

	DWORD streamIX, streamFlags;
	int64_t timestamp;
	char c = 0;
	while (true) {
		inputReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIX, &streamFlags, &timestamp, &soundSample);

		// We got data!
		if (soundSample != nullptr) {
			IMFMediaBuffer* buffer = nullptr;
			soundSample->ConvertToContiguousBuffer(&buffer);
			DWORD length = 0;
			buffer->GetCurrentLength(&length);
			uint8_t* rawData = nullptr;
			buffer->Lock(&rawData, nullptr, &length);
			callback(rawData, length);
			buffer->Unlock();
		}

		if (kbhit()) {
			break;
		}
	}
}

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

void TestWaveAudio2(std::function<void(uint8_t*, size_t)> callback) {
	HRESULT hr;
	REFERENCE_TIME reqDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME actualDuration;
	uint32_t bufferFrameCount;
	uint32_t numFramesAvailable;
	IMMDeviceEnumerator* deviceEnumerator = nullptr;
	IMMDeviceCollection* devices = nullptr;
	IMMDevice* device = nullptr;
	IAudioClient* audioClient = nullptr;
	IAudioCaptureClient* captureClient = nullptr;
	WAVEFORMATEX* mixFormat = nullptr;
	WAVEFORMATEXTENSIBLE* extendedFormat = nullptr;


	// Initialize Media Foundation
	hr = MFStartup(MF_VERSION);

	// Create a device enumerator to look for audio devices
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&deviceEnumerator);

	// Get all the audio capture devices that are active
	hr = deviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &devices);

	uint32_t devCount = 0;
	hr = devices->GetCount(&devCount);
	for (int ix = 0; ix < devCount; ix++) {
		IMMDevice* device = nullptr;
		IPropertyStore* propStore = nullptr;

		// Create a prop to store the name and initialize it
		PROPVARIANT name;
		PropVariantInit(&name);
		
		// Get device from collection
		hr = devices->Item(ix, &device);

		// Get the properties for the device
		hr = device->OpenPropertyStore(STGM_READ, &propStore);

		// Get and display device name
		hr = propStore->GetValue(PKEY_Device_FriendlyName, &name);
		std::wcout << ix << ": " << name.pwszVal << std::endl;
	}
	std::cout << "Enter device ID: ";
	int id = 0;
	std::cin >> id;

	// Select audio device from collection
	hr = devices->Item(id, &device);

	// Activate the audio device
	hr = device->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audioClient);

	// Get the format of the audio device
	hr = audioClient->GetMixFormat(&mixFormat);

	// Only input format for audio devices is wave format
	if (mixFormat->cbSize == 22) {
		extendedFormat = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat);
	} else {
		std::cout << "Does not use extensible wave format, aborting!" << std::endl;
		return;
	}

	// Log audio device settings
	std::cout << "====== FORMAT ===========" << std::endl;
	std::cout << "Channels: " << extendedFormat->Format.nChannels << std::endl;
	std::cout << "Block Alignment: " << extendedFormat->Format.nBlockAlign << std::endl;
	std::cout << "Sample Rate: " << extendedFormat->Format.nSamplesPerSec << std::endl;
	std::cout << "Bits per Sample: " << extendedFormat->Format.wBitsPerSample << std::endl;
	std::cout << "Sub Format: " << SubTypeMap[extendedFormat->SubFormat] << std::endl;
	std::cout << "========================" << std::endl;

	// Initialize the audio device
	hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, reqDuration, 0, mixFormat, nullptr);
	hr = audioClient->GetBufferSize(&bufferFrameCount);
	hr = audioClient->GetService(IID_IAudioCaptureClient, (void**)&captureClient);

	// TODO: Notify sink of data format

	actualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / mixFormat->nSamplesPerSec;

	hr = audioClient->Start();

	while (true) {
		uint32_t packetLen = 0;

		hr = captureClient->GetNextPacketSize(&packetLen);

		if (packetLen > 0) {
			uint8_t* buffer;
			uint32_t length = 0;
			DWORD bufferFlags = 0;

			hr = captureClient->GetBuffer(&buffer, &length, &bufferFlags, nullptr, nullptr);

			// IF the buffer should be silent, we can use null to indicate this for the audio sink
			if (bufferFlags & AUDCLNT_BUFFERFLAGS_SILENT) {
				buffer = nullptr;
			}

			// Pass data back to sink
			callback(buffer, length * (mixFormat->wBitsPerSample / 8));

			// Release space in buffer
			hr = captureClient->ReleaseBuffer(length);
		}

		if (kbhit()) {
			break;
		}
	}
}