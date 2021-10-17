#include "BinaryFileWriter.h"

BinaryFileWriter::BinaryFileWriter(const std::string& fname) :
	file(nullptr) {
	file = fopen(fname.c_str(), "wb");

}

BinaryFileWriter::~BinaryFileWriter() {
	if (file != nullptr) {
		fclose(file);
	}
}

void BinaryFileWriter::WriteBytes(const void* data, size_t length) {
	if (file != nullptr) {
		fwrite(data, 1, length, file);
	}
}

void BinaryFileWriter::Flush() {
	fflush(file);
}
