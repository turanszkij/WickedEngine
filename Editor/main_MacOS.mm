#include "stdafx.h"

#import <AppKit/AppKit.h>

Editor editor;

@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
	editor.exit_requested = true;
}
- (void)windowDidResize:(NSNotification *)notification {
	NSWindow* nsWindow = (NSWindow*)notification.object;
	editor.SetWindow((__bridge NS::Window*)nsWindow);
	editor.SaveWindowSize();
}
@end

int main( int argc, char* argv[] )
{
	@autoreleasepool{
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		
		int width = 1280;
		int height = 720;
		bool fullscreen = false;
		bool borderless = false;
		
		wi::Timer timer;
		if (editor.config.Open("config.ini"))
		{
			if (editor.config.Has("width"))
			{
				width = editor.config.GetInt("width");
				height = editor.config.GetInt("height");
			}
			fullscreen = editor.config.GetBool("fullscreen");
			borderless = editor.config.GetBool("borderless");
			editor.allow_hdr = editor.config.GetBool("allow_hdr");
			
			wilog("config.ini loaded in %.2f milliseconds\n", (float)timer.elapsed_milliseconds());
		}
		
		
		CGRect frame = (CGRect){ {100.0, 100.0}, {(float)width, (float)height} };
		NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
													   styleMask:(NSWindowStyleMaskTitled |
																  NSWindowStyleMaskClosable |
																  NSWindowStyleMaskMiniaturizable |
																  NSWindowStyleMaskResizable)
														 backing:NSBackingStoreBuffered
														   defer:NO];
		[window setTitle:@"Wicked Editor"];
		[window center];
		[window makeKeyAndOrderFront:nil];
		
		WindowDelegate *delegate = [[WindowDelegate alloc] init];
		[window setDelegate:delegate];
		
		[NSApp activateIgnoringOtherApps:YES];
		
		editor.SetWindow((__bridge NS::Window*)window);
		
		// The shader binary path is set to source path because that is a writeable folder on Mac OS
		wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
		
		while (!editor.exit_requested)
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
				
				if (editor.exit_requested)
					break;
				
				editor.Run();
				
			}
		}
		
		wi::jobsystem::ShutDown();
	}
	
	return 0;
}
