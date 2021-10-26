/**
 * NOTE: you MAY NOT use this file in your GDW game or graphics assignments
 * (at least for the fall semester)
 * 
 * You may use this implementation as a reference to implement your own version
 * using similar concepts, and that fit better with your game
 */
#pragma once
#include <fstream>

#include "Graphics/VertexArrayObject.h"
#include "Graphics/VertexTypes.h"

#include "Utils/MeshBuilder.h"


class OptimizedObjLoader {
public:
	static VertexArrayObject::Sptr LoadFromFile(const std::string& filename);
	static void ConvertToBinary(const std::string& inFile, const std::string& outFile = "");

	/// <summary>
	/// Saves a mesh builder of the given type to a binary file
	/// </summary>
	/// <typeparam name="VertexType"></typeparam>
	/// <param name="mesh"></param>
	/// <param name="outFilename"></param>
	template <typename VertexType>
	static void SaveBinaryFile(MeshBuilder<VertexType>& mesh, const std::string& outFilename);

protected:
	struct BinaryHeader {
		char      HeaderBytes[4] ={ 'B', 'O', 'B', 'J' };
		uint16_t  Version;
		uint32_t  NumIndices;
		IndexType IndicesType;
		uint32_t  NumVertices;
		uint16_t  VertexStride;
		uint8_t   NumAttributes;
	};

	OptimizedObjLoader() = default;
	~OptimizedObjLoader() = default;

	static MeshBuilder<VertexPosNormTexCol>* _LoadFromObjFile(const std::string& filename);
	static VertexArrayObject::Sptr _LoadFromBinFile(const std::string& filename);
};

template <typename VertexType>
void OptimizedObjLoader::SaveBinaryFile(MeshBuilder<VertexType>& mesh, const std::string& outFilename) {
	// Open the output file
	std::ofstream file(outFilename, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Failed to open output file");
	}

	// Create the fixed size header for our output file
	BinaryHeader header  = BinaryHeader();
	header.Version       = 0x01; // This is version 1! Update this and implement different readers if changes to format are made
	header.NumIndices    = mesh.GetIndexCount();
	header.IndicesType   = IndexType::UInt;
	header.NumVertices   = mesh.GetVertexCount();
	header.VertexStride  = sizeof(VertexType);
	header.NumAttributes = VertexType::V_DECL.size();

	// Write header bytes to the stream
	file.write(reinterpret_cast<const char*>(&header), sizeof(BinaryHeader));

	// Write which attributes we have to the stream
	for (int ix = 0; ix < VertexType::V_DECL.size(); ix++) {
		file.write(reinterpret_cast<const char*>(&VertexType::V_DECL[ix]), sizeof(BufferAttribute));
	}
	// Write any index data to the file
	if (mesh.GetIndexCount() > 0) {
		file.write(reinterpret_cast<const char*>(mesh.GetIndexDataPtr()), mesh.GetIndexCount() * sizeof(uint32_t));
	}

	// Write vertex data to file
	file.write(reinterpret_cast<const char*>(mesh.GetVertexDataPtr()), mesh.GetVertexCount() * sizeof(VertexType));
}
