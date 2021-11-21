#include "WinAudioCapDeviceEnumerator.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mmdeviceapi.h>
#include <mfobjects.h>
#include <stdexcept>

#include "AudioFramework/Platform/Windows/WinAudioCapDevice.h"

WinAudioCapDeviceEnumerator::WinAudioCapDeviceEnumerator() : IAudioCapDeviceEnumerator()
{
	// Attributes of devices to search for (audio capture devices)
	IMFAttributes* attributes = nullptr; 

	// Create an attribute set and set it to only include audio capture devices
	MFCreateAttributes(&attributes, 1);
	attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);

	// List all devices based on attributes defined above
	IMFActivate** devices = nullptr; 
	UINT numDevs = 0;
	MFEnumDeviceSources(attributes, &devices, &numDevs);

	// Free the attributes now that we're done with them
	attributes->Release();

	// Allocate an array to store the device list
	_numDevices = numDevs;
	_devices = new IAudioCapDevice*[numDevs];

	try {
		// Create a new device for each IMFAttribute configuration
		for (int ix = 0; ix < numDevs; ix++) {
			WinAudioCapDevice* device = new WinAudioCapDevice(devices[ix]);
			_devices[ix] = device;
		}
	} catch (std::runtime_error& error) {
		// Delete our temporary IMF device list using Windows magic
		CoTaskMemFree(devices);

		throw error;
	} 

	// Delete our temporary IMF device list using Windows magic
	CoTaskMemFree(devices);
}

WinAudioCapDeviceEnumerator::~WinAudioCapDeviceEnumerator() {
	for (int ix = 0; ix < _numDevices; ix++) {
		delete _devices[ix];
	}
	delete[] _devices;
}

size_t WinAudioCapDeviceEnumerator::DeviceCount() const {
	return _numDevices;
}

IAudioCapDevice**const WinAudioCapDeviceEnumerator::GetDevices() const {
	return _devices;
}

IAudioCapDevice* WinAudioCapDeviceEnumerator::GetDefaultDevice() const {
	return nullptr;
}

IAudioCapDevice* WinAudioCapDeviceEnumerator::GetDevice(uint32_t index) const {
	return index < _numDevices ? _devices[index] : nullptr;
}

IAudioCapDevice* WinAudioCapDeviceEnumerator::GetDevice(const std::string& name) const {
	for (int ix = 0; ix < _numDevices; ix++) {
		if (_devices[ix]->GetName() == name) {
			return _devices[ix];
		}
	}
	return nullptr;
}
