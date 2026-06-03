#include "wiAppleHelper.h"
#include "wiHelper.h"
#include "wiInput.h"

#ifdef PLATFORM_MACOS
#include <AppKit/AppKit.h>
#include <CoreGraphics/CoreGraphics.h>

#include <mach-o/dyld.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

namespace wi::apple
{

void SetMetalLayerToWindow(void* _window, void* _layer)
{
	NSWindow* window = (__bridge NSWindow*)_window;
	if (!window)
		return;
	CAMetalLayer* layer = (__bridge CAMetalLayer*)_layer;
	if (!layer)
		return;
	window.contentView.wantsLayer = true;
	window.contentView.layer = layer;
}

void* GetViewFromWindow(void* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window)
		return nullptr;

	NSView* contentView = window.contentView;
	return (__bridge void*)contentView;
}

XMUINT2 GetWindowSizeNoScaling(void* handle)
{
	if (!handle)
		return XMUINT2(0, 0);

	NSWindow* window = (__bridge NSWindow*)handle;
	NSView* contentView = window.contentView;
	if (!contentView)
		return XMUINT2(0, 0);

	CGRect bounds = contentView.bounds;

	uint32_t pixelWidth  = (uint32_t)round(bounds.size.width);
	uint32_t pixelHeight = (uint32_t)round(bounds.size.height);

	return XMUINT2(pixelWidth, pixelHeight);
}
XMUINT2 GetWindowSize(void* handle)
{
	if (!handle)
		return XMUINT2(0, 0);

	NSWindow* window = (__bridge NSWindow*)handle;
	NSView* contentView = window.contentView;
	if (!contentView)
		return XMUINT2(0, 0);

	CGRect bounds = contentView.bounds;
	CGFloat scale = window.backingScaleFactor;  // 1.0 on non-Retina, 2.0 on Retina

	uint32_t pixelWidth  = (uint32_t)round(bounds.size.width * scale);
	uint32_t pixelHeight = (uint32_t)round(bounds.size.height * scale);

	return XMUINT2(pixelWidth, pixelHeight);
}

float GetDPIForWindow(void* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window || !window.screen)
		return 96.0f;
	CGFloat scale = window.screen.backingScaleFactor;
	return scale * 96.0f;
}

XMFLOAT2 GetMousePositionInWindow(void* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	NSView* view = (__bridge NSView*)GetViewFromWindow(handle);
	if (!window || !view)
		return XMFLOAT2(-1.f, -1.f);
	NSPoint mouseLocScreen = [NSEvent mouseLocation];
	NSPoint mouseInWindow = [window convertRectFromScreen:NSMakeRect(mouseLocScreen.x, mouseLocScreen.y, 0, 0)].origin;
	NSPoint mouseInView = [view convertPoint:mouseInWindow fromView:nil];
	CGFloat heightPoints = view.bounds.size.height;
	mouseInView.y = heightPoints - mouseInView.y;
	CGFloat scale = view.window.backingScaleFactor;
	return XMFLOAT2((float)(mouseInView.x * scale), (float)(mouseInView.y * scale));
}
void SetMousePositionInWindow(void* handle, XMFLOAT2 value)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (!window)
		return;
	NSView* view = window.contentView;
	if (!view)
		return;
	CGFloat scale = view.window.backingScaleFactor;
	CGFloat pixelX = value.x;
	CGFloat pixelY = value.y;
	CGFloat pointX = pixelX / scale;
	CGFloat pointY = pixelY / scale;
	CGFloat heightPoints = view.bounds.size.height;
	CGFloat flippedY = heightPoints - pointY;
	NSPoint localPointInView = NSMakePoint(pointX, flippedY);
	NSPoint pointInWindow = [view convertPoint:localPointInView toView:nil];
	NSPoint pointOnScreen = [window convertPointToScreen:pointInWindow];
	NSScreen* screen = window.screen;
	CGFloat screenHeight = screen.frame.size.height;
	pointOnScreen.y = screenHeight - pointOnScreen.y;
	CGWarpMouseCursorPosition(pointOnScreen);
}

