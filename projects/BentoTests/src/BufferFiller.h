#pragma once
#include <functional>

class BufferFiller {
public:
	BufferFiller(uint8_t** dataStores, uint32_t numBuffers, size_t size);
	BufferFiller(size_t size, uint32_t count = 1);
	~BufferFiller();

	void FeedData(const uint8_t** data, size_t length, std::function<void(uint8_t**, size_t)> onFullCallback);
	void Flush();

	bool HasData() const { return bufferOffset > 0; }

	//uint8_t** Data() { return buffers; }
	//const uint8_t** Data() const { return buffers; }
	uint8_t** DataBuffers() { return buffers; }
	const uint8_t** DataBuffers() const { return (const uint8_t**)buffers; }

	size_t Size() const { return bufferSize; }

protected: 
	uint8_t** buffers;
	uint32_t bufferCount;
	size_t   bufferSize;
	size_t   bufferOffset;
	bool     ownsBuffer;
};