#include "stdafx.h"
#include "wiAppleHelper.h"

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
	
	// Touch callbacks:
	UITapGestureRecognizer* tap =
		[[UITapGestureRecognizer alloc]
			initWithTarget:self
					action:@selector(OnTap:)];
	[self.window addGestureRecognizer:tap];
	
	UIPanGestureRecognizer* pan =
		[[UIPanGestureRecognizer alloc]
			initWithTarget:self
					action:@selector(OnPan:)];
	[self.window addGestureRecognizer:pan];
	
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

- (void)OnTap:(UITapGestureRecognizer*)gesture
{
	CGPoint p = [gesture locationInView:self.window];
	wi::input::Touch touch;
	touch.state = wi::input::Touch::TOUCHSTATE_PRESSED;
	touch.pos = XMFLOAT2(p.x, p.y);
	wi::input::AddTouchEvent(touch);
}
- (void)OnPan:(UIPanGestureRecognizer*)gesture
{
	CGPoint p = [gesture locationInView:self.window];
	wi::input::Touch touch;
	touch.pos = XMFLOAT2(p.x, p.y);
	switch (gesture.state)
	{
		case UIGestureRecognizerStateBegan:
		case UIGestureRecognizerStateChanged:
			touch.state = wi::input::Touch::TOUCHSTATE_MOVED;
			break;
		case UIGestureRecognizerStateEnded:
			touch.state = wi::input::Touch::TOUCHSTATE_RELEASED;
			break;
		default:
			break;
	}
	wi::input::AddTouchEvent(touch);
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
