#include "HierarchyWindow.h"
#include "../Application.h"
#include "../../Week9-Completed/src/Utils/ImGuiHelper.h"
#include "imgui_internal.h"

HierarchyWindow::HierarchyWindow() :
	IEditorWindow()
{

}

HierarchyWindow::~HierarchyWindow() = default;

void HierarchyWindow::Render()
{
	Open = ImGui::Begin("Hierarchy");

	if (Open) {
		Application& app = Application::Get();
		
		for (const auto& object : app.CurrentScene()->_objects) {
			_RenderObjectNode(object);
		}
	}

	ImGui::End();
}

void HierarchyWindow::_RenderObjectNode(Gameplay::GameObject::Sptr object, int depth) {
	using namespace Gameplay;

	if (object->GetParent() != nullptr && depth == 0) {
		return;
	}

	Application& app = Application::Get();

	ImGui::PushID(object->GetGUID().str().c_str());

	// Figure out how the object node should be displayed
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	GameObject::Sptr selectedObject = app.EditorState.SelectedObject.lock();

	if (selectedObject != nullptr && selectedObject == object) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (object->GetChildren().size() == 0) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	// Determine the text of the node
	static char buffer[256];
	sprintf_s(buffer, 256, "%s###GO_HEADER", object->Name.c_str());
	bool isOpen = ImGui::TreeNodeEx(buffer, flags);
	if (ImGui::IsItemClicked()) {
		// TODO: Properly handle multi-selection
		//ImGuiIO& io = ImGui::GetIO();
		//if (!io.KeyShift) {
		//	_selectedObjects.clear();
		//}

		app.EditorState.SelectedObject = object;
	}

	ImGuiID deletePopup = ImGui::GetID("Delete Gameobject###HIERARCHY_DELETE");

	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Add Child")) {
			object->AddChild(object->GetScene()->CreateGameObject("GameObject"));
		}
		if (ImGui::MenuItem("Delete")) {
			ImGui::OpenPopupEx(deletePopup);
		}

		ImGui::EndPopup();
	}

	// Draw our delete modal
	if (ImGui::BeginPopupModal("Delete Gameobject###HIERARCHY_DELETE")) {
		ImGui::Text("Are you sure you want to delete this game object?");
		if (ImGuiHelper::WarningButton("Yes")) {
			// Remove ourselves from the scene
			object->GetScene()->RemoveGameObject(object);

			// Restore imgui state so we can early bail
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			ImGui::PopID();
			if (isOpen) {
				ImGui::TreePop();
			}
			return;
		}
		ImGui::SameLine();
		if (ImGui::Button("No")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();

	}

	// If the node is expanded, render the child nodes
	if (isOpen) {
		for (const auto& child : object->GetChildren()) {
			_RenderObjectNode(child, depth + 1);
		}
		ImGui::TreePop();
	}


	ImGui::PopID();
}
