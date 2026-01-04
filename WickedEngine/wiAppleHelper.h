#pragma once
#include "wiMath.h"

#include <string>

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

// The purpose of this helper is to expose functionality that is specific to Apple platform and must be implemented in Objective C++

namespace wi::apple
{
void SetMetalLayerToWindow(NS::Window* window, CA::MetalLayer* layer);
NS::View* GetViewFromWindow(NS::Window* window);
XMUINT2 GetWindowSize(NS::Window* handle);
float GetDPIForWindow(NS::Window* handle);
XMFLOAT2 GetMousePositionInWindow(NS::Window* handle);
int MessageBox(const char* title, const char* message, const char* buttons = nullptr);
std::string GetExecutablePath();
void CursorInit(void** cursor_table);
void CursorSet(void* cursor);
void CursorHide(bool hide);
}
