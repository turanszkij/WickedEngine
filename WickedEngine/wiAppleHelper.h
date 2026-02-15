#pragma once
#include "wiMath.h"

#include <string>

// The purpose of this helper is to expose functionality that is specific to Apple platform and must be implemented in Objective C++

namespace wi::apple
{
void SetMetalLayerToWindow(void* window, void* layer);
void* GetViewFromWindow(void* window);
XMUINT2 GetWindowSizeNoScaling(void* handle);
XMUINT2 GetWindowSize(void* handle);
float GetDPIForWindow(void* handle);
XMFLOAT2 GetMousePositionInWindow(void* handle);
void SetMousePositionInWindow(void* window, XMFLOAT2 value);
int MessageBox(const char* title, const char* message, const char* buttons = nullptr);
std::string GetExecutablePath();
void CursorInit(void** cursor_table);
void CursorSet(void* cursor);
void CursorHide(bool hide);
void SetWindowFullScreen(void* handle, bool fullscreen);
bool IsWindowFullScreen(void* handle);
void OpenUrl(const char* url);
std::string GetClipboardText();
void SetClipboardText(const char* str);
void* CreateCursorFromARGB8ImageData(const void* data, uint32_t width, uint32_t height, int hotspotX, int hotspotY);
}
