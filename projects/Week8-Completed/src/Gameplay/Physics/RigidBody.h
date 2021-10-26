#pragma once
#include <EnumToString.h>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Physics/ICollider.h"

ENUM(RigidBodyType, int,
	Unknown   = 0,
	// Does not move within the scene, EX: level geometry
    Static    = 1,
	// Objects that are driven by physics
	Dynamic   = 2,
	// Objects that are driven by some control method, ex: doors, moving
	// platforms, etc...
	// kinematic objects will not collide with static or other
	// kinematic objects
	Kinematic = 3,
);

// We'll need to get stuff from the scene, which we can grab from our parent GO
class Scene;

class RigidBody : public IComponent {
public:
	typedef std::shared_ptr<RigidBody> Sptr;

	RigidBody(RigidBodyType type = RigidBodyType::Static);
	virtual ~RigidBody();

	/// <summary>
	/// Sets the mass for this object in KG
	/// </summary>
	/// <param name="value">The new mass of the object</param>
	void SetMass(float value);
	/// <summary>
	/// Gets the object's mass in KG
	/// </summary>
	float GetMass() const;

	/// <summary>
	/// Sets the linear damping for this object
	/// This is how quickly the object will bleed off linear (moving)
	/// velocity without any outside forces acting on it. Can be
	/// thought of as "air drag" for the object
	/// </summary>
	/// <param name="value">he new value for linear drag, default 0</param>
	void SetLinearDamping(float value);
	/// <summary>
	/// Gets the linear damping (ie drag) for this object
	/// </summary>
	float GetLinearDamping() const;

	/// <summary>
	/// Sets the angular damping for this object
	/// This is how quickly the object will bleed off angular (rotational)
	/// velocity without any outside forces acting on it. Can be
	/// thought of as "air drag" for the object
	/// </summary>
	/// <param name="value">The new value for angular drag, default 0.005f</param>
	void SetAngularDamping(float value);
	/// <summary>
	/// Gets the angular damping (ie drag) for this object
	/// </summary>
	float GetAngularDamping() const;

	/// <summary>
	/// A value between 0 and 31
	/// 
	/// Sets the collision group for the body (using the formula 1 << value)
	/// 
	/// When bullet goes to resolve collisions, only objects who's groups and masks 
	/// bitwise and (a & b) into a non-zero value are considered for collision. 
	/// Combined with CollisionMask, this allows objects to only collide with certain 
	/// other objects. For instance, if you have enemies that you do not want to be able
	/// to collide, you would assign them to some group (ex enemies are group 3), and
	/// then set their mask to a value where the corresponding bit is 0 (ex: 0b11110111)
	/// </summary>
	/// <param name="value">The new collision group for the object</param>
	void SetCollisionGroup(int value);
	/// <summary>
	/// Sets this object to belong to multiple collision groups, value should
	/// be a bitwise or (a | b) of all the groups that the object should belong
	/// to. See SetCollisionGroup
	/// </summary>
	/// <param name="value">The new muli-group value for collisiong group</param>
	void SetCollisionGroupMulti(int value);
	/// <summary>
	/// Gets the collision group for this object
	/// </summary>
	int GetCollisionGroup() const;

	/// <summary>
	/// Sets the mask of which collision groups this object should collide
	/// with. This is a bitwise mask, and should align with the collision
	/// groups
	/// </summary>
	/// <param name="value">The new collision mask for the object</param>
	void SetCollisionMask(int value);
	/// <summary>
	/// Gets the collision mask for this object
	/// </summary>
	int GetCollisionMask() const;

	/// <summary>
	/// Adds a new collider to this rigidbody.
	/// Multiple colliders can be added to a rigidbody, as internally it
	/// uses a compound shape collider
	/// </summary>
	/// <param name="collider">The collider to add to this body</param>
	/// <returns></returns>
	virtual ICollider::Sptr AddCollider(const ICollider::Sptr& collider);
	/// <summary>
	/// Removes a collider from this RigidBody
	/// </summary>
	/// <param name="collider">The collider to remove</param>
	void RemoveCollider(const ICollider::Sptr& collider);

	/// <summary>
	/// Applies a force in world space to this object, this would be used
	/// if you want to apply a force every frame on an object
	/// </summary>
	/// <param name="worldForce">The force in world space and Newtons</param>
	void ApplyForce(const glm::vec3& worldForce);
	/// <summary>
	/// Applies a force in world space to this object, this would be used
	/// if you want to apply a force every frame on an object
	/// </summary>
	/// <param name="worldForce">The force in world space and Newtons</param>
	/// <param name="localOffset">The offset from the object in worldspace units to apply the force</param>
	void ApplyForce(const glm::vec3& worldForce, const glm::vec3& localOffset);
	/// <summary>
	/// Applies a direct change in velocity on the object
	/// </summary>
	/// <param name="worldForce">The force in world space and m/s</param>
	void ApplyImpulse(const glm::vec3& worldForce);
	/// <summary>
	/// Applies a direct change in velocity on the object, relative to a given
	/// offset to the object
	/// </summary>
	/// <param name="worldForce">The force in world space and m/s</param>
	/// <param name="localOffset">The offset from the object in worldspace units to apply the impulse</param>
	void ApplyImpulse(const glm::vec3& worldForce, const glm::vec3& localOffset);
	/// <summary>
	/// Applies torque (rotational energy) to the object
	/// </summary>
	/// <param name="worldTorque">The torque, in world units and in Nm</param>
	void ApplyTorque(const glm::vec3& worldTorque);
	/// <summary>
	/// Applies an immediate change in angular momentum to an object
	/// </summary>
	/// <param name="worldTorque">The torque, in radians per second and world space</param>
	void ApplyTorqueImpulse(const glm::vec3& worldTorque);

	/// <summary>
	/// Sets the type of rigid body (static, dynamic, kinematic)
	/// </summary>
	/// <param name="type">The new type for the body</param>
	void SetType(RigidBodyType type);
	/// <summary>
	/// Returns this RigidBody's type (static, dynamic, kinematic)
	/// </summary>
	RigidBodyType GetType() const;

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

	// Inherited from IComponent
	virtual void Awake() override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static RigidBody::Sptr FromJson(const nlohmann::json& data);
	MAKE_TYPENAME(RigidBody)


protected:
	friend class PhysicsSystem;
	Scene*        _scene;

	glm::vec3 _prevScale;

	// The physics update mode for the body (static, dynamic, kinematic)
	RigidBodyType _type;

	// The mass of the object, in KG
	float         _mass;
	mutable bool  _isMassDirty;

	// List of colliders and whether they have been changed
	std::vector<ICollider::Sptr> _colliders;
	mutable bool  _isShapeDirty;

	// Controls phsyics damping (how quickly motion stops)
	// can be thought of as the "air resistance"
	float _angularDamping;
	float _linearDamping;
	mutable bool _isDampingDirty;

	// This lets us have objects that do not collide with each other!
	int _collisionGroup;
	int _collisionMask;
	mutable bool _isGroupMaskDirty;

	// Our bullet state stuff
	btRigidBody*     _body;
	btCompoundShape* _shape;
	btMotionState*   _motionState;
	btVector3        _inertia;

	// Handles adding a collider to our compound shape
	void _AddColliderToShape(ICollider* collider);

	// Handles resolving any dirty state stuff for our object
	void _HandleStateDirty();

protected:
	static int _editorSelectedColliderType;
};