#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"

class JumpBehaviour : public IComponent {
public:
	typedef std::shared_ptr<JumpBehaviour> Sptr;

	JumpBehaviour();
	virtual ~JumpBehaviour();

	virtual void Awake(GameObject* context) override;
	virtual void Update(GameObject* context, float deltaTime) override;

	virtual void RenderImGui(GameObject* context) override;
	MAKE_TYPENAME(JumpBehaviour);
	virtual nlohmann::json ToJson() const override;
	static JumpBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _impulse;

	bool _isPressed = false;
	RigidBody::Sptr _body;
};