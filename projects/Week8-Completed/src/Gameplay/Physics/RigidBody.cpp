#include "RigidBody.h"

#include <algorithm>
#include <GLM/glm.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Utils
#include "Utils/ImGuiHelper.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/GlmBulletConversions.h"

int RigidBody::_editorSelectedColliderType = 0;

RigidBody::RigidBody(RigidBodyType type) :
	IComponent(),
	_scene(nullptr),
	_type(type),
	_mass(1.0f),
	_isMassDirty(true),
	_colliders(std::vector<ICollider::Sptr>()),
	_isShapeDirty(true),
	_body(nullptr),
	_shape(nullptr),
	_motionState(nullptr),
	_inertia(btVector3())
{ }

RigidBody::~RigidBody() {
	if (_body != nullptr) {
		// Remove from the physics world
		_scene->_physicsWorld->removeRigidBody(_body);

		// Remove from the scene's list of bodies
		auto& it = std::find(_scene->_rigidBodies.begin(), _scene->_rigidBodies.end(), this);
		if (it != _scene->_rigidBodies.end()) {
			_scene->_rigidBodies.erase(it);
		}

		// Clean up all our memory
		delete _motionState;
		delete _shape;
		delete _body;
		_colliders.clear();
	}
}

void RigidBody::SetMass(float value) {
	if (_type != RigidBodyType::Static) {
		_isMassDirty = value != _mass;
		_mass = value;
	}
}

float RigidBody::GetMass() const {
	return _type == RigidBodyType::Static ? 0.0f : _mass;
}

void RigidBody::SetLinearDamping(float value) {
	_linearDamping = value;
	_isDampingDirty = true;
}

float RigidBody::GetLinearDamping() const {
	return _linearDamping;
}

void RigidBody::SetAngularDamping(float value) {
	_angularDamping = value;
	_isDampingDirty = true;
}

float RigidBody::GetAngularDamping() const {
	return _angularDamping;
}

void RigidBody::SetCollisionGroup(int value) {
	_collisionGroup = 1 << value;
	_isGroupMaskDirty = true;
}

void RigidBody::SetCollisionGroupMulti(int value) {
	_collisionGroup   = value;
	_isGroupMaskDirty = true;
}

int RigidBody::GetCollisionGroup() const {
	return _collisionGroup;
}

void RigidBody::SetCollisionMask(int value) {
	_collisionMask = value;
	_isGroupMaskDirty = true;
}

int RigidBody::GetCollisionMask() const {
	return _collisionMask;
}

ICollider::Sptr RigidBody::AddCollider(const ICollider::Sptr& collider) {
	_colliders.push_back(collider);
	_isShapeDirty = true;
	return collider;
}

void RigidBody::RemoveCollider(const ICollider::Sptr& collider) {
	auto& it = std::find(_colliders.begin(), _colliders.end(), collider);
	if (it != _colliders.end()) {
		if (collider->_shape != nullptr) {
			_shape->removeChildShape(collider->_shape);
			_isMassDirty = true;
		}
		_colliders.erase(it);
	}
}

void RigidBody::ApplyForce(const glm::vec3& worldForce) {
	_body->applyCentralForce(ToBt(worldForce));
}

void RigidBody::ApplyForce(const glm::vec3& worldForce, const glm::vec3& localOffset) {
	_body->applyForce(ToBt(worldForce), ToBt(localOffset));
}

void RigidBody::ApplyImpulse(const glm::vec3& worldForce) {
	_body->applyCentralImpulse(ToBt(worldForce));
}

void RigidBody::ApplyImpulse(const glm::vec3& worldForce, const glm::vec3& localOffset) {
	_body->applyImpulse(ToBt(worldForce), ToBt(localOffset));
}

void RigidBody::ApplyTorque(const glm::vec3& worldTorque) {
	_body->applyTorque(ToBt(worldTorque));
}

void RigidBody::ApplyTorqueImpulse(const glm::vec3& worldTorque) {
	_body->applyTorqueImpulse(ToBt(worldTorque));
}

void RigidBody::PhysicsPreStep(float dt) {
	// Update any dirty state that may have changed
	_HandleStateDirty();

	if (_type != RigidBodyType::Static) {
		// Copy our transform info from OpenGL
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(ToBt(_context->Position));
		transform.setRotation(ToBt(glm::quat(glm::radians(_context->Rotation))));
		if (_context->Scale != _prevScale) {
			_scene->_physicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(_body->getBroadphaseHandle(), _scene->_physicsWorld->getDispatcher());
			_prevScale = _context->Scale;
		}

		// Copy to body and to it's motion state
		if (_type == RigidBodyType::Dynamic) {
			_body->setWorldTransform(transform);
		} else {
			// Kinematics prefer to be driven my motion state for some reason :|
			_body->getMotionState()->setWorldTransform(transform);
		}
	}
}

