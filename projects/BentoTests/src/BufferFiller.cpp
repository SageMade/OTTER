#include "BufferFiller.h"
#include <GLM/glm.hpp>

BufferFiller::BufferFiller(uint8_t** dataStores, uint32_t numBuffers, size_t size)
	: buffers(nullptr), bufferCount(numBuffers), bufferSize(size), bufferOffset(0), ownsBuffer(false)
{
	buffers = new uint8_t*[bufferSize];
	for(int ix = 0; ix < bufferCount; ix++) {
		buffers[ix] = dataStores[ix];
		memset(buffers[ix], 0, bufferSize);
	}
}

BufferFiller::BufferFiller(size_t size, uint32_t count)
	: buffers(nullptr), bufferSize(size), bufferOffset(0), ownsBuffer(true), bufferCount(count) {
	buffers = new uint8_t*[bufferSize];
	for (int ix = 0; ix < bufferCount; ix++) {
		buffers[ix] = reinterpret_cast<uint8_t*>(malloc(bufferSize));
		memset(buffers[ix], 0, bufferSize);
	}
}

BufferFiller::~BufferFiller() {
	if (ownsBuffer) {
		for (int ix = 0; ix < bufferCount; ix++) {
			free(buffers[ix]);
		}
	}
	delete[] buffers;
}

void BufferFiller::FeedData(const uint8_t** data, size_t length, std::function<void(uint8_t**, size_t)> onFullCallback) {
	// We use remaining and an offset so we don't have to spend the time going back and forth
	size_t remaining = length;
	size_t offset = 0;

	// If we have a partial frame, we should try to complete it
	if (bufferOffset > 0) {
		// Determine the number of bytes to copy, then copy into partial frame
		size_t count = glm::min(bufferSize - bufferOffset, length);
		for(int ix = 0; ix < bufferCount; ix++) {
			memcpy(buffers[ix] + bufferOffset, data[ix], count);
		}
		// If the frame is still incomplete, increase the offset by the number of bytes we've added
		if (bufferOffset + count < bufferSize) {
			bufferOffset += count;
			return; // We've written all data, bye bi
		}
		// If the frame is now complete, encode it into our FFMPEG stream
		else if (bufferOffset + count == bufferSize) {
			onFullCallback(buffers, bufferSize);
			bufferOffset = 0;
		}
		// Modify our counters based on number of bytes read from input
		remaining -= count;
		offset = count;
	}

	// Read full frames from input to FFMPEG
	while (remaining >= bufferSize) {
		for (int ix = 0; ix < bufferCount; ix++) {
			memcpy(buffers[ix], data[ix] + offset, bufferSize);
		}
		onFullCallback(buffers, bufferSize);
		remaining -= bufferSize;
		offset += bufferSize;
	}
	// Read any remaining data into a partial frame
	if (remaining > 0) {
		for (int ix = 0; ix < bufferCount; ix++) {
			memcpy(buffers[ix], data[ix] + offset, remaining);
		}
		bufferOffset = remaining;
	}
}

void BufferFiller::Flush() {
	for (int ix = 0; ix < bufferCount; ix++) {
		memset(buffers[ix] + bufferOffset, 0, bufferSize - bufferOffset);
	}
}
