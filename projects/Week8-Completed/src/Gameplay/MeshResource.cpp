#include "MeshResource.h"
#include "Utils/ObjLoader.h"
#include <filesystem>

MeshResource::MeshResource() :
	IResource(),
	Filename(""),
	MeshBuilderParams(std::vector<MeshBuilderParam>()),
	Mesh(nullptr)
{ }

MeshResource::MeshResource(const std::string& filename) :
	IResource(),
	Filename(filename),
	MeshBuilderParams(std::vector<MeshBuilderParam>()),
	Mesh(nullptr)
{
	Mesh = ObjLoader::LoadFromFile(filename);
}

nlohmann::json MeshResource::ToJson() const {
	nlohmann::json result;
	if (MeshBuilderParams.size() > 0) {
		std::vector<nlohmann::json> params = std::vector<nlohmann::json>();
		params.resize(MeshBuilderParams.size());
		for (int ix = 0; ix < MeshBuilderParams.size(); ix++) {
			params[ix] = MeshBuilderParams[ix].ToJson();
		}
		result["params"] = params;
	} else {
		result["filename"] = Filename.empty() ? "null" : Filename;
	}
	return result;
}

MeshResource::Sptr MeshResource::FromJson(const nlohmann::json& blob)
{
	MeshResource::Sptr result = std::make_shared<MeshResource>();
	if (blob.contains("params") && blob["params"].is_array()) {
		std::vector<nlohmann::json> meshbuilderParams = blob["params"].get<std::vector<nlohmann::json>>();
		MeshBuilder<VertexPosNormTexCol> mesh;
		for (int ix = 0; ix < meshbuilderParams.size(); ix++) {
			MeshBuilderParam p = MeshBuilderParam::FromJson(meshbuilderParams[ix]);
			result->MeshBuilderParams.push_back(p);
			MeshFactory::AddParameterized(mesh, p);
		}
		result->Mesh = mesh.Bake();
	} else {
		result->Filename = JsonGet<std::string>(blob, "filename", "null");
		if (result->Filename != "null" && std::filesystem::exists(result->Filename)) {
			result->Mesh = ObjLoader::LoadFromFile(result->Filename);
		}
	}
	return result;
}

void MeshResource::GenerateMesh() {
	MeshBuilder<VertexPosNormTexCol> mesh;
	for (auto& param : MeshBuilderParams) {
		MeshFactory::AddParameterized(mesh, param);
	}
	Mesh = mesh.Bake();
}

void MeshResource::AddParam(const MeshBuilderParam& param) {
	MeshBuilderParams.push_back(param);
}
