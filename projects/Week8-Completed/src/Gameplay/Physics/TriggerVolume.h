#pragma once
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Physics/PhysicsBase.h"
#include "Gameplay/Physics/RigidBody.h"

class btPairCachingGhostObject;

class TriggerVolume : public PhysicsBase {
public:
	typedef std::function<void(const std::shared_ptr<RigidBody>& obj)> TriggerCallback;

	typedef std::shared_ptr<TriggerVolume> Sptr;
	virtual ~TriggerVolume();
	TriggerVolume();

	/// <summary>
	/// Invoked for each RigidBody before the physics world is stepped forward a frame,
	/// handles body initialization, shape changes, mass changes, etc...
	/// </summary>
	/// <param name="dt">The time in seconds since the last frame</param>
	virtual void PhysicsPreStep(float dt);
	/// <summary>
	/// Invoked for each RigidBody after the physics world is stepped forward a frame,
	/// handles copying transform to the OpenGL state
	/// </summary>
	/// <param name="dt">The time in seconds since the last frame</param>
	virtual void PhysicsPostStep(float dt);

	void SetEnterCallback(const std::shared_ptr<IComponent>& component, TriggerCallback callback);
	void RemoveEnterCallback(const std::shared_ptr<IComponent>& component);

	void SetLeaveCallback(const std::shared_ptr<IComponent>& component, TriggerCallback callback);
	void RemoveLeaveCallback(const std::shared_ptr<IComponent>& component);

	// Inherited from IComponent

	virtual void Awake() override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static TriggerVolume::Sptr FromJson(const nlohmann::json& data);
	MAKE_TYPENAME(TriggerVolume);

protected:
	btPairCachingGhostObject*   _ghost;

	std::unordered_map<std::shared_ptr<IComponent>, TriggerCallback> _enterCallbacks;
	std::unordered_map<std::shared_ptr<IComponent>, TriggerCallback> _exitCallbacks;
	std::vector<std::weak_ptr<RigidBody>> _currentCollisions;

	virtual btBroadphaseProxy* _GetBroadphaseHandle() override;

};