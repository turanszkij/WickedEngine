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
	
	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.fpsinfo = true;
	
	wi::Canvas canvas;
	canvas.width = (uint32_t)frame.size.width;
	canvas.height = (uint32_t)frame.size.height;
	application.SetWindow(view.get(), canvas);
	
	// The shader binary path is set to source path because that is a writeable folder on Mac OS
	wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
	
	wi::initializer::InitializeComponentsImmediate();
	wi::RenderPath3D path;
	application.ActivatePath(&path);
	auto& cam = wi::scene::GetCamera();
	cam.CreatePerspective(canvas.width, canvas.height, 0.1f, 1000.0f);
	cam.Eye = XMFLOAT3(0, 1, -3);
	cam.UpdateCamera();
	auto& scene = wi::scene::GetScene();
	scene.weather.ambient = XMFLOAT3(0.5f, 0.5f, 0.5f);
	scene.Entity_CreateCube("cube");
	//wi::scene::LoadModel("/Users/turanszkij/PROJECTS/WickedEngine/Content/models/Sponza/Sponza.wiscene");
	wi::renderer::SetOcclusionCullingEnabled(false);
	wi::renderer::SetToDrawGridHelper(true);
	
	wi::Sprite sprite;
	sprite.textureResource.SetTexture(*wi::texturehelper::getLogo());
	sprite.params = wi::image::Params(300, 100, 256, 256);
	sprite.params.enableCornerRounding();
	sprite.params.corners_rounding[0].radius = 40;
	path.AddSprite(&sprite);
	
	wi::gui::ColorPicker colorpicker;
	colorpicker.Create("Color");
	colorpicker.SetPos(XMFLOAT2(50,100));
	path.GetGUI().AddWidget(&colorpicker);
	
	class MyMTKViewDelegate : public MTK::ViewDelegate
	{
	public:
		void drawInMTKView( MTK::View* pView ) override
		{
			NS::SharedPtr<NS::AutoreleasePool> pAutoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
			application.Run();
		}
		void drawableSizeWillChange( class MTK::View* pView, CGSize size ) override
		{
			NS::SharedPtr<NS::AutoreleasePool> pAutoreleasePool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
			wi::Canvas canvas;
			canvas.width = (uint32_t)size.width;
			canvas.height = (uint32_t)size.height;
			application.SetWindow(pView, canvas);
			
			auto& cam = wi::scene::GetCamera();
			cam.CreatePerspective(canvas.width, canvas.height, 0.01f, 1000.0f);
			cam.UpdateCamera();
		}
	} view_delegate;
	view->setDelegate(&view_delegate);
	
	window->setContentView(view.get());
	
	pSharedApplication->run();
	
	wi::jobsystem::ShutDown();
	
	return 0;
}
