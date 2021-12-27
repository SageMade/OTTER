#pragma once
#include "../IEditorWindow.h"
#include "Gameplay/GameObject.h"

class HierarchyWindow : public IEditorWindow {
public:
	MAKE_PTRS(HierarchyWindow)

	HierarchyWindow();
	virtual ~HierarchyWindow();

	virtual void Render() override;

protected:
	void _RenderObjectNode(Gameplay::GameObject::Sptr object, int depth = 0);
};