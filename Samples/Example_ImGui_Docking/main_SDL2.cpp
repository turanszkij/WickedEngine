#include "stdafx.h"
#include "sdl2.h"
#include "ImGui/imgui_impl_sdl.h"

Example_ImGui* exampleImGui;

int sdl_loop()
{
    bool quit = false;
    while (!quit)
    {
        exampleImGui->Run();
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE: // exit tests
                            quit = true;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            // Tells the engine to reload window configuration (size and dpi)
                            exampleImGui->SetWindow(exampleImGui->window);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST: //TODO
                            exampleImGui->is_window_active = false;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            exampleImGui->is_window_active = true;
                            if (wi::shadercompiler::GetRegisteredShaderCount() > 0)
                            {
                                std::thread([] {
                                    wi::backlog::post("[Shader check] Started checking " + std::to_string(wi::shadercompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
                                    if (wi::shadercompiler::CheckRegisteredShadersOutdated())
                                    {
                                        wi::backlog::post("[Shader check] Changes detected, initiating reload...");
                                        wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
                                            wi::renderer::ReloadShaders();
                                            });
                                    }
                                    else
                                    {
                                        wi::backlog::post("[Shader check] All up to date");
                                    }
                                    }).detach();
                            }
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
            wi::input::sdlinput::ProcessEvent(event);
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    // TODO: Place code here.

	exampleImGui = new Example_ImGui();
    wi::arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (*system) {
		wilog_error("Error creating SDL2 system");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Tests",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
		wilog_error("Error creating window");
    }

    exampleImGui->SetWindow(window.get());

    int ret = sdl_loop();

	delete exampleImGui;
    SDL_Quit();
    return ret;
}
