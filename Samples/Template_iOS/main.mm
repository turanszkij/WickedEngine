#include "WickedEngine.h"

#import <UIKit/UIKit.h>

wi::Application app;
bool running = true;

@interface SimpleViewController : UIViewController
@end

@implementation SimpleViewController

- (void)loadView {
	UIView *view = [[UIView alloc] init];
	view.backgroundColor = [UIColor colorWithRed:1 green:0 blue:0 alpha:1.0];
	self.view = view;
}

- (void)viewDidLoad {
	[super viewDidLoad];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
	return UIStatusBarStyleLightContent;
}

@end

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (nonatomic, strong) UIWindow *window;
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application
	didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
	
	SimpleViewController *vc = [[SimpleViewController alloc] init];
	self.window.rootViewController = vc;
	
	[self.window makeKeyAndVisible];
	
	app.infoDisplay.active = true;
	app.infoDisplay.fpsinfo = true;
	app.infoDisplay.watermark = true;
	app.infoDisplay.logical_size = true;
	app.infoDisplay.mouse_info = true;
	
	app.SetWindow((__bridge void*)self.window);
	
	while (running)
	{
		app.Run();
	}
	
	return YES;
}

@end

int main(int argc, char *argv[])
{
	@autoreleasepool {
		return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
	}
}
