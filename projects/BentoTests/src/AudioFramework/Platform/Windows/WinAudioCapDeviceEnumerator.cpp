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

	_numDevices = numDevs;
	_devices = reinterpret_cast<IAudioCapDevice**>(malloc(sizeof(IAudioCapDevice*) * numDevs));

	for (int ix = 0; ix < numDevs; ix++) {
		WinAudioCapDevice* device = new WinAudioCapDevice(devices[ix]);
		_devices[ix] = device;
	}

	_imfDevices = devices;
}

WinAudioCapDeviceEnumerator::~WinAudioCapDeviceEnumerator() {
	CoTaskMemFree(_imfDevices);
	for (int ix = 0; ix < _numDevices; ix++) {
		delete _devices[ix];
	}
	free(_devices);	
}
