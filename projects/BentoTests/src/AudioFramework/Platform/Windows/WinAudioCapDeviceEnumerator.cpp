#include "WinAudioCapDeviceEnumerator.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mmdeviceapi.h>

#include <AudioFramework/Platform/Windows/WinAudioCapDevice.h>

WinAudioCapDeviceEnumerator::WinAudioCapDeviceEnumerator() : IAudioCapDeviceEnumerator()
{
	IMFAttributes* attributes = nullptr;          // Attributes of all audio recording devices (?)
	IMFActivate** devices = nullptr;              // List of all audio recording devices

	// Create an attribute set and set it to only include audio capture devices
	MFCreateAttributes(&attributes, 1);
	attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);

	// List all devices
	UINT numDevs = 0;
	MFEnumDeviceSources(attributes, &devices, &numDevs);

	// Free the attributes now that we're done with them
	attributes->Release();

	// Allocate an array to store the device list
	_numDevices = numDevs;
	_devices = new IAudioCapDevice*[numDevs];

	// Create a new device for each IMFAttribute configuration
	for (int ix = 0; ix < numDevs; ix++) {
		WinAudioCapDevice* device = new WinAudioCapDevice(devices[ix]);
		_devices[ix] = device;
	}

	// Store the IMF devices array for later cleanup
	_imfDevices = devices;
}

WinAudioCapDeviceEnumerator::~WinAudioCapDeviceEnumerator() {
	CoTaskMemFree(_imfDevices);
	for (int ix = 0; ix < _numDevices; ix++) {
		delete _devices[ix];
	}
	delete[] _devices;
}
