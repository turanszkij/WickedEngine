#include "stdafx.h"
#include "main_Windows.h"
#include "Editor.h"

#include <fstream>
#include <thread>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
Editor editor;


enum Hotkeys
{
	UNUSED = 0,
	PRINTSCREEN = 1,
};

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

	BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	assert(dpi_success);

	wi::arguments::Parse(lpCmdLine); // if you wish to use command line arguments, here is a good place to parse them...

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WICKEDENGINEGAME, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WICKEDENGINEGAME));


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

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WICKEDENGINEGAME));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    //wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WICKEDENGINEGAME);
	wcex.lpszMenuName = L"";
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

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

	   wi::backlog::post("config.ini loaded in " + std::to_string(timer.elapsed_milliseconds()) + " milliseconds\n");
   }

   HWND hWnd = NULL;

   if (borderless || fullscreen)
   {
	   width = std::max(100, width);
	   height = std::max(100, height);

	   hWnd = CreateWindowEx(
		   WS_EX_APPWINDOW,
		   szWindowClass,
		   szTitle,
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
		   szWindowClass,
		   szTitle,
		   WS_OVERLAPPEDWINDOW,
		   CW_USEDEFAULT, 0, width, height,
		   NULL,
		   NULL,
		   hInstance,
		   NULL
	   );
   }

   if (!hWnd)
   {
      return FALSE;
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

   editor.SetWindow(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   RegisterHotKey(hWnd, PRINTSCREEN, 0, VK_SNAPSHOT);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        } 
        break;
	case WM_SIZE:
	case WM_DPICHANGED:
		if(editor.is_window_active)
			editor.SetWindow(hWnd);
	    break;
	case WM_HOTKEY:
		switch (wParam)
		{
		case PRINTSCREEN:
			{
				wi::helper::screenshot(editor.swapChain);
			}
			break;
		default:
			break;
		}
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
