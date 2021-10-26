#pragma once
#include "Utils/ResourceManager/IResource.h"
#include "Graphics/VertexArrayObject.h"
#include "Utils/MeshFactory.h"

// You are NOT allowed to use the optimized loader in your GDW
// This define lets us easily disable the optimized loader without
// having to do too much code spelunking, by simply commenting out
// the following line
#define OPTIMIZED_OBJ_LOADER

class btTriangleMesh;

/// <summary>
/// A mesh resource contains information on how to generate a VAO at runtime
/// It can either load a VAO from a file, or generate one using the mesh 
/// factory and MeshBuilderParams
/// </summary>
class MeshResource : public IResource {
public:
	typedef std::shared_ptr<MeshResource> Sptr;

	MeshResource();
	MeshResource(const std::string& filename);

	virtual ~MeshResource();

	std::string                   Filename;
	VertexArrayObject::Sptr       Mesh;
	std::vector<MeshBuilderParam> MeshBuilderParams;

	std::shared_ptr<btTriangleMesh> BulletTriMesh;

	void GenerateMesh();
	void AddParam(const MeshBuilderParam& param);

	virtual nlohmann::json ToJson() const override;
	static MeshResource::Sptr FromJson(const nlohmann::json& blob);
};