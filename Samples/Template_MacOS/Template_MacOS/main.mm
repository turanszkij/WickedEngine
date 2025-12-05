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
		[NSApplication sharedApplication];

		// Create window
		NSRect frame = NSMakeRect(0, 0, 1280, 720);
		NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
		NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
		[window setTitle:@"Wicked Engine App"];
		[window center];  // Optional: Center on screen

		// Create content view and set up Metal layer
		NSView *contentView = [[NSView alloc] initWithFrame:frame];
		[contentView setWantsLayer:YES];

		[window setContentView:contentView];
		[window makeKeyAndOrderFront:nil];
		
		wi::Application application;
		
		application.SetWindow((__bridge void*)contentView);
		
		while(true)
		{
			application.Run();
		}
    }
    return 0;
}
