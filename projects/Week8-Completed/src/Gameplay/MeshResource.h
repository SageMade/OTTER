#pragma once
#include "Utils/ResourceManager/IResource.h"
#include "Graphics/VertexArrayObject.h"
#include "Utils/MeshFactory.h"

class MeshResource : public IResource {
public:
	typedef std::shared_ptr<MeshResource> Sptr;

	MeshResource();
	MeshResource(const std::string& filename);

	virtual ~MeshResource() = default;

	std::string                   Filename;
	VertexArrayObject::Sptr       Mesh;
	std::vector<MeshBuilderParam> MeshBuilderParams;

	void GenerateMesh();
	void AddParam(const MeshBuilderParam& param);

	virtual nlohmann::json ToJson() const override;
	static MeshResource::Sptr FromJson(const nlohmann::json& blob);
};