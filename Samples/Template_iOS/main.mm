#include "WickedEngine.h"

#import <UIKit/UIKit.h>

wi::Application app;

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
	
	app.infoDisplay.active = true;
	app.infoDisplay.fpsinfo = true;
	app.infoDisplay.watermark = true;
	app.infoDisplay.logical_size = true;
	app.infoDisplay.mouse_info = true;
	
	app.SetWindow((__bridge void*)self.window);
	
	self.displayLink = [CADisplayLink displayLinkWithTarget:self
													   selector:@selector(gameLoop)];
	[self.displayLink addToRunLoop:[NSRunLoop currentRunLoop]
						   forMode:NSRunLoopCommonModes];
	
	return YES;
}
- (void)gameLoop {
	app.Run();
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
