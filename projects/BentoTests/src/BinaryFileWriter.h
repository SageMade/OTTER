#pragma once
#include "IBinaryStream.h"
#include <string>

class BinaryFileWriter : public IBinaryStream {
public:
	BinaryFileWriter(const std::string& fname);
	virtual ~BinaryFileWriter();

	virtual void WriteBytes(const void* data, size_t length) override;

	virtual void Flush() override;

protected:
	FILE* file;
};