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

	/// <summary>
	/// Gets the mesh resource which contains the mesh and serialization info for
	/// this render component
	/// </summary>
	const MeshResource::Sptr& GetMeshResource() const;
	/// <summary>
	/// Gets the VAO of the underlying mesh resource
	/// </summary>
	const VertexArrayObject::Sptr& GetMesh() const;
	/// <summary>
	/// Gets the material that this renderer is using
	/// </summary>
	const Material::Sptr& GetMaterial() const;

	void SetMesh(const MeshResource::Sptr& mesh);
	void SetMaterial(const Material::Sptr& mat);
	void SetMaterial(Guid mat);
	void SetMesh(Guid mesh);

	virtual void RenderImGui() override;

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