int MessageBox(const char* title, const char* message, const char* buttons)
{
	@autoreleasepool {
		NSAlert *alert = [[NSAlert alloc] init];
		[alert setMessageText:@(title)];
		[alert setInformativeText:@(message)];
		[alert setAlertStyle:NSAlertStyleInformational];
		
		if (buttons == nullptr)
		{
			[alert addButtonWithTitle:@"OK"];
			[alert runModal];
		}
		else if (strcmp(buttons, "YesNo") == 0)
		{
			[alert addButtonWithTitle:@"Yes"];
			[alert addButtonWithTitle:@"No"];
			NSInteger button = [alert runModal];
			switch (button) {
				case NSAlertFirstButtonReturn:  return (int)wi::helper::MessageBoxResult::Yes;
				case NSAlertSecondButtonReturn: return (int)wi::helper::MessageBoxResult::No;
				default:  return (int)wi::helper::MessageBoxResult::Cancel;
			}
		}
		else if (strcmp(buttons, "YesNoCancel") == 0)
		{
			[alert addButtonWithTitle:@"Yes"];
			[alert addButtonWithTitle:@"No"];
			[alert addButtonWithTitle:@"Cancel"];
			NSInteger button = [alert runModal];
			switch (button) {
				case NSAlertFirstButtonReturn:  return (int)wi::helper::MessageBoxResult::Yes;
				case NSAlertSecondButtonReturn: return (int)wi::helper::MessageBoxResult::No;
				default:  return (int)wi::helper::MessageBoxResult::Cancel;
			}
		}
		else if (strcmp(buttons, "OKCancel") == 0)
		{
			[alert addButtonWithTitle:@"OK"];
			[alert addButtonWithTitle:@"Cancel"];
			NSInteger button = [alert runModal];
			switch (button) {
				case NSAlertFirstButtonReturn:  return (int)wi::helper::MessageBoxResult::OK;
				default:  return (int)wi::helper::MessageBoxResult::Cancel;
			}
		}
		else if (strcmp(buttons, "AbortRetryIgnore") == 0)
		{
			[alert addButtonWithTitle:@"Abort"];
			[alert addButtonWithTitle:@"Retry"];
			[alert addButtonWithTitle:@"Ignore"];
			NSInteger button = [alert runModal];
			switch (button) {
				case NSAlertFirstButtonReturn:  return (int)wi::helper::MessageBoxResult::Abort;
				case NSAlertSecondButtonReturn:  return (int)wi::helper::MessageBoxResult::Retry;
				default:  return (int)wi::helper::MessageBoxResult::Ignore;
			}
		}

	}
	return (int)wi::helper::MessageBoxResult::OK;
}

std::string GetExecutablePath()
{
	char rawPath[PATH_MAX];
	uint32_t rawLength = sizeof(rawPath);
	
	if (_NSGetExecutablePath(rawPath, &rawLength) != 0)
		return {};
	
	char* resolved = realpath(rawPath, nullptr);
	if (!resolved)
		return {};
	
	std::string path(resolved);
	free(resolved);
	return path;
}

void CursorInit(void** cursor_table)
{
	cursor_table[wi::input::CURSOR_DEFAULT] = (__bridge void*)[NSCursor arrowCursor];
	cursor_table[wi::input::CURSOR_TEXTINPUT] = (__bridge void*)[NSCursor IBeamCursor];
	cursor_table[wi::input::CURSOR_RESIZEALL] = (__bridge void*)[NSCursor closedHandCursor];
	cursor_table[wi::input::CURSOR_RESIZE_NS] = (__bridge void*)[NSCursor resizeUpDownCursor];
	cursor_table[wi::input::CURSOR_RESIZE_EW] = (__bridge void*)[NSCursor resizeLeftRightCursor];
	cursor_table[wi::input::CURSOR_RESIZE_NESW] = (__bridge void*)[NSCursor closedHandCursor];
	cursor_table[wi::input::CURSOR_RESIZE_NWSE] = (__bridge void*)[NSCursor closedHandCursor];
	cursor_table[wi::input::CURSOR_HAND] = (__bridge void*)[NSCursor pointingHandCursor];
	cursor_table[wi::input::CURSOR_NOTALLOWED] = (__bridge void*)[NSCursor operationNotAllowedCursor];
}
void CursorSet(void* cursor)
{
	NSCursor* _cursor = (__bridge NSCursor*)cursor;
	[_cursor set];
}
void CursorHide(bool hide)
{
	static bool hidden = false;
	if (hide)
	{
		if (!hidden)
		{
			hidden = true;
			[NSCursor hide];
		}
	}
	else
	{
		hidden = false;
		[NSCursor unhide];
	}
}

