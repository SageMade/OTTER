#include "BinaryWriter.h"

BinaryWriter::BinaryWriter(size_t initialSize) :
	dataSize(initialSize),
	dataStore(nullptr),
	seekPos(0)
{
	_Resize(dataSize);
}

BinaryWriter::~BinaryWriter() {
	if (dataStore != nullptr) {
		free(dataStore);
	}
}

void BinaryWriter::Reserve(size_t bytes) {
	_Resize(dataSize + bytes);
}

void BinaryWriter::WriteBytes(const void* data, size_t length) {
	if (seekPos + length > dataSize) {
		_Resize(dataSize + length);
	}
	memcpy(dataStore + seekPos, data, length);
	seekPos += length;
}
void BinaryWriter::_Resize(size_t newLen) {
	if (newLen < dataSize) return; // TODO: log errors;
	uint8_t* newData = reinterpret_cast<uint8_t*>(malloc(newLen));
	if (newData == nullptr) {
		return;
	}
	if (dataStore != nullptr) {
		memcpy(newData, dataStore, dataSize);
	}
	dataSize = newLen;
	dataStore = newData;
}
