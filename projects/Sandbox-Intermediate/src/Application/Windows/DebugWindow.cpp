#include "DebugWindow.h"
#include "Application/Application.h"
#include "Application/ApplicationLayer.h"
#include "Application/Layers/RenderLayer.h"

DebugWindow::DebugWindow() :
	IEditorWindow()
{
	Name = "Debug";
	SplitDirection = ImGuiDir_::ImGuiDir_None;
	SplitDepth = 0.5f;
	Requirements = EditorWindowRequirements::Menubar;
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::RenderMenuBar() 
{
	Application& app = Application::Get();
	RenderLayer::Sptr renderLayer = app.GetLayer<RenderLayer>(); 

	BulletDebugMode physicsDrawMode = app.CurrentScene()->GetPhysicsDebugDrawMode();
	if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDrawMode)) { 
		app.CurrentScene()->SetPhysicsDebugDrawMode(physicsDrawMode);
	}

	ImGui::Separator();

	RenderFlags flags = renderLayer->GetRenderFlags();
	bool changed = false;
	bool temp = *(flags & RenderFlags::EnableLights);
	if (ImGui::Checkbox("Enable Lights", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableLights) | (temp ? RenderFlags::EnableLights : RenderFlags::None);
	}

	temp = *(flags & RenderFlags::EnableShadows);
	if (ImGui::Checkbox("Enable Shadows", &temp)) {
		changed = true;
		flags = (flags & ~*RenderFlags::EnableShadows) | (temp ? RenderFlags::EnableShadows : RenderFlags::None);
	}

	if (changed) {
		renderLayer->SetRenderFlags(flags);
	}
}
