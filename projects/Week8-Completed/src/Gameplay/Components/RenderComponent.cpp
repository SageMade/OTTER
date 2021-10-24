#include "Gameplay/Components/RenderComponent.h"

#include "Utils/ResourceManager/ResourceManager.h"

void RenderComponent::SetMaterial(Guid mat) {
	_material = ResourceManager::Get<Material>(mat);
}

void RenderComponent::SetMaterial(const Material::Sptr& mat) {
	_material = mat;
}

RenderComponent::RenderComponent(const MeshResource::Sptr& mesh, const Material::Sptr& material) :
	_mesh(mesh), 
	_material(material), 
	_meshBuilderParams(std::vector<MeshBuilderParam>()) 
{ }

RenderComponent::RenderComponent() : 
	_mesh(nullptr), 
	_material(nullptr), 
	_meshBuilderParams(std::vector<MeshBuilderParam>())
{ }

const VertexArrayObject::Sptr& RenderComponent::GetMesh() const {
	return _mesh->Mesh;
}

const Material::Sptr& RenderComponent::GetMaterial() const {
	return _material;
}

void RenderComponent::SetMesh(Guid mesh) {
	_mesh = ResourceManager::Get<MeshResource>(mesh);
}

void RenderComponent::SetMesh(const MeshResource::Sptr& mesh) {
	_mesh = mesh;
}


nlohmann::json RenderComponent::ToJson() const {
	nlohmann::json result;
	result["mesh"] = _mesh ? _mesh->GetGUID().str() : "null";
	result["material"] = _material ? _material->GetGUID().str() : "null";
	return result;
}

RenderComponent::Sptr RenderComponent::FromJson(const nlohmann::json& data) {
	RenderComponent::Sptr result = std::make_shared<RenderComponent>();
	result->_mesh = ResourceManager::Get<MeshResource>(Guid(data["mesh"].get<std::string>()));
	result->_material = ResourceManager::Get<Material>(Guid(data["material"].get<std::string>()));

	return result;
}

void RenderComponent::RenderImGui(GameObject* context) {
	// TODO
}
