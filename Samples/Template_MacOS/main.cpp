#include "WickedEngine.h"

#include <AppKit/AppKit.hpp>

wi::Application application;

int main( int argc, char* argv[] )
{
	NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
	NS::Application* pSharedApplication = NS::Application::sharedApplication();
	
	CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };
	NS::Window* window = NS::Window::alloc()->init(
		 frame,
		 NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
		 NS::BackingStoreBuffered,
		 false
	);
	window->setTitle(NS::String::string("Wicked Engine MacOS Template", NS::StringEncoding::UTF8StringEncoding));
	window->makeKeyAndOrderFront( nullptr );
	
	application.SetWindow(window);
	
	class MyAppDelegate : public NS::ApplicationDelegate
	{
	public:
		virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override{}
		virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override{
			std::thread([]{
				while(true){
					application.Run();
				}
			}).detach();
		}
		virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override {return  true;}
	} application_delegate;
	
	pSharedApplication->setDelegate( &application_delegate );
	
	pSharedApplication->run();
	pAutoreleasePool->release();
	return 0;
}
