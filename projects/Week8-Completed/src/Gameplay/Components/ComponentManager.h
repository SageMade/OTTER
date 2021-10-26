#pragma once
#include <functional>
#include "IComponent.h"
#include <typeindex>
#include <optional>

/// <summary>
/// Helper class for component types, this class is what lets us load component types
/// from scene files, as well as providing a way to iterate over all active components
/// of a given type (and sort them!)
/// </summary>
class ComponentManager {
public:
	typedef std::function<IComponent::Sptr(const nlohmann::json&)> LoadComponentFunc;

	/// <summary>
	/// Loads a component with the given type name from a JSON blob
	/// If the type name does not correspond to a registered type, will
	/// return nullptr
	/// </summary>
	/// <param name="typeName">The name of the type to load (taken from GetComponentTypeName of component)</param>
	/// <param name="blob">The JSON blob to decode</param>
	/// <returns>The component as decoded from the JSON data, or nullptr</returns>
	static IComponent::Sptr Load(const std::string& typeName, const nlohmann::json& blob) {
		// Try and get the type index from the name
		std::optional<std::type_index> typeIndex = _TypeNameMap[typeName];

		// If we have a value for type index, this component type was registered!
		if (typeIndex.has_value()) {
			// Get the load callback and make sure it exists
			LoadComponentFunc callback = _TypeLoadRegistry[typeIndex.value()];
			if (callback) {
				// Invoke the loader, also load additional component data
				IComponent::Sptr result = callback(blob);
				IComponent::LoadBaseJson(result, blob);

				// Make sure the component knows it's own type
				result->_realType = typeIndex.value();

				// Add the component to the global pools
				_Components[result->_realType].push_back(result);
				return result;
			}
		}
		return nullptr;
	}

	/// <summary>
	/// Creates a new component and adds it to the global component pools
	/// </summary>
	/// <typeparam name="ComponentType">Type type of component to create</typeparam>
	/// <typeparam name="...TArgs">The types of params to forward to the component's constructor</typeparam>
	/// <param name="...args">The arguments to forward to the constructor</param>
	/// <returns>The new component that has been created</returns>
	template <
		typename ComponentType, 
		typename ... TArgs, 
		typename = typename std::enable_if<std::is_base_of<IComponent, ComponentType>::value>::type>
	static std::shared_ptr<ComponentType> Create(TArgs&& ... args) {
		// We can use typeid and type_index to get a unique ID for our types
		std::type_index type = std::type_index(typeid(ComponentType));
		LOG_ASSERT(_TypeLoadRegistry[type] != nullptr, "You must register component types before creating them!");

		// Create component, forwarding arguments
		std::shared_ptr<ComponentType> component = std::make_shared<ComponentType>(std::forward<TArgs>(args)...);

		// Make sure the component knows it's concrete type
		component->_realType = type;

		// Add to global component list for that type
		_Components[type].push_back(component);
		// Return the result
		return component;
	}

	/// <summary>
	/// Searches for a component with the given GUID, allowing components to cross reference each other
	/// and survive scene serialization
	/// </summary>
	/// <typeparam name="ComponentType">The type of component to get</typeparam>
	/// <param name="id">The unique ID of the component to get</param>
	/// <returns>The component with the given ID, or nullptr if it does not exist</returns>
	template <
		typename ComponentType,
		typename = typename std::enable_if<std::is_base_of<IComponent, ComponentType>::value>::type>
	static std::shared_ptr<ComponentType> GetComponentByGUID(Guid id) {
		// We can use typeid and type_index to get a unique ID for our types
		std::type_index type = std::type_index(typeid(ComponentType));
		LOG_ASSERT(_TypeLoadRegistry[type] != nullptr, "You must register component types before creating them!");

		// Search the component store for a component that matches that ID
		auto& it = std::find_if(_Components[type].begin(), _Components[type].end(), [&](const std::shared_ptr<IComponent>& ptr) {
			return ptr->GetGUID() == id;
		});

		// If the component was found, return it. Otherwise return nullptr
		if (it != _Components[type].end()) {
			return *it;
		} else {
			return nullptr;
		}
	}

	template <
		typename ComponentType,
		typename = typename std::enable_if<std::is_base_of<IComponent, ComponentType>::value>::type>
	static void Each(std::function<void(const std::shared_ptr<ComponentType>&)> callback, bool includeDisabled = false) {
		// We can use typeid and type_index to get a unique ID for our types
		std::type_index type = std::type_index(typeid(ComponentType));
		LOG_ASSERT(_TypeLoadRegistry[type] != nullptr, "You must register component types before creating them!");

		for (auto& sptr : _Components[type]) {
			if (sptr->IsEnabled | includeDisabled) {
				// Upcast to component type and invoke the callback
				callback(std::dynamic_pointer_cast<ComponentType>(sptr));
			}
		}
	}

	/// <summary>
	/// Removes a given component from the global pools. To be used in the IComponent destructor
	/// </summary>
	/// <typeparam name="ComponentType">The type of component to destroy</typeparam>
	/// <param name="component">A raw pointer to the component to remove (should be called from IComponent destructor)</param>
	/// <returns>True if the element was removed, false if not</returns>
	inline static bool Remove(const IComponent* component) {
		// Make sure the component's type was one that was registered
		LOG_ASSERT(_TypeLoadRegistry[component->_realType] != nullptr, "You must register component types before creating them!");

		// Search the component registry for the component
		auto& it = std::find_if(_Components[component->_realType].begin(), _Components[component->_realType].end(), [&](const std::shared_ptr<IComponent>& ptr) {
			return ptr.get() == component;
		});
		// If it was found, remove it and return true
		if (it != _Components[component->_realType].end()) {
			_Components[component->_realType].erase(it);
			return true;
		} else {
			return false;
		}
	}

	/// <summary>
	/// Attempts to register a given type as a component, should be called for each component type 
	/// at the start of you application
	/// </summary>
	/// <typeparam name="T">The type to register, should extend the IComponent interface and have appropriate static methods</typeparam>
	template <typename T>
	static void RegisterType() {
		// Make sure the component type is valid (see bottom of IComponent.h)
		static_assert(is_valid_component<T>(), "Type is not a valid component type!");

		// We use the type ID to map types to the underlying helpers
		std::type_index type(typeid(T));

		// if type NOT registered
		if (_TypeLoadRegistry.find(type) == _TypeLoadRegistry.end()) {
			// Store the loading function in the registry, as well as the
			// name to type index mapping
			_TypeLoadRegistry[type] = &ComponentManager::ParseTypeFromBlob<T>;
			_TypeNameMap[StringTools::SanitizeClassName(typeid(T).name())] = type;
		}
	}

private:
	inline static std::unordered_map<std::string, std::optional<std::type_index>> _TypeNameMap;
	inline static std::unordered_map<std::type_index, LoadComponentFunc> _TypeLoadRegistry;

	inline static std::unordered_map<std::type_index, std::vector<IComponent::Sptr>> _Components;

	template <typename T>
	static IComponent::Sptr ParseTypeFromBlob(const nlohmann::json& blob) {
		return T::FromJson(blob);
	}
};