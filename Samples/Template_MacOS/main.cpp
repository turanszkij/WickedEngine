#include "WickedEngine.h"

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

wi::Application application;

int main( int argc, char* argv[] )
{
	NS::SharedPtr<NS::AutoreleasePool> pAutoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
	NS::SharedPtr<NS::Application> pSharedApplication = NS::TransferPtr(NS::Application::sharedApplication());
	
	class MyAppDelegate : public NS::ApplicationDelegate
	{
	 public:
		void applicationWillFinishLaunching( NS::Notification* pNotification ) override {}
		void applicationDidFinishLaunching( NS::Notification* pNotification ) override {}
		bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override { return true; }
	} application_delegate;
	pSharedApplication->setDelegate(&application_delegate);
	
	CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };
	NS::Window* window = NS::Window::alloc()->init(
		 frame,
		 NS::WindowStyleMaskClosable|NS::WindowStyleMaskResizable|NS::WindowStyleMaskMiniaturizable|NS::WindowStyleMaskTitled,
		 NS::BackingStoreBuffered,
		 false
	);
	window->setTitle(NS::String::string("Wicked Engine MacOS Template", NS::StringEncoding::UTF8StringEncoding));
	window->makeKeyAndOrderFront( nullptr );
	
	NS::SharedPtr<MTK::View> view = NS::TransferPtr(MTK::View::alloc());
	
	wi::Canvas canvas;
	canvas.width = (uint32_t)frame.size.width;
	canvas.height = (uint32_t)frame.size.height;
	application.SetWindow(view.get(), canvas);
	
	wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
	
	class MyMTKViewDelegate : public MTK::ViewDelegate
	{
	public:
		void drawInMTKView( MTK::View* pView ) override
		{
			application.Run();
		}
		void drawableSizeWillChange( class MTK::View* pView, CGSize size ) override
		{
			wi::Canvas canvas;
			canvas.width = (uint32_t)size.width;
			canvas.height = (uint32_t)size.height;
			application.SetWindow(pView, canvas);
		}
	} view_delegate;
	view->setDelegate(&view_delegate);
	
	window->setContentView(view.get());
	
	pSharedApplication->run();
	
	wi::jobsystem::ShutDown();
	
	return 0;
}
