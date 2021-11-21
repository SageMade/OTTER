#pragma once
#include <functional>

/// <summary>
/// Utility class for filling a collection of buffers with data, and
/// invoking a callback method when the buffer is full
/// 
/// Can be configured to self manage internal buffers, or manage existing
/// data buffers
/// </summary>
class BufferFiller {
public:
	~BufferFiller();

	BufferFiller(const BufferFiller& other) = delete;
	BufferFiller(BufferFiller&& other) = delete;
	BufferFiller& operator=(const BufferFiller& other) = delete;
	BufferFiller& operator=(BufferFiller&& other) = delete;

	/// <summary>
	/// Create a new instance of a buffer filler that will manage an existing set of
	/// buffers. Note that any existing data in the buffers will be zeroed out
	/// </summary>
	/// <param name="dataStores">An array of buffer pointers</param>
	/// <param name="numBuffers">The numbder of buffers in the array</param>
	/// <param name="size">The size of a single buffer in bytes</param>
	BufferFiller(uint8_t** dataStores, uint32_t numBuffers, size_t size);
	/// <summary>
	/// Creates a new instance of a buffer that manages an internal set of buffers
	/// </summary>
	/// <param name="count">The number of buffers to allocate</param>
	/// <param name="size">The size of the buffers</param>
	BufferFiller(uint32_t count, size_t size);

	/**
	 * Feeds a stream of planar data into this buffer filler.
	 * 
	 * For interleaved data streams, only a single buffer should be used (and only the first buffer set)
	 * 
	 * Passing zero for length will immediately invoke the callback and reset the buffer's offset to zero,
	 * effectively flushing any remaining data stored in the buffer
	 * 
	 * @param data The data planes to feed to the buffer filler, must match the number of data planes in this filler
	 * @param length The length of the data to feed in bytes, each data plane must contain at least this number of bytes
	 * @param onFullCallback The callback to invoke when the buffer has reached capacity and is about to loop back
	 */
	void FeedData(const uint8_t** data, size_t length, std::function<void(uint8_t**, size_t)> onFullCallback);
	/// <summary>
	/// Zeroes any remaining elements in the buffers
	/// </summary>
	void Flush();

	/// <summary>
	/// Returns true if there is data remaining in this buffer
	/// </summary>
	/// <returns>True if there is data remaining in the buffer</returns>
	bool HasData() const { return bufferOffset > 0; }

	/// <summary>
	/// Returns the array of buffer pointers that this object manages
	/// </summary>
	uint8_t** DataBuffers() { return buffers; }
	/// <summary>
	/// Returns the array of buffer pointers that this object manages
	/// </summary>
	const uint8_t** DataBuffers() const { return (const uint8_t**)buffers; }

	/// <summary>
	/// Returns the current offset into the buffer, can be used to determine
	/// how full the buffers are
	/// </summary>
	size_t InternalBufferOffset() const { return bufferOffset; }

	/// <summary>
	/// Returns the size of the buffers that this object is managing in bytes
	/// </summary>
	size_t Size() const { return bufferSize; }

protected: 
	uint8_t** buffers;
	uint32_t bufferCount;
	size_t   bufferSize;
	size_t   bufferOffset;  
	// True if this instance has allocated it's own buffers
	bool     ownsBuffer; 
};