void SetWindowFullScreen(void* handle, bool fullscreen)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (window == nil)
		return;
	bool isCurrentlyFullscreen = (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
	if (isCurrentlyFullscreen == fullscreen)
		return;
	if (fullscreen)
	{
		NSWindowCollectionBehavior currentBehavior = window.collectionBehavior;
		if ((currentBehavior & NSWindowCollectionBehaviorFullScreenPrimary) == 0 &&
			(currentBehavior & NSWindowCollectionBehaviorFullScreenAuxiliary) == 0) {
			window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
		}
	}
	[window toggleFullScreen:nil];
}

bool IsWindowFullScreen(void* handle)
{
	NSWindow* window = (__bridge NSWindow*)handle;
	if (window == nil)
		return false;
	return (window.styleMask & NSWindowStyleMaskFullScreen) != 0;
}

void OpenUrl(const char* url)
{
	@autoreleasepool
	{
		NSString* nsurl = [NSString stringWithUTF8String:url];
		NSURL* nsURL = [NSURL URLWithString:nsurl];
		
		if (nsURL)
		{
			[[NSWorkspace sharedWorkspace] openURL:nsURL];
		}
	}
}

std::string GetClipboardText()
{
	std::string ret;
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	NSString* available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:NSPasteboardTypeString]];
	if (![available isEqualToString:NSPasteboardTypeString])
		return ret;
	NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
	if (string == nil)
		return ret;
	const char* string_c = (const char*)[string UTF8String];
	size_t string_len = strlen(string_c);
	ret.resize((int)string_len);
	strcpy(ret.data(), string_c);
	return ret;
}
void SetClipboardText(const char* str)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
}

void* CreateCursorFromARGB8ImageData(const void* data, uint32_t width, uint32_t height, int hotspotX, int hotspotY)
{
	const size_t bytesPerRow = width * sizeof(uint32_t);
	NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
								initWithBitmapDataPlanes: nullptr
								pixelsWide: width
								pixelsHigh: height
								bitsPerSample: 8
								samplesPerPixel: 4
								hasAlpha: YES
								isPlanar: NO
								colorSpaceName: NSDeviceRGBColorSpace
								bitmapFormat: NSBitmapFormatAlphaNonpremultiplied | NSBitmapFormatAlphaFirst
								bytesPerRow: bytesPerRow
								bitsPerPixel: 32];
	
	uint8_t* dst = [bitmap bitmapData];
	size_t bytesToCopy = size_t(height) * bytesPerRow;
	std::memcpy(dst, data, bytesToCopy);
	
	NSImage* image = [[NSImage alloc] initWithSize:NSMakeSize(width, height)];
	[image addRepresentation:bitmap];
	
	NSCursor* cursor = [[NSCursor alloc]
						initWithImage:image
						hotSpot:NSMakePoint(hotspotX, hotspotY)];
	
	static wi::vector<NSCursor*> cursors; // Lifetime extender, a proper solution would require engine to know about autorelease pools lifetimes and manage cursor lifetime with that
	cursors.push_back(cursor);
	
	return (__bridge void*)cursor;
}

}

#elif defined(PLATFORM_IOS)

#include <UIKit/UIKit.h>
#include <QuartzCore/QuartzCore.h>

namespace wi::apple
{

void SetMetalLayerToWindow(void* _window, void* _layer)
{
	UIWindow* window = (__bridge UIWindow*)_window;
	if (!window || !window.rootViewController)
		return;
	
	CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)_layer;
	if (!metalLayer)
		return;
	
	UIView* mainView = window.rootViewController.view;
	
	CAMetalLayer* viewLayer = (CAMetalLayer*)mainView.layer;
	
	if (metalLayer != viewLayer) {
		metalLayer.frame = viewLayer.frame;
		metalLayer.contentsScale = viewLayer.contentsScale;
		[mainView.layer addSublayer:metalLayer];
	}
}

void* GetViewFromWindow(void* handle)
{
	UIWindow* window = (__bridge UIWindow*)handle;
	if (!window || !window.rootViewController)
		return nullptr;
	
	UIView* view = window.rootViewController.view;
	return (__bridge void*)view;
}

XMUINT2 GetWindowSizeNoScaling(void* handle)
{
	if (!handle)
		return XMUINT2(0, 0);
	
	UIWindow* window = (__bridge UIWindow*)handle;
	UIView* view = window.rootViewController ? window.rootViewController.view : nil;
	if (!view)
		return XMUINT2(0, 0);
	
	CGRect bounds = view.bounds;
	uint32_t pixelWidth  = (uint32_t)round(bounds.size.width);
	uint32_t pixelHeight = (uint32_t)round(bounds.size.height);
	
	return XMUINT2(pixelWidth, pixelHeight);
}

