#pragma once
#include <memory>
#include "json.hpp"
#include <imgui.h>
#include "Utils/StringUtils.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/ResourceManager/IResource.h"
#include "Utils/TypeHelpers.h"

// We pre-declare GameObject to avoid circular dependencies in the headers
class GameObject;

/// <summary>
/// Base class for components that can be attached to game objects
/// 
/// NOTE:
/// Components must additionally define a static method as such:
/// 
/// static std::shared_ptr<Type> FromJson(const nlohmann::json&);
/// 
/// where Type is the Type of component
/// </summary>
class IComponent : public IResource {
public:
	typedef std::shared_ptr<IComponent> Sptr;

	/// <summary>
	/// True when this component is enabled and should perform update and 
	/// renders
	/// </summary>
	bool IsEnabled;

	virtual ~IComponent() = default;

	/// <summary>
	/// Invoked when a component has been added to a game object, note that this function
	/// should only perform local setup (i.e never look for game objects or other components)
	/// </summary>
	/// <param name="context">The game object that the component belongs to</param>
	virtual void OnLoad(GameObject* context) { };
	/// <summary>
	/// Invoked when the scene has finished loading, and all objects and components
	/// are set up
	/// </summary>
	/// <param name="context">The game object that the component belongs to</param>
	virtual void Awake(GameObject* context) { };

	/// <summary>
	/// Invoked during the update loop
	/// </summary>
	/// <param name="context">The game object that the component belongs to</param>
	/// <param name="deltaTime">The time since the last frame, in seconds</param>
	virtual void Update(GameObject* context, float deltaTime) {};

	/// <summary>
	/// All components should override this to allow us to render component
	/// info in ImGui for easy editing
	/// </summary>
	/// <param name="context">The game object that the component belongs to</param>
	virtual void RenderImGui(GameObject* context) = 0;

	/// <summary>
	/// Returns the component's type name
	/// To override in child classes, use MAKE_TYPENAME(Type) instead of
	/// manually implementing, otherwise serialization may have isues
	/// </summary>
	virtual std::string ComponentTypeName() const = 0;


	static inline void LoadBaseJson(const IComponent::Sptr& result, const nlohmann::json& blob) {
		result->OverrideGUID(Guid(blob["guid"]));
		result->IsEnabled = blob["enabled"];
	}
	static inline void SaveBaseJson(const IComponent::Sptr& instance, nlohmann::json& data) {
		data["guid"] = instance->GetGUID().str();
		data["enabled"] = instance->IsEnabled;
	}


protected:
	IComponent() : IsEnabled(true) { }
};

/// <summary>
/// Returns true if the given type is a valid component type
/// </summary>
/// <typeparam name="T">The type to check</typeparam>
template <typename T>
constexpr bool is_valid_component() {
	return std::is_base_of<IComponent, T>::value && test_json<T, const nlohmann::json&>::value;
}

// Defines the ComponentTypeName interface to match those used elsewhere by other systems
#define MAKE_TYPENAME(T) \
	inline virtual std::string ComponentTypeName() const { \
		static std::string name = StringTools::SanitizeClassName(typeid(T).name()); return name; }