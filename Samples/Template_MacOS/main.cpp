#include "WickedEngine.h"

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

wi::Application application;

int main( int argc, char* argv[] )
{
	NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();
	NS::Application* pSharedApplication = NS::Application::sharedApplication();
	
	CGRect frame = (CGRect){ {100.0, 100.0}, {512.0, 512.0} };
	NS::Window* window = NS::Window::alloc()->init(
		 frame,
		 NS::WindowStyleMaskClosable|NS::WindowStyleMaskResizable|NS::WindowStyleMaskMiniaturizable|NS::WindowStyleMaskTitled,
		 NS::BackingStoreBuffered,
		 false
	);
	window->setTitle(NS::String::string("Wicked Engine MacOS Template", NS::StringEncoding::UTF8StringEncoding));
	window->makeKeyAndOrderFront( nullptr );
	
	MTK::View* view = MTK::View::alloc();
	
	application.canvas.width = (uint32_t)frame.size.width;
	application.canvas.height = (uint32_t)frame.size.height;
	application.SetWindow(view);
	
	class MyMTKViewDelegate : public MTK::ViewDelegate
	{
	public:
		void drawInMTKView( MTK::View* pView ) override
		{
			application.Run();
		}
		void drawableSizeWillChange( class MTK::View* pView, CGSize size ) override
		{
			application.canvas.width = (uint32_t)size.width;
			application.canvas.height = (uint32_t)size.height;
			application.SetWindow(pView);
		}
	} view_delegate;
	view->setDelegate(&view_delegate);
	
	window->setContentView(view);
	
	pSharedApplication->run();
	
	wi::jobsystem::ShutDown();
	
	pAutoreleasePool->release();
	return 0;
}
