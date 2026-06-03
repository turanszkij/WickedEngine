#include "stdafx.h"

#import <UIKit/UIKit.h>

Editor editor;

@interface MyViewController : UIViewController
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow *window;
@property (nonatomic, strong) CADisplayLink *displayLink;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
	didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	MyViewController *vc = [[MyViewController alloc] init];
	self.window.rootViewController = vc;
	
	[self.window makeKeyAndVisible];
	
	editor.SetWindow((__bridge void*)self.window);
	
	self.displayLink = [CADisplayLink displayLinkWithTarget:self
													   selector:@selector(gameLoop)];
	[self.displayLink addToRunLoop:[NSRunLoop currentRunLoop]
						   forMode:NSRunLoopCommonModes];
	
	return YES;
}
- (void)gameLoop {
	@autoreleasepool{
		editor.Run();
	}
}

- (void)dealloc {
	[self.displayLink invalidate];
	self.displayLink = nil;
	wi::jobsystem::ShutDown();
}

@end

@implementation MyViewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
	return UIInterfaceOrientationMaskLandscape;
}
@end

int main(int argc, char *argv[])
{
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
