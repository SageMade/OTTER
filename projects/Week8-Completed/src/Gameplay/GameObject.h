#pragma once
#include <string>

// Utils
#include "Utils/GUID.hpp"

// GLM
#define GLM_ENABLE_EXPERIMENTAL
#include "GLM/gtx/common.hpp"

// Others
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/ComponentManager.h"

// Predeclaration for Scene
class Scene;

/// <summary>
/// Represents an object in our scene with a transformation and a collection
/// of components. Components provide gameobject's with behaviours
/// </summary>
struct GameObject {
	typedef std::shared_ptr<GameObject> Sptr;

	// Human readable name for the object
	std::string             Name;
	// Unique ID for the object
	Guid                    GUID;
	// The object's world transform
	glm::mat4               Transform;

	// The components that this game object has attached to it
	std::vector<IComponent::Sptr> Components;

	// Position of the object
	glm::vec3 Position;
	// Rotation of the object in Euler angler
	glm::vec3 Rotation;
	// The scale of the object
	glm::vec3 Scale;

	/// <summary>
	/// Recalculates the object's transformation (T * R * S)
	/// </summary>
	void RecalcTransform();

	/// <summary>
	/// Returns a pointer to the scene that this GameObject belongs to
	/// </summary>
	Scene* GetScene() const;

	/// <summary>
	/// Notify all enabled components in this gameObject that the scene has been loaded
	/// </summary>
	void Awake();

	/// <summary>
	/// Calls update on all enabled components in this object
	/// </summary>
	/// <param name="deltaTime">The time since the last frame, in seconds</param>
	void Update(float dt);

	/// <summary>
	/// Checks whether this gameobject has a component of the given type
	/// </summary>
	/// <typeparam name="T">The type of component to search for</typeparam>
	template <typename T, typename = typename std::enable_if<std::is_base_of<IComponent, T>::value>::type>
	bool Has() {
		// Iterate over all the pointers in the components list
		for (const auto& ptr : Components) {
			// If the pointer type matches T, we return true
			if (std::type_index(typeid(*ptr.get())) == std::type_index(typeid(T))) {
				return true;
			}
		}
		return false;
	}

	/// <summary>
	/// Gets the component of the given type from this gameobject, or nullptr if it does not exist
	/// </summary>
	/// <typeparam name="T">The type of component to search for</typeparam>
	template <typename T, typename = typename std::enable_if<std::is_base_of<IComponent, T>::value>::type>
	std::shared_ptr<T> Get() {
		// Iterate over all the pointers in the binding list
		for (const auto& ptr : Components) {
			// If the pointer type matches T, we return that behaviour, making sure to cast it back to the requested type
			if (std::type_index(typeid(*ptr.get())) == std::type_index(typeid(T))) {
				return std::dynamic_pointer_cast<T>(ptr);
			}
		}
		return nullptr;
	}

	/// <summary>
	/// Adds a component of the given type to this gameobject. Note that only one component
	/// of a given type may be attached to a gameobject
	/// </summary>
	/// <typeparam name="T">The type of component to add</typeparam>
	/// <typeparam name="TArgs">The arguments to forward to the component constructor</typeparam>
	template <typename T, typename ... TArgs>
	std::shared_ptr<T> Add(TArgs&&... args) {
		static_assert(is_valid_component<T>(), "Type is not a valid component type!");
		LOG_ASSERT(!Has<T>(), "Cannot add 2 instances of a component type to a game object");

		// Make a new component, forwarding the arguments
		std::shared_ptr<T> component = ComponentManager::Create<T>(std::forward<TArgs>(args)...);
		component->_context = this;

		// Append it to the binding component's storage, and invoke the OnLoad
		Components.push_back(component);
		component->OnLoad();

		return component;
	}

	/// <summary>
	/// Draws the ImGui window for this game object and all nested components
	/// </summary>
	void DrawImGui(float indent = 0.0f);

	/// <summary>
	/// Loads a render object from a JSON blob
	/// </summary>
	static GameObject::Sptr FromJson(const nlohmann::json& data, Scene* scene);
	/// <summary>
	/// Converts this object into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const;

private:
	friend class Scene;

	// Pointer to the scene, we use raw pointers since 
	// this will always be set by the scene on creation
	// or load, we don't need to worry about ref counting
	Scene* _scene;

	/// <summary>
	/// Only scenes will be allowed to create gameobjects
	/// </summary>
	GameObject();
};