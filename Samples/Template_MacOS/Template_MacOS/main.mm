//
//  main.m
//  Template_MacOS
//
//  Created by János Turánszki on 2025. 12. 05..
//

#import <Cocoa/Cocoa.h>
#include "WickedEngine.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
		// Initialize NSApplication
		NSApplication *app = [NSApplication sharedApplication];
		[app activateIgnoringOtherApps:YES];  // Bring app to front

		// Create window
		NSRect frame = NSMakeRect(0, 0, 1280, 720);
		NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
		NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
		[window setTitle:@"Wicked Engine MacOS Template"];
		[window center];  // Optional: Center on screen

		// Create content view and set up Metal layer
		NSView *contentView = [[NSView alloc] initWithFrame:frame];
		[contentView setWantsLayer:YES];

		[window setContentView:contentView];
		[window makeKeyAndOrderFront:nil];
		
		wi::Application application;
		
		application.SetWindow((__bridge void*)window);
		
		BOOL running = YES;
		while (running) {
			@autoreleasepool {
				NSEvent *event;
				do {
					event = [app nextEventMatchingMask:NSEventMaskAny
											 untilDate:nil  // Non-blocking: distantPast for immediate return if no event
												inMode:NSDefaultRunLoopMode
											   dequeue:YES];
					if (event) {
						[app sendEvent:event];
					}
				} while (event);  // Process all pending events
			}

			application.Run();

			// Basic exit check: Stop if window is closed (add delegate for more robust handling)
			if (![window isVisible]) {
				running = NO;
			}
		}
    }
	return 0;
}
