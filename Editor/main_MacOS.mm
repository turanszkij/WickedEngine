#include "stdafx.h"

#import <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@interface EditorContentView : NSView <NSDraggingDestination>
@end

Editor editor;

int main( int argc, char* argv[] )
{
	@autoreleasepool{
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		
		wi::arguments::Parse(argc, argv);
		
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
		
		EditorContentView* contentView = [[EditorContentView alloc] initWithFrame:window.contentView.frame];
		[window setContentView:contentView];
		
		[NSApp activateIgnoringOtherApps:YES];
		
		editor.SetWindow((__bridge wi::platform::window_type)window);
		
		while (!editor.exit_requested)
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
						case NSEventTypeScrollWheel:
						{
							float amount = event.scrollingDeltaY;
							if (event.hasPreciseScrollingDeltas)
							{
								amount *= 0.05f;
							}
							wi::input::AddMouseScrollEvent(amount);
						}
						break;
						case NSEventTypeMouseMoved:
						case NSEventTypeLeftMouseDragged:
						case NSEventTypeRightMouseDragged:
						case NSEventTypeOtherMouseDragged:
							wi::input::AddMouseMoveDeltaEvent(XMFLOAT2(event.deltaX, event.deltaY));
							break;
						default:
							[NSApp sendEvent:event];
							break;
					}
				}
				
				editor.Run();
				
			}
		}
		
		wi::jobsystem::ShutDown();
	}
	
	return 0;
}

// Window events handling:
@implementation WindowDelegate
- (void)windowWillClose:(NSNotification *)notification {
	editor.exit_requested = true;
}
- (void)windowDidResize:(NSNotification *)notification {
	NSWindow* nsWindow = (NSWindow*)notification.object;
	editor.SetWindow((__bridge wi::platform::window_type)nsWindow);
	editor.SaveWindowSize();
}
- (void)windowDidBecomeMain:(NSNotification *)notification {
	editor.is_window_active = true;
	editor.HotReload();
}
- (void)windowDidResignMain:(NSNotification *)notification {
	editor.is_window_active = false;
}
@end

// Drag and drop handling:
@implementation EditorContentView
- (instancetype)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
	}
	return self;
}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	NSPasteboard* pboard = [sender draggingPasteboard];
	if ([pboard canReadObjectForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}]) {
		return NSDragOperationCopy;  // Show copy cursor
	}
	return NSDragOperationNone;
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	NSPasteboard* pboard = [sender draggingPasteboard];

	NSArray<NSURL*>* fileURLs = [pboard readObjectsForClasses:@[[NSURL class]]
													 options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
	if (fileURLs.count > 0) {
		for (NSURL* url in fileURLs) {
			if (url.isFileURL) {
				const char* path = url.fileSystemRepresentation;
				editor.renderComponent.Open(path);
			}
		}
		return YES;
	}
	return NO;
}
@end
