#pragma once
#include <string>

class IBinaryStream {
public:
	virtual ~IBinaryStream() = default;

	virtual void WriteBytes(const void* data, size_t length) = 0;

	template <typename T>
	static void Write(IBinaryStream& stream, const T& value) {
		stream.WriteBytes(*value, sizeof(T));
	}
	void WriteStr(const char* value) {
		size_t len = strlen(value);
		WriteBytes(value, len);
	}

	virtual void Flush() {};

protected:
	IBinaryStream() = default;
};