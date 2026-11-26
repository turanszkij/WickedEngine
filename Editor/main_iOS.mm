#include "stdafx.h"
#include "wiAppleHelper.h"

#import <UIKit/UIKit.h>

Editor editor;

@interface MyViewController : UIViewController
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end

@interface AppSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property (nonatomic, strong) UIWindow *window;
@property (nonatomic, strong) CADisplayLink *displayLink;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
	didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	return YES;
}

- (UISceneConfiguration *)application:(UIApplication *)application
	configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
	options:(UISceneConnectionOptions *)options
{
	UISceneConfiguration *configuration = [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
	configuration.delegateClass = [AppSceneDelegate class];
	return configuration;
}

@end

@implementation AppSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
	if (![scene isKindOfClass:[UIWindowScene class]])
		return;
	if (self.window != nil)
		return;

	UIWindowScene *windowScene = (UIWindowScene *)scene;
	self.window = [[UIWindow alloc] initWithWindowScene:windowScene];

	MyViewController *vc = [[MyViewController alloc] init];
	self.window.rootViewController = vc;
	[self.window makeKeyAndVisible];

	wi::Timer timer;
	if (editor.config.Open((wi::apple::GetApplicationSupportPath() + "/config.ini").c_str()))
	{
		wilog("config.ini loaded in %.2f milliseconds\n", (float)timer.elapsed_milliseconds());
	}

	editor.SetWindow((__bridge void*)self.window);

	self.displayLink = [CADisplayLink displayLinkWithTarget:self
												   selector:@selector(gameLoop)];
	[self.displayLink addToRunLoop:[NSRunLoop currentRunLoop]
						   forMode:NSRunLoopCommonModes];

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

	UIPinchGestureRecognizer* pinch =
		[[UIPinchGestureRecognizer alloc]
			initWithTarget:self
					action:@selector(OnPinch:)];
	[self.window addGestureRecognizer:pinch];
}

- (void)sceneDidEnterBackground:(UIScene *)scene
{
	editor.is_window_active = false;
	if (!editor.IsScriptReplacement()) // editor config is not saved by script replaced executable
	{
		editor.config.Commit();
	}
}

- (void)sceneWillEnterForeground:(UIScene *)scene
{
	editor.is_window_active = true;
}

- (void)gameLoop {
	@autoreleasepool{
		if (editor.is_window_active) // normally the Run handles inactive window and still present, but on iOS I completely disable Run instead
		{
			editor.Run();
		}
	}
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
- (void)OnPinch:(UIPinchGestureRecognizer*)gesture
{
	CGPoint p = [gesture locationInView:self.window];
	wi::input::Touch touch;
	touch.pos = XMFLOAT2(p.x, p.y);
	touch.scale = gesture.scale;
	switch (gesture.state)
	{
		case UIGestureRecognizerStateBegan:
			touch.state = wi::input::Touch::TOUCHSTATE_MOVED;
			break;
		case UIGestureRecognizerStateEnded:
			touch.state = wi::input::Touch::TOUCHSTATE_RELEASED;
			break;
		default:
			touch.state = wi::input::Touch::TOUCHSTATE_PINCHED;
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
