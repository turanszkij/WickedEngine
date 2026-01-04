#include "WickedEngine.h"

#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

wi::Application application;
bool running = true;

@interface WindowDelegate : NSObject <NSWindowDelegate>
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
		
		application.SetWindow((__bridge wi::platform::window_type)window);
		
		application.infoDisplay.active = true;
		application.infoDisplay.watermark = true;
		application.infoDisplay.fpsinfo = true;
		application.infoDisplay.resolution = true;
		application.infoDisplay.logical_size = true;
		application.infoDisplay.mouse_info = true;
		
		while (running)
		{
			@autoreleasepool{
				NSEvent* event;
				while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
												   untilDate:[NSDate distantPast]
													  inMode:NSDefaultRunLoopMode
													 dequeue:YES]))
				{
					switch (event.type) {
						case NSEventTypeKeyDown:
							switch (event.keyCode) {
								case kVK_Delete:
									wi::gui::TextInputField::DeleteFromInput();
									break;
								case kVK_Return:
									break;
								default:
								{
									NSString* characters = event.characters;
									if (characters && characters.length > 0)
									{
										unichar c = [characters characterAtIndex:0];
										wchar_t wchar = (wchar_t)c;
										wi::gui::TextInputField::AddInput(wchar);
									}
								}
								break;
							}
							break;
						case NSEventTypeKeyUp:
							break;
						default:
							[NSApp sendEvent:event];
							break;
					}
				}
				
				application.Run();
				
			}
		}
		
		wi::jobsystem::ShutDown();
	}
	
	return 0;
}

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
	running = false;
}
- (void)windowDidResize:(NSNotification *)notification {
	NSWindow* nsWindow = (NSWindow*)notification.object;
	application.SetWindow((__bridge wi::platform::window_type)nsWindow);
}
@end
