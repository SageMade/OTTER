#pragma once
#include <cstdint>
#include <memory>
#include "IBinaryStream.h"

class BinaryWriter : public IBinaryStream {
public:
	BinaryWriter(size_t initialSize = 0);
	virtual ~BinaryWriter();

	void Reserve(size_t bytes);
	size_t ReservedSize() const { return dataSize; }
	size_t Length() const { return seekPos; }

	template <typename T>
	void Write(const T& value) {
		WriteBytes(*value, sizeof(T));
	}

	virtual void WriteBytes(const void* data, size_t length) override;


protected:
	size_t dataSize;
	size_t seekPos;
	uint8_t* dataStore;
	
	void _Resize(size_t newLen);
};