XMUINT2 GetWindowSize(void* handle)
{
	if (!handle)
		return XMUINT2(0, 0);
	
	UIWindow* window = (__bridge UIWindow*)handle;
	UIView* view = window.rootViewController ? window.rootViewController.view : nil;
	if (!view)
		return XMUINT2(0, 0);
	
	CGRect bounds = view.bounds;
	CGFloat scale = window.screen.scale;
	
	uint32_t pixelWidth  = (uint32_t)round(bounds.size.width * scale);
	uint32_t pixelHeight = (uint32_t)round(bounds.size.height * scale);
	
	return XMUINT2(pixelWidth, pixelHeight);
}

float GetDPIForWindow(void* handle)
{
	UIWindow* window = (__bridge UIWindow*)handle;
	if (!window || !window.screen)
		return 96.0f;
	
	CGFloat scale = window.screen.scale;
	return scale * 96.0f;
}

XMFLOAT2 GetMousePositionInWindow(void* handle)
{
	return XMFLOAT2(-1.f, -1.f); // TODO primary touch should act as mouse
}

void SetMousePositionInWindow(void* handle, XMFLOAT2 value)
{
}

int MessageBox(const char* title, const char* message, const char* buttons)
{
	@autoreleasepool
	{
		UIAlertController* alert = [UIAlertController
			alertControllerWithTitle:@(title)
			message:@(message)
			preferredStyle:UIAlertControllerStyleAlert];
		
		if (buttons == nullptr || strcmp(buttons, "OK") == 0)
		{
			UIAlertAction* ok = [UIAlertAction actionWithTitle:@"OK"
				style:UIAlertActionStyleDefault
				handler:nil];
			[alert addAction:ok];
			
			[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:alert animated:YES completion:nil];
			return (int)wi::helper::MessageBoxResult::OK;
		}
		else if (strcmp(buttons, "YesNo") == 0)
		{
			__block int result = (int)wi::helper::MessageBoxResult::Cancel;
			
			[alert addAction:[UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
				result = (int)wi::helper::MessageBoxResult::Yes;
			}]];
			
			[alert addAction:[UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
				result = (int)wi::helper::MessageBoxResult::No;
			}]];
			
			[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:alert animated:YES completion:nil];
			return result;
		}
		else if (strcmp(buttons, "OKCancel") == 0)
		{
			__block int result = (int)wi::helper::MessageBoxResult::Cancel;
			
			[alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
				result = (int)wi::helper::MessageBoxResult::OK;
			}]];
			
			[alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:nil]];
			
			[UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:alert animated:YES completion:nil];
			return result;
		}
	}
	return (int)wi::helper::MessageBoxResult::OK;
}

std::string GetExecutablePath()
{
	NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
	return std::string([bundlePath UTF8String]);
}

void CursorInit(void** cursor_table)
{
	for (int i = 0; i < wi::input::CURSOR_COUNT; ++i)
		cursor_table[i] = nullptr;
}

void CursorSet(void* cursor)
{
}

void CursorHide(bool hide)
{
}

void SetWindowFullScreen(void* handle, bool fullscreen)
{
}

bool IsWindowFullScreen(void* handle)
{
	return true;
}

void OpenUrl(const char* url)
{
	@autoreleasepool
	{
		NSString* nsurl = [NSString stringWithUTF8String:url];
		NSURL* nsURL = [NSURL URLWithString:nsurl];
		
		if (nsURL)
		{
			[UIApplication.sharedApplication openURL:nsURL options:@{} completionHandler:nil];
		}
	}
}

std::string GetClipboardText()
{
	std::string ret;
	UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
	NSString* string = pasteboard.string;
	
	if (string)
	{
		const char* cstr = [string UTF8String];
		ret = cstr;
	}
	return ret;
}

void SetClipboardText(const char* str)
{
	UIPasteboard* pasteboard = [UIPasteboard generalPasteboard];
	pasteboard.string = [NSString stringWithUTF8String:str];
}

void* CreateCursorFromARGB8ImageData(const void* data, uint32_t width, uint32_t height, int hotspotX, int hotspotY)
{
	return nullptr;
}

} // namespace wi::apple
#endif
