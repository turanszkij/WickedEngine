#include "stdafx.h"

#include <fstream>
#include <shellapi.h> // drag n drop
#include <dwmapi.h> // DwmSetWindowAttribute

#pragma comment(lib, "dwmapi.lib")

Editor editor;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	wi::arguments::Parse(lpCmdLine);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	static auto WndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		switch (message)
		{
		case WM_SIZE:
		case WM_DPICHANGED:
			if (editor.is_window_active && LOWORD(lParam) > 0 && HIWORD(lParam) > 0)
				editor.SetWindow(hWnd);
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
				const wchar_t c = (const wchar_t)wParam;
				wi::gui::TextInputField::AddInput(c);
				break;
			}
			break;
		case WM_INPUT:
			wi::input::rawinput::ParseMessage((void*)lParam);
			break;
		case WM_POINTERUPDATE:
		{
			POINTER_PEN_INFO pen_info = {};
			if (GetPointerPenInfo(GET_POINTERID_WPARAM(wParam), &pen_info))
			{
				ScreenToClient(hWnd, &pen_info.pointerInfo.ptPixelLocation);
				const float dpiscaling = (float)GetDpiForWindow(hWnd) / 96.0f;
				wi::input::Pen pen;
				pen.position = XMFLOAT2(pen_info.pointerInfo.ptPixelLocation.x / dpiscaling, pen_info.pointerInfo.ptPixelLocation.y / dpiscaling);
				pen.pressure = float(pen_info.pressure) / 1024.0f;
				wi::input::SetPen(pen);
			}
		}
		break;
		case WM_KILLFOCUS:
			editor.is_window_active = false;
			break;
		case WM_SETFOCUS:
			editor.is_window_active = true;
			editor.HotReload();
			break;
		case WM_DROPFILES:
		{
			HDROP hdrop = (HDROP)wParam;
			UINT filecount = DragQueryFile(hdrop, 0xFFFFFFFF, nullptr, 0);
			assert(filecount != 0);
			for (UINT i = 0; i < filecount; ++i)
			{
				wchar_t wfilename[1024] = {};
				UINT res = DragQueryFile(hdrop, i, wfilename, arraysize(wfilename));
				if (res == 0)
				{
					assert(0);
					continue;
				}
				std::string filename;
				wi::helper::StringConvert(wfilename, filename);
				editor.renderComponent.Open(filename);
			}
			SetForegroundWindow(hWnd);
		}
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_SETCURSOR:
			switch (LOWORD(lParam))
			{
			case HTBOTTOM:
			case HTBOTTOMLEFT:
			case HTBOTTOMRIGHT:
			case HTLEFT:
			case HTRIGHT:
			case HTTOP:
			case HTTOPLEFT:
			case HTTOPRIGHT:
				// allow the system to handle these window resize cursors:
				return DefWindowProc(hWnd, message, wParam, lParam);
			default:
				// notify the engine at other cursor changes to set its own cursor instead
				wi::input::NotifyCursorChanged();
				break;
			}
			break;
		case WM_SETTINGCHANGE:
			{
				// Change window theme dark mode based on system setting:
				HKEY hKey;
				DWORD value = 1; // Default to light mode (1 = light, 0 = dark)
				DWORD dataSize = sizeof(value);
				if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
				{
					RegQueryValueEx(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &dataSize);
					RegCloseKey(hKey);
				}
				BOOL darkmode = value == 0;
				DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkmode, sizeof(darkmode));
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	};

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1001)); // 1001 = icon from Resource.rc file
	wcex.hIconSm = NULL;
	wcex.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wchar_t wname[256] = {};
	wi::helper::StringConvert(wi::helper::StringRemoveTrailingWhitespaces(exe_customization.name_padded).c_str(), wname, arraysize(wname));
	wcex.lpszClassName = wname;
	RegisterClassExW(&wcex);



	int width = CW_USEDEFAULT;
	int height = 0;
	bool fullscreen = false;
	bool borderless = false;

	wi::Timer timer;
	if (editor.config.Open("config.ini"))
	{
		if (editor.config.Has("width"))
		{
			width = editor.config.GetInt("width");
			height = editor.config.GetInt("height");
		}
		fullscreen = editor.config.GetBool("fullscreen");
		borderless = editor.config.GetBool("borderless");
		editor.allow_hdr = editor.config.GetBool("allow_hdr");

		wilog("config.ini loaded in %.2f milliseconds\n", (float)timer.elapsed_milliseconds());
	}

	HWND hWnd = NULL;

	if (borderless || fullscreen)
	{
		width = std::max(100, width);
		height = std::max(100, height);

		hWnd = CreateWindowEx(
			WS_EX_APPWINDOW,
			wcex.lpszClassName,
			wcex.lpszClassName,
			WS_POPUP,
			CW_USEDEFAULT, 0, width, height,
			NULL,
			NULL,
			hInstance,
			NULL
		);
	}
	else
	{
		hWnd = CreateWindow(
			wcex.lpszClassName,
			wcex.lpszClassName,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, width, height,
			NULL,
			NULL,
			hInstance,
			NULL
		);
	}
	if (hWnd == NULL)
	{
		wilog_error("Win32 window creation failure!");
		return -1;
	}
	if (fullscreen)
	{
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &info);
		width = info.rcMonitor.right - info.rcMonitor.left;
		height = info.rcMonitor.bottom - info.rcMonitor.top;
		MoveWindow(hWnd, 0, 0, width, height, FALSE);
	}
	SendMessage(hWnd, WM_SETTINGCHANGE, 0, 0); // trigger dark mode theme detection
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	DragAcceptFiles(hWnd, TRUE);

	editor.SetWindow(hWnd);

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {

			editor.Run();

		}
	}

	wi::jobsystem::ShutDown();

    return (int)msg.wParam;
}
