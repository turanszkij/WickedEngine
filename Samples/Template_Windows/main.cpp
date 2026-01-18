#include "WickedEngine.h"

wi::Application application;

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
	HWND hWnd = CreateWindowW(wcex.lpszClassName, wcex.lpszClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	ShowWindow(hWnd, SW_SHOWDEFAULT);

	// set Win32 window to engine:
	application.SetWindow(hWnd);

	// process command line string:
	wi::arguments::Parse(lpCmdLine);

	// just show some basic info:
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

	wi::jobsystem::ShutDown(); // waits for jobs to finish before shutdown
	application.ShutDown();

	return (int)msg.wParam;
}
