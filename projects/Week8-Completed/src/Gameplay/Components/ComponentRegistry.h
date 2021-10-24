#pragma once
#include <functional>
#include "IComponent.h"
#include <typeindex>
#include <optional>

/// <summary>
/// Helper class for component types, this class is what lets us load component types
/// from scene files
/// </summary>
class ComponentRegistry {
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
		std::optional<std::type_index> typeIndex = _TypeNameMap[typeName];
		if (typeIndex.has_value()) {
			LoadComponentFunc callback = _TypeLoadRegistry[typeIndex.value()];
			if (callback) {
				IComponent::Sptr result = callback(blob);
				IComponent::LoadBaseJson(result, blob);
				return result;
			}
		}
		return nullptr;
	}

	/// <summary>
	/// Attempts to register a given type as a component, should be called for each component type 
	/// at the start of you application
	/// </summary>
	/// <typeparam name="T">The type to register, should extend the IComponent interface and have appropriate static methods</typeparam>
	template <typename T>
	static void TryRegisterType() {
		// Make sure the component type is valid (see bottom of IComponent.h)
		static_assert(is_valid_component<T>(), "Type is not a valid component type!");
		// We use the type ID to map types to the underlying helpers
		std::type_index type(typeid(T));
		// if type NOT registered
		if (_TypeLoadRegistry.find(type) == _TypeLoadRegistry.end()) {
			// Store the loading function in the registry, as well as the
			// name to type index mapping
			_TypeLoadRegistry[type] = &ComponentRegistry::ParseTypeFromBlob<T>;
			_TypeNameMap[StringTools::SanitizeClassName(typeid(T).name())] = type;
		}
	}
private:
	inline static std::unordered_map<std::string, std::optional<std::type_index>> _TypeNameMap;
	inline static std::unordered_map<std::type_index, LoadComponentFunc> _TypeLoadRegistry;

	template <typename T>
	static IComponent::Sptr ParseTypeFromBlob(const nlohmann::json& blob) {
		return T::FromJson(blob);
	}
};