#include "App.h"
#include "stdafx.h"

#include <iostream>

using std::cout;

void App::Initialize()
{
    wi::renderer::SetShaderSourcePath(WickedEngine_SHADER_DIR);
    wi::Application::Initialize();

    infoDisplay.active = false;
    infoDisplay.watermark = false;
    infoDisplay.fpsinfo = false;
    infoDisplay.resolution = false;
    infoDisplay.heap_allocation_counter = false;

    renderer.init(canvas);
    renderer.Load();

	ActivatePath(&renderer);
}

void Renderer::Load()
{
    setSSREnabled(true);
    setReflectionsEnabled(true);
    setFXAAEnabled(true);

    static wi::audio::Sound sound;
    static wi::audio::SoundInstance soundinstance;

    // Reset all state that tests might have modified:
    wi::eventhandler::SetVSync(true);
    wi::renderer::SetToDrawGridHelper(false);
    wi::renderer::ClearWorld(wi::scene::GetScene());
    wi::scene::GetScene().weather = wi::scene::WeatherComponent();
    this->ClearSprites();
    this->ClearFonts();
    
    if (wi::lua::GetLuaState() != nullptr) 
    {
        wi::lua::KillProcesses();
    }

    // Reset camera position:
    wi::scene::TransformComponent transform;
    transform.Translate(XMFLOAT3(0, 2.f, -4.5f));
    transform.UpdateTransform();
    wi::scene::GetCamera().TransformCamera(transform);

    float screenW = GetLogicalWidth();
    float screenH = GetLogicalHeight();

    RenderPath3D::Load();
}

void Renderer::Update(float dt)
{
    RenderPath3D::Update(dt);
}
