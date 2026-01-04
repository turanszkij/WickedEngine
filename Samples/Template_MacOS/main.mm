#include "WickedEngine.h"

#import <AppKit/AppKit.h>

wi::Application application;
bool running = true;

@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
	running = false;
}
- (void)windowDidResize:(NSNotification *)notification {
	NSWindow* nsWindow = (NSWindow*)notification.object;
	application.SetWindow((__bridge NS::Window*)nsWindow);
}
@end

int main( int argc, char* argv[] )
{
	@autoreleasepool{
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		
		CGRect frame = (CGRect){ {100.0, 100.0}, {800.0, 600.0} };
		NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
													   styleMask:(NSWindowStyleMaskTitled |
																  NSWindowStyleMaskClosable |
																  NSWindowStyleMaskMiniaturizable |
																  NSWindowStyleMaskResizable)
														 backing:NSBackingStoreBuffered
														   defer:NO];
		[window setTitle:@"Wicked Engine Mac OS Template"];
		[window center];
		[window makeKeyAndOrderFront:nil];
		
		WindowDelegate *delegate = [[WindowDelegate alloc] init];
		[window setDelegate:delegate];
		
		[NSApp activateIgnoringOtherApps:YES];
		
		application.SetWindow((__bridge NS::Window*)window);
		
		application.infoDisplay.active = true;
		application.infoDisplay.watermark = true;
		application.infoDisplay.fpsinfo = true;
		application.infoDisplay.resolution = true;
		application.infoDisplay.logical_size = true;
		application.infoDisplay.mouse_info = true;
		
		// The shader binary path is set to source path because that is a writeable folder on Mac OS
		wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
		
		while (running)
		{
			@autoreleasepool{
				NSEvent* event;
				while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
												   untilDate:[NSDate distantPast]
													  inMode:NSDefaultRunLoopMode
													 dequeue:YES]))
				{
					if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)
					{
						uint16_t keyCode = event.keyCode;
						bool isDown = (event.type == NSEventTypeKeyDown);
						bool repeat = event.isARepeat;
					}
					else
					{
						[NSApp sendEvent:event];
					}
				}
				
				application.Run();
				
			}
		}
		
		wi::jobsystem::ShutDown();
	}
	
	return 0;
}
