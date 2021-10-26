#pragma once
#include "IComponent.h"

class RotatingBehaviour : public IComponent {
public:
	typedef std::shared_ptr<RotatingBehaviour> Sptr;

	RotatingBehaviour() = default;
	glm::vec3 RotationSpeed;

	virtual void Update(float deltaTime) override;

	virtual void RenderImGui() override;

	virtual nlohmann::json ToJson() const override;
	static RotatingBehaviour::Sptr FromJson(const nlohmann::json& data);

	MAKE_TYPENAME(RotatingBehaviour);
};

