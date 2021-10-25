#include "GameObject.h"

// Utilities
#include "Utils/JsonGlmHelpers.h"

// GLM
#define GLM_ENABLE_EXPERIMENTAL
#include "GLM/gtc/matrix_transform.hpp"
#include "GLM/glm.hpp"
#include "Utils/GlmDefines.h"
#include "Utils/ImGuiHelper.h"

#include "Gameplay/Scene.h"

GameObject::GameObject() :
	Name("Unknown"),
	GUID(Guid::New()),
	Transform(MAT4_IDENTITY),
	Position(ZERO),
	Rotation(ZERO),
	Scale(ONE),
	Components(std::vector<IComponent::Sptr>()),
	_scene(nullptr)
{ }

void GameObject::RecalcTransform() {
	Rotation = glm::fmod(Rotation, glm::vec3(360.0f)); // Wrap all our angles into the 0-360 degree range
	Transform = glm::translate(MAT4_IDENTITY, Position) * glm::mat4_cast(glm::quat(glm::radians(Rotation))) * glm::scale(MAT4_IDENTITY, Scale);
}

Scene* GameObject::GetScene() const {
	return _scene;
}

void GameObject::Awake() {
	for (auto& component : Components) {
		component->Awake(this);
	}
}

void GameObject::Update(float dt) {
	for (auto& component : Components) {
		if (component->IsEnabled) {
			component->Update(this, dt);
		}
	}
}

void GameObject::DrawImGui(float indent) {
	if (ImGui::CollapsingHeader(Name.c_str())) {
		ImGui::PushID(this); // Push a new ImGui ID scope for this object
		ImGui::Indent();
		LABEL_LEFT(ImGui::DragFloat3, "Position", &Position.x, 0.01f);
		LABEL_LEFT(ImGui::DragFloat3, "Rotation", &Rotation.x, 1.0f);
		LABEL_LEFT(ImGui::DragFloat3, "Scale   ", &Scale.x, 0.01f, 0.0f);
		ImGui::Separator();
		ImGui::TextUnformatted("Components");
		ImGui::Separator();
		for (auto& component : Components) {
			if (ImGui::CollapsingHeader(component->ComponentTypeName().c_str())) {
				ImGui::PushID(component.get()); 
				component->RenderImGui(this);
				ImGui::PopID();
			}
		}
		ImGui::Unindent();
		ImGui::PopID(); // Pop the ImGui ID scope for the object
	}
}

GameObject::Sptr GameObject::FromJson(const nlohmann::json& data, Scene* scene)
{
	// We need to manually construct since the GameObject constructor is
	// protected. We can call it here since Scene is a friend class of GameObjects
	GameObject::Sptr result(new GameObject());
	result->_scene = scene;

	// Load in basic info
	result->Name = data["name"];
	result->GUID = Guid(data["guid"]);
	result->Position = ParseJsonVec3(data["position"]);
	result->Rotation = ParseJsonVec3(data["rotation"]);
	result->Scale = ParseJsonVec3(data["scale"]);

	// Since our components are stored based on the type name, we iterate
	// on the keys and values from the components object
	nlohmann::json components = data["components"];
	for (auto& [typeName, value] : components.items()) {
		// We need to reference the component registry to load our components
		// based on the type name (note that all component types need to be
		// registered at the start of the application)
		IComponent::Sptr component = ComponentRegistry::Load(typeName, value);

		// Add component to object and allow it to perform self initialization
		result->Components.push_back(component);
		component->OnLoad(result.get());
	}
	return result;
}

nlohmann::json GameObject::ToJson() const {
	nlohmann::json result = {
		{ "name", Name },
		{ "guid", GUID.str() },
		{ "position", GlmToJson(Position) },
		{ "rotation", GlmToJson(Rotation) },
		{ "scale", GlmToJson(Scale) },
	};
	result["components"] = nlohmann::json();
	for (auto& component : Components) {
		result["components"][component->ComponentTypeName()] = component->ToJson();
		IComponent::SaveBaseJson(component, result["components"][component->ComponentTypeName()]);
	}
	return result;
}
