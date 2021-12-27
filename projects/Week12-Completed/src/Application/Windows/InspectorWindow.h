#pragma once
#include "../IEditorWindow.h"

class InspectorWindow final : public IEditorWindow {
public:
	MAKE_PTRS(InspectorWindow);
	InspectorWindow();
	virtual ~InspectorWindow();

	virtual void Render() override;

protected:
};