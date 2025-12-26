#include "wiAppleHelper.h"

#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>
#include <MetalKit/MTKView.h>

namespace wi::apple
{

MTK::View* GetMTKViewFromWindow(NS::Window* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window)
		return nullptr;

	NSView* contentView = window.contentView;
	if ([contentView isKindOfClass:[MTKView class]]) {
		return (__bridge MTK::View*)contentView;
	}
	return nullptr;
}

XMUINT2 GetWindowSize(NS::Window* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window)
		return XMUINT2(0, 0);
	
	NSView* contentView = window.contentView;
	if (!contentView)
		return XMUINT2(0, 0);
	
	// Case 1: contentView is directly an MTKView (most common in Metal apps)
	if ([contentView isKindOfClass:[MTKView class]])
	{
		MTKView* mtkView = (MTKView*)contentView;
		CGSize drawableSize = mtkView.drawableSize;
		return XMUINT2((uint32_t)drawableSize.width, (uint32_t)drawableSize.height);
	}
	
	// Case 2: Search for an MTKView in the subview hierarchy
	__block MTKView* foundMTKView = nil;
	[contentView.subviews enumerateObjectsUsingBlock:^(NSView* subview, NSUInteger idx, BOOL* stop) {
		if ([subview isKindOfClass:[MTKView class]]) {
			foundMTKView = (MTKView*)subview;
			*stop = YES;
		}
	}];
	
	if (foundMTKView)
	{
		CGSize drawableSize = foundMTKView.drawableSize;
		return XMUINT2((uint32_t)drawableSize.width, (uint32_t)drawableSize.height);
	}
	
	// Fallback: Return the backing pixel size of the content view
	// (useful if using CAMetalLayer directly or custom setup)
	CGRect bounds = contentView.bounds;
	CGFloat scale = window.backingScaleFactor;
	uint32_t pixelWidth  = (uint32_t)(bounds.size.width * scale);
	uint32_t pixelHeight = (uint32_t)(bounds.size.height * scale);
	
	return XMUINT2(pixelWidth, pixelHeight);
}
float GetDPIForWindow(NS::Window* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window || !window.screen)
		return 96.0f; // Fallback

	NSScreen* screen = window.screen;

	NSDictionary* desc = [screen deviceDescription];
	
	// Get logical size in points
	NSValue* sizeValue = [desc objectForKey:NSDeviceSize];
	if (!sizeValue)
		return 96.0f;
	NSSize logicalSize = [sizeValue sizeValue];

	// Get CGDisplay ID
	NSNumber* screenNumber = [desc objectForKey:@"NSScreenNumber"];
	if (!screenNumber)
		return 96.0f;
	CGDirectDisplayID displayID = [screenNumber unsignedIntValue];

	// Physical size in mm
	CGSize physicalSizeMM = CGDisplayScreenSize(displayID);
	if (physicalSizeMM.width <= 0 || physicalSizeMM.height <= 0)
		return 96.0f;

	// Backing scale (1.0 or 2.0 usually)
	CGFloat scale = screen.backingScaleFactor;

	// Native pixel width
	double pixelWidth = logicalSize.width * scale;
	double inchesWidth = physicalSizeMM.width / 25.4;

	return (inchesWidth > 0) ? (float)(pixelWidth / inchesWidth) : 96.0f;
}

XMFLOAT2 GetMousePositionInWindow(NS::Window* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	MTKView* mtkView = (__bridge MTKView*)GetMTKViewFromWindow(handle);
	if (!window || !mtkView)
		return XMFLOAT2(-1.f, -1.f);

	NSPoint mouseLocScreen = [NSEvent mouseLocation];
	NSPoint mouseInWindow = [window convertRectFromScreen:NSMakeRect(mouseLocScreen.x, mouseLocScreen.y, 0, 0)].origin;
	NSPoint mouseInView = [mtkView convertPoint:mouseInWindow fromView:nil];

	// Flip Y
	CGFloat heightPoints = mtkView.bounds.size.height;
	mouseInView.y = heightPoints - mouseInView.y;

	// Use the actual drawable scale (most accurate)
	CGFloat scale = mtkView.window.backingScaleFactor;

	return XMFLOAT2((float)(mouseInView.x * scale), (float)(mouseInView.y * scale));
}

}
