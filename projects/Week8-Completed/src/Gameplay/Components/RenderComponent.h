#pragma once
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/MeshResource.h"
#include "Gameplay/Material.h"
#include "Utils/MeshFactory.h"

class RenderComponent : public IComponent {
public:
	typedef std::shared_ptr<RenderComponent> Sptr;

	RenderComponent();
	RenderComponent(const MeshResource::Sptr& mesh, const Material::Sptr& material);

	const VertexArrayObject::Sptr& GetMesh() const;
	const Material::Sptr& GetMaterial() const;

	void SetMesh(const MeshResource::Sptr& mesh);
	void SetMaterial(const Material::Sptr& mat);
	void SetMaterial(Guid mat);
	void SetMesh(Guid mesh);

	virtual void RenderImGui(GameObject* context) override;

	virtual nlohmann::json ToJson() const override;
	static RenderComponent::Sptr FromJson(const nlohmann::json& data);

	MAKE_TYPENAME(RenderComponent);



protected:
	// The object's mesh
	MeshResource::Sptr _mesh;
	// The object's material
	Material::Sptr      _material;

	// If we want to use MeshFactory, we can populate this list
	std::vector<MeshBuilderParam> _meshBuilderParams;
};