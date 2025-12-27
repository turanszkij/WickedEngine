#pragma once
#include "wiMath.h"

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

namespace wi::apple
{
MTK::View* GetMTKViewFromWindow(NS::Window* window);
XMUINT2 GetWindowSize(NS::Window* handle);
float GetDPIForWindow(NS::Window* handle);
XMFLOAT2 GetMousePositionInWindow(NS::Window* handle);
}
