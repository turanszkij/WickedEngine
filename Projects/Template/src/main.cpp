#include "WickedEngine.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <SDL2/SDL.h>
#endif

wi::Application application;

#ifdef _WIN32
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	// Win32 window and message loop setup:
	static auto WndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		switch (message)
		{
		case WM_SIZE:
		case WM_DPICHANGED:
			if (application.is_window_active)
				application.SetWindow(hWnd);
			break;
		case WM_CHAR:
			switch (wParam)
			{
			case VK_BACK:
				wi::gui::TextInputField::DeleteFromInput();
				break;
			case VK_RETURN:
				break;
			default:
			{
				const wchar_t c = (const wchar_t)wParam;
				wi::gui::TextInputField::AddInput(c);
			}
			break;
			}
			break;
		case WM_INPUT:
			wi::input::rawinput::ParseMessage((void*)lParam);
			break;
		case WM_KILLFOCUS:
			application.is_window_active = false;
			break;
		case WM_SETFOCUS:
			application.is_window_active = true;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	};
	
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WickedEngineApplicationTemplate";
	wcex.hIconSm = NULL;
	RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(wcex.lpszClassName, wcex.lpszClassName, WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	ShowWindow(hWnd, SW_SHOWDEFAULT);

	application.SetWindow(hWnd);
	wi::arguments::Parse(lpCmdLine);

	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.resolution = true;
	application.infoDisplay.fpsinfo = true;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			application.Run();
		}
	}

	wi::jobsystem::ShutDown();
	return (int)msg.wParam;
}
#else
int main(int argc, char *argv[])
{
	// SDL window setup for Linux and other platforms:
	sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
	sdl2::window_ptr_t window = sdl2::make_window(
		"WickedEngineApplicationTemplate",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		1920, 1080,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	application.SetWindow(window.get());
	wi::arguments::Parse(argc, argv);

	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.resolution = true;
	application.infoDisplay.fpsinfo = true;

	bool quit = false;
	while (!quit)
	{
		SDL_PumpEvents();
		application.Run();

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					quit = true;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					application.SetWindow(application.window);
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			wi::input::sdlinput::ProcessEvent(event);
		}
	}

	wi::jobsystem::ShutDown();
	SDL_Quit();

	return 0;
}
#endif