void RigidBody::PhysicsPostStep(float dt) {
	// TODO: If we have parented objects in the future, we'd need to convert to local space!
	if (_type != RigidBodyType::Static) {
		btTransform transform;

		if (_type == RigidBodyType::Dynamic) {
			transform = _body->getWorldTransform();
		} else {
			// Kinematics prefer to be driven my motion state for some reason :|
			_body->getMotionState()->getWorldTransform(transform);
		}

		// Update the pos and rotation params
		_context->Position = ToGlm(transform.getOrigin());
		_context->Rotation = glm::degrees(glm::eulerAngles(ToGlm(transform.getRotation())));
	}
}

void RigidBody::Awake(GameObject* context) {
	_context = context;
	_scene = context->GetScene();
	_prevScale = context->Scale;

	// Create our compound shape and add all colliders
	_shape = new btCompoundShape(true, _colliders.size());
	_shape->setLocalScaling(ToBt(context->Scale));
	for (auto& collider : _colliders) {
		_AddColliderToShape(collider.get());
	}
	// Update inertia
	_shape->calculateLocalInertia(_mass, _inertia);
	_isMassDirty = false;

	// Create a default motion state instance for tracking the bodies motion
	_motionState = new btDefaultMotionState();

	// Get the object's starting transform, create a bullet representation for it
	btTransform transform; 
	transform.setIdentity();
	transform.setOrigin(ToBt(context->Position));
	transform.setRotation(ToBt(glm::quat(glm::radians(context->Rotation))));

	// Create the bullet rigidbody and add it to the physics scene
	_body = new btRigidBody(_mass, _motionState, _shape, _inertia);
	_scene->_physicsWorld->addRigidBody(_body);

	// If the object is kinematic (driven by a controller), tell bullet that
	if (_type == RigidBodyType::Kinematic) {
		_body->setCollisionFlags(_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	} 
	// If the object is static, disable it's gravity and notify bullet
	else if (_type == RigidBodyType::Static) {
		_body->setGravity(btVector3(0.0f, 0.0f, 0.0f));
		_body->setCollisionFlags(_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	}
	
	_body->setActivationState(DISABLE_DEACTIVATION);

	// Copy over group and mask info
	_body->getBroadphaseProxy()->m_collisionFilterGroup = _collisionGroup;
	_body->getBroadphaseProxy()->m_collisionFilterMask  = _collisionMask;

	// Store ourselves in the scene's list of rigid bodies
	_scene->_rigidBodies.push_back(this);
}

void RigidBody::RenderImGui(GameObject* context)
{
	_isMassDirty |= LABEL_LEFT(ImGui::DragFloat, "Mass", &_mass, 0.1f, 0.0f);
	// Our colliders header
	ImGui::Separator(); ImGui::TextUnformatted("Colliders"); ImGui::Separator();
	ImGui::Indent();
	// Draw UI for all colliders
	for (int ix = 0; ix < _colliders.size(); ix++) {
		ICollider::Sptr& collider = _colliders[ix];
		ImGui::PushID(ix);
		// Draw collider type name and the delete button
		ImGui::Text((~collider->GetType()).c_str());
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			RemoveCollider(collider);
			ix--;
			ImGui::PopID();
			continue;
		}

		collider->_isDirty |= LABEL_LEFT(ImGui::DragFloat3, "Position", &collider->_position.x, 0.01f);
		collider->_isDirty |= LABEL_LEFT(ImGui::DragFloat3, "Rotation", &collider->_rotation.x, 1.0f);
		collider->_isDirty |= LABEL_LEFT(ImGui::DragFloat3, "Scale   ", &collider->_scale.x, 0.01f);
		// Draw collider's editor
		collider->DrawImGui();
		
		ImGui::Separator();
		ImGui::PopID();
	}
	// Draw the add collider combo box and button
	ImGui::Combo("", &_editorSelectedColliderType, ColliderTypeComboNames);
	ImGui::SameLine();
	if (ImGui::Button("Add Collider")) {
		// Since the combo box contains all valid items (and Unknown is 0)
		// we need to add 1 to the resulting selection index
		ColliderType type = (ColliderType)(_editorSelectedColliderType + 1);
		_colliders.push_back(ICollider::Create(type));
	}

	ImGui::Unindent();
}

nlohmann::json RigidBody::ToJson() const {
	nlohmann::json result;
	result["type"] = ~_type;
	result["mass"] = _mass;
	result["linear_damping"] = _linearDamping;
	result["angular_damping"] = _angularDamping;
	result["group"] = _collisionGroup;
	result["mask"]  = _collisionMask;
	// Make an array and store all the colliders
	result["colliders"] = std::vector<nlohmann::json>();
	for (auto& collider : _colliders) {
		nlohmann::json blob;
		blob["guid"] = collider->_guid.str();
		blob["type"] = ~collider->_type;
		blob["position"] = GlmToJson(collider->_position);
		blob["rotation"] = GlmToJson(collider->_rotation);
		blob["scale"]    = GlmToJson(collider->_scale);
		collider->ToJson(blob);
		result["colliders"].push_back(blob);
	}
	return result;
}

RigidBody::Sptr RigidBody::FromJson(const nlohmann::json& data) {
	RigidBody::Sptr result = std::make_shared<RigidBody>();
	// Read out the RigidBody config
	result->_type = ParseRigidBodyType(data["type"], RigidBodyType::Unknown);
	result->_mass = data["mass"];
	result->_linearDamping  = data["linear_damping"];
	result->_angularDamping = data["angular_damping"];
	result->_collisionGroup = data["group"];
	result->_collisionMask  = data["mask"];

	// There should always be colliders, but just to be safe...
	if (data.contains("colliders") && data["colliders"].is_array()) {
		// Iterate over all colliders
		for (auto& blob : data["colliders"]) {
			// Get the type
			ColliderType type = ParseColliderType(blob["type"], ColliderType::Unknown);
			// Get the actual collider based on the type we got from our file
			ICollider::Sptr collider = ICollider::Create(type);
			// If we got a valid shape and thus collider, load and store
			if (collider != nullptr) {
				// Copy in collider info
				collider->_guid = Guid(blob["guid"]);
				collider->_position = ParseJsonVec3(blob["position"]);
				collider->_rotation = ParseJsonVec3(blob["rotation"]);
				collider->_scale = ParseJsonVec3(blob["scale"]);
				// Allow the derived loading
				collider->FromJson(blob);
				// Mark dirty and store
				collider->_isDirty = true;
				result->_colliders.push_back(collider);
			}
		}

		return result;
	}
}

void RigidBody::_AddColliderToShape(ICollider* collider) {
	// Create the bullet collision shape from the collider
	btCollisionShape* newShape = collider->CreateShape();
	collider->_shape = newShape;

	// We convert our shape parameters to a bullet transform
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(ToBt(collider->_position));
	transform.setRotation(ToBt(glm::quat(glm::radians(collider->_rotation))));
	newShape->setLocalScaling(ToBt(collider->_scale));

	// Add the shape to the compound shape
	_shape->addChildShape(transform, newShape);

	// Remove any existing collision manifolds, so that our body can properly be updated with it's new shape
	if (_body != nullptr) {
		_scene->_physicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(_body->getBroadphaseHandle(), _scene->_physicsWorld->getDispatcher());
	}

	// Our inertia has changed, so flag mass as dirty so it's recalculated
	_isMassDirty = true;
}

void RigidBody::_HandleStateDirty() {
	// If one of our colliders has changed, replace it's shape with it's
	// updated version (and delete the old one)
	for (auto& collider : _colliders) {
		if (collider->_isDirty) {
			// If the collider already had a shape, delete it
			if (collider->_shape != nullptr) {
				_shape->removeChildShape(collider->_shape);
				delete collider->_shape;
			}
			_AddColliderToShape(collider.get());
			collider->_isDirty = false;
		}
	}

	// If the group or mask have changed, notify bullet
	if (_isGroupMaskDirty) {
		_body->getBroadphaseProxy()->m_collisionFilterGroup = _collisionGroup;
		_body->getBroadphaseProxy()->m_collisionFilterMask  = _collisionMask;

		_isGroupMaskDirty = false;
	}
	
	// If our damping parameters have changed, notify Bullet and clear the flag
	if (_isDampingDirty) {
		_body->setDamping(_linearDamping, _angularDamping);
		_isDampingDirty = false;
	}

	// If the mass has changed, we need to notify bullet
	if (_isMassDirty) {
		// Static bodies don't have mass or inertia
		if (_type != RigidBodyType::Static) {
			// Recalulcate our inertia properties and send to bullet
			_shape->calculateLocalInertia(_mass, _inertia);
			_body->setMassProps(_mass, _inertia);
		}
		_isMassDirty = false;
	}
}

