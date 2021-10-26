#pragma once
#include "Gameplay/Physics/Collider.h"

class ConeCollider final : public ICollider {
public:
	typedef std::shared_ptr<ConeCollider> Sptr;
	static ConeCollider::Sptr Create(float radius = 0.5f, float height = 1.0f);
	virtual ~ConeCollider();

	ConeCollider* SetRadius(float value);
	float GetRadius() const;

	ConeCollider* SetHeight(float value);
	float GetHeight() const;

	// Inherited from ICollider
	virtual void DrawImGui() override;
	virtual void ToJson(nlohmann::json& blob) const override;
	virtual void FromJson(const nlohmann::json& data) override;

protected:
	virtual btCollisionShape* CreateShape() const override;

private:
	float _radius;
	float _height;

	ConeCollider(float radius, float height);
};