#pragma once

#include "Gameplay/Physics/Collider.h"

class ConvexMeshCollider final : public ICollider {
public:
	typedef std::shared_ptr<ConvexMeshCollider> Sptr;
	static ConvexMeshCollider::Sptr Create();
	virtual ~ConvexMeshCollider();

	// Inherited from ICollider
	virtual void Awake(GameObject* context) override;
	virtual void DrawImGui() override;
	virtual void ToJson(nlohmann::json& blob) const override;
	virtual void FromJson(const nlohmann::json& data) override;

protected:
	btTriangleMesh* _triMesh;
	ConvexMeshCollider();

	virtual btCollisionShape* CreateShape() const override;

};