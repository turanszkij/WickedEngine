#pragma once
#include "wiMath.h"

#include <string>

// The purpose of this helper is to expose functionality that is specific to Apple platform and must be implemented in Objective C++

namespace wi::apple
{
void SetMetalLayerToWindow(void* window, void* layer);
void* GetViewFromWindow(void* window);
XMUINT2 GetWindowSize(void* handle);
float GetDPIForWindow(void* handle);
XMFLOAT2 GetMousePositionInWindow(void* handle);
int MessageBox(const char* title, const char* message, const char* buttons = nullptr);
std::string GetExecutablePath();
void CursorInit(void** cursor_table);
void CursorSet(void* cursor);
void CursorHide(bool hide);
}
