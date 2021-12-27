#pragma once

#include "Utils/Macros.h"

/**
 * The editor window interface allows us to separate implementation for 
 * editor or debug windows
 */
class IEditorWindow {
public:
	MAKE_PTRS(IEditorWindow);
	NO_COPY(IEditorWindow);
	NO_MOVE(IEditorWindow);

	bool Open = true;

	virtual ~IEditorWindow() =default;

	virtual void Render() = 0;

protected:
	IEditorWindow() = default;
};