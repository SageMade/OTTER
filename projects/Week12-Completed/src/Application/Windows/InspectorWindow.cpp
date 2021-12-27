#include "InspectorWindow.h"
#include "../Application.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/GlmDefines.h"
#include "Gameplay/Components/ComponentManager.h"

InspectorWindow::InspectorWindow() = default;
InspectorWindow::~InspectorWindow() = default;

void InspectorWindow::Render()
{
	Open = ImGui::Begin("Inspector");

	if (Open) {
		Application& app = Application::Get();

		Gameplay::GameObject::Sptr selection = app.EditorState.SelectedObject.lock();
		if (selection != nullptr) {
			ImGui::PushID(selection.get());

			// Draw a textbox for the object name
			static char nameBuff[256];
			memcpy(nameBuff, selection->Name.c_str(), selection->Name.size());
			nameBuff[selection->Name.size()] = '\0';
			if (ImGui::InputText("##name", nameBuff, 256)) {
				selection->Name = nameBuff;
			}

			ImGui::Separator();

			// Render position label
			selection->_isLocalTransformDirty |= LABEL_LEFT(ImGui::DragFloat3, "Position", &selection->_position.x, 0.01f);

			// Get the ImGui storage state so we can avoid gimbal locking issues by storing euler angles in the editor
			glm::vec3 euler = selection->GetRotationEuler();
			ImGuiStorage* guiStore = ImGui::GetStateStorage();

			// Extract the angles from the storage, note that we're only using the address of the position for unique IDs
			euler.x = guiStore->GetFloat(ImGui::GetID(&selection->_position.x), euler.x);
			euler.y = guiStore->GetFloat(ImGui::GetID(&selection->_position.y), euler.y);
			euler.z = guiStore->GetFloat(ImGui::GetID(&selection->_position.z), euler.z);

			//Draw the slider for angles
			if (LABEL_LEFT(ImGui::DragFloat3, "Rotation", &euler.x, 1.0f)) {
				// Wrap to the -180.0f to 180.0f range for safety
				euler = Wrap(euler, -180.0f, 180.0f);

				// Update the editor state with our new values
				guiStore->SetFloat(ImGui::GetID(&selection->_position.x), euler.x);
				guiStore->SetFloat(ImGui::GetID(&selection->_position.y), euler.y);
				guiStore->SetFloat(ImGui::GetID(&selection->_position.z), euler.z);

				//Send new rotation to the gameobject
				selection->SetRotation(euler);
			}

			// Draw the scale
			selection->_isLocalTransformDirty |= LABEL_LEFT(ImGui::DragFloat3, "Scale   ", &selection->_scale.x, 0.01f, 0.0f);


			ImGui::Separator();

			// Render each component under it's own header
			for (int ix = 0; ix < selection->_components.size(); ix++) {
				std::shared_ptr<Gameplay::IComponent> component = selection->_components[ix];
				if (ImGui::CollapsingHeader(component->ComponentTypeName().c_str())) {
					ImGui::PushID(component.get());
					component->RenderImGui();
					// Render a delete button for the component
					if (ImGuiHelper::WarningButton("Delete")) {
						selection->_components.erase(selection->_components.begin() + ix);
						ix--;
					}
					ImGui::PopID();
				}
			}
			ImGui::Separator();

			// Render a combo box for selecting a component to add
			static std::string preview = "";
			static std::optional<std::type_index> selectedType;
			if (ImGui::BeginCombo("##AddComponents", preview.c_str())) {
				Gameplay::ComponentManager::EachType([&](const std::string& typeName, const std::type_index type) {
					// Hide component types already added
					if (!selection->Has(type)) {
						bool isSelected = typeName == preview;
						if (ImGui::Selectable(typeName.c_str(), &isSelected)) {
							preview = typeName;
							selectedType = type;
						}
					}
				});
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			// Button to add component and reset the selected type
			if (ImGui::Button("Add Component") && selectedType.has_value() && !selection->Has(selectedType.value())) {
				selection->Add(selectedType.value());
				selectedType.reset();
				preview = "";
			}

			ImGui::PopID();
		}
	}
	ImGui::End();
}
