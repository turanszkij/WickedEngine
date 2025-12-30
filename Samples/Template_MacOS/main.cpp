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
	static NS::Window* window = NS::Window::alloc()->init(
		 frame,
		 NS::WindowStyleMaskClosable|NS::WindowStyleMaskResizable|NS::WindowStyleMaskMiniaturizable|NS::WindowStyleMaskTitled,
		 NS::BackingStoreBuffered,
		 false
	);
	window->setTitle(NS::String::string("Wicked Engine MacOS Template", NS::StringEncoding::UTF8StringEncoding));
	window->makeKeyAndOrderFront( nullptr );
	
	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.fpsinfo = true;
	application.infoDisplay.logical_size = true;
	application.infoDisplay.resolution = true;
	application.infoDisplay.mouse_info = true;
	
	application.SetWindow(window);
	
	// The shader binary path is set to source path because that is a writeable folder on Mac OS
	wi::renderer::SetShaderPath(wi::renderer::GetShaderSourcePath() + "metal/");
	
	//wi::initializer::InitializeComponentsImmediate();
	
	wi::RenderPath3D path;
	application.ActivatePath(&path);
	auto& cam = wi::scene::GetCamera();
	cam.CreatePerspective(application.canvas.width, application.canvas.height, 0.01f, 1000.0f);
	cam.Eye = XMFLOAT3(0, 1.8f, -6);
	cam.UpdateCamera();
	auto& scene = wi::scene::GetScene();
	scene.weather.ambient = XMFLOAT3(0.5f, 0.5f, 0.5f);
	
	//wi::ecs::Entity entity = scene.Entity_CreateCube("cube");
	//auto& material = *scene.materials.GetComponent(entity);
	//material.textures[0].name = "/Users/turanszkij/PROJECTS/WickedEngine/Content/models/Sponza/textures/lion.png";
	//material.CreateRenderData();
	
	wi::scene::LoadModel("/Users/turanszkij/PROJECTS/WickedEngine/Content/models/Sponza/Sponza.wiscene");
	
	wi::renderer::SetOcclusionCullingEnabled(false);
	wi::renderer::SetToDrawGridHelper(true);
	//wi::profiler::SetEnabled(true);
	
	//wi::Sprite sprite;
	//sprite.textureResource.SetTexture(*wi::texturehelper::getLogo());
	//sprite.params = wi::image::Params(300, 100, 256, 256);
	//sprite.params.enableCornerRounding();
	//sprite.params.corners_rounding[0].radius = 40;
	//path.AddSprite(&sprite);
	
	//wi::gui::ColorPicker colorpicker;
	//colorpicker.Create("Color");
	//colorpicker.SetPos(XMFLOAT2(50,100));
	//path.GetGUI().AddWidget(&colorpicker);
	
	//application.Run();
	//wi::lua::RunFile("/Users/turanszkij/PROJECTS/WickedEngine/Content/scripts/character_controller/character_controller.lua");
	
	class MyMTKViewDelegate : public MTK::ViewDelegate
	{
	public:
		void drawInMTKView( MTK::View* pView ) override
		{
			application.Run();
		}
		void drawableSizeWillChange( class MTK::View* pView, CGSize size ) override
		{
			application.SetWindow(window);
			
			auto& cam = wi::scene::GetCamera();
			cam.CreatePerspective(application.canvas.width, application.canvas.height, 0.01f, 1000.0f);
			cam.UpdateCamera();
		}
	} view_delegate;
	
	MTK::View* view = wi::apple::GetMTKViewFromWindow(window);
	view->setDelegate(&view_delegate);
	
	pSharedApplication->run();
	
	wi::jobsystem::ShutDown();
	
	return 0;
}
