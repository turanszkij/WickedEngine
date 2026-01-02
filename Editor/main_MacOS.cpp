#include "stdafx.h"

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

Editor editor;

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
	
	CGRect frame = (CGRect){ {100.0, 100.0}, {1920, 1080} };
	static NS::Window* window = NS::Window::alloc()->init(
		 frame,
		 NS::WindowStyleMaskClosable|NS::WindowStyleMaskResizable|NS::WindowStyleMaskMiniaturizable|NS::WindowStyleMaskTitled,
		 NS::BackingStoreBuffered,
		 false
	);
	window->setTitle(NS::String::string("Wicked Editor", NS::StringEncoding::UTF8StringEncoding));
	window->makeKeyAndOrderFront( nullptr );
	
	editor.SetWindow(window);
	
	// The shader binary path is set to source path because that is a writeable folder on Mac OS
	wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
	
	class MyMTKViewDelegate : public MTK::ViewDelegate
	{
	public:
		Editor editor;
		
		void drawInMTKView( MTK::View* pView ) override
		{
			editor.Run();
			if (editor.exit_requested)
			{
				wi::platform::Exit();
			}
		}
		void drawableSizeWillChange( class MTK::View* pView, CGSize size ) override
		{
			editor.SetWindow(window);
		}
	} view_delegate;
	
	MTK::View* view = wi::apple::GetMTKViewFromWindow(window);
	view->setDelegate(&view_delegate);
	
	pSharedApplication->run();
	
	wi::jobsystem::ShutDown();
	
	return 0;
}
