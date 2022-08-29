#include "stdafx.h"
#include "Editor.h"

// Use the C++ standard templated min/max
#define NOMINMAX

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <future>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <filesystem>

#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Graphics.Display.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.Input.h"
#include "winrt/Windows.UI.ViewManagement.h"
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Foundation.Collections.h>


using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Storage;

winrt::fire_and_forget copy_folder(StorageFolder src, StorageFolder dst)
{
	auto items = co_await src.GetItemsAsync();
	for (auto item : items)
	{
		if (item.IsOfType(StorageItemTypes::File))
		{
			StorageFile file = item.as<StorageFile>();
			try {
				file.CopyAsync(dst);
			}
			catch (...) {
				// file already exists, we don't want to overwrite
			}
		}
		else if (item.IsOfType(StorageItemTypes::Folder))
		{
			StorageFolder src_child = item.as<StorageFolder>();
			auto dst_child = co_await dst.CreateFolderAsync(item.Name(), CreationCollisionOption::OpenIfExists);
			if (dst_child)
			{
				copy_folder(src_child, dst_child);
			}
		}
	}
};
winrt::fire_and_forget uwp_copy_assets()
{
	// On UWP we will copy the base content from application folder to 3D Objects directory
	//	for easy access to the user:
	StorageFolder location = KnownFolders::Objects3D();

	// Objects3D/WickedEngine
	auto destfolder = co_await location.CreateFolderAsync(L"WickedEngine", CreationCollisionOption::OpenIfExists);

	std::string rootdir = std::filesystem::current_path().string() + "\\";
	std::wstring wstr;

	// scripts:
	{
		wi::helper::StringConvert(rootdir + "scripts\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = co_await destfolder.CreateFolderAsync(L"scripts", CreationCollisionOption::OpenIfExists);
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}

	// models:
	{
		wi::helper::StringConvert(rootdir + "models\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = co_await destfolder.CreateFolderAsync(L"models", CreationCollisionOption::OpenIfExists);
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}

	// Documentation:
	{
		wi::helper::StringConvert(rootdir + "Documentation\\", wstr);
		auto src = co_await StorageFolder::GetFolderFromPathAsync(wstr.c_str());
		if (src)
		{
			auto dst = destfolder.CreateFolderAsync(L"Documentation", CreationCollisionOption::OpenIfExists).get();
			if (dst)
			{
				copy_folder(src, dst);
			}
		}
	}
}

class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView>
{
public:

	// IFrameworkView methods
	void Initialize(CoreApplicationView const& applicationView)
	{
		applicationView.Activated({ this, &ViewProvider::OnActivated });

		CoreApplication::Suspending({ this, &ViewProvider::OnSuspending });

		CoreApplication::Resuming({ this, &ViewProvider::OnResuming });

		uwp_copy_assets();

		//wi::Timer timer;
		//if (editor.config.Open("config.ini"))
		//{
		//	editor.allow_hdr = editor.config.GetBool("allow_hdr");

		//	wi::backlog::post("config.ini loaded in " + std::to_string(timer.elapsed_milliseconds()) + " milliseconds\n");
		//}
	}

	void Uninitialize() noexcept
	{
	}

	void SetWindow(CoreWindow const& window)
	{
		window.SizeChanged({ this, &ViewProvider::OnWindowSizeChanged });

		window.VisibilityChanged({ this, &ViewProvider::OnVisibilityChanged });

		window.Closed([this](auto&&, auto&&) { m_exit = true; });

		auto dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();

		dispatcher.AcceleratorKeyActivated({ this, &ViewProvider::OnAcceleratorKeyActivated });

		auto navigation = SystemNavigationManager::GetForCurrentView();

		// UWP on Xbox One triggers a back request whenever the B button is pressed
		// which can result in the app being suspended if unhandled
		navigation.BackRequested([](const winrt::Windows::Foundation::IInspectable&, const BackRequestedEventArgs& args)
			{
				args.Handled(true);
			});

		auto currentDisplayInformation = DisplayInformation::GetForCurrentView();

		currentDisplayInformation.DpiChanged({ this, &ViewProvider::OnDpiChanged });

		DisplayInformation::DisplayContentsInvalidated({ this, &ViewProvider::OnDisplayContentsInvalidated });

		m_DPI = currentDisplayInformation.LogicalDpi();

		m_logicalWidth = window.Bounds().Width;
		m_logicalHeight = window.Bounds().Height;

		editor.SetWindow(&window);
	}

	void Load(winrt::hstring const&) noexcept
	{
	}

	void Run()
	{
		while (!m_exit)
		{
			if (m_visible)
			{
				editor.Run();

				CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			}
			else
			{
				CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
			}
		}
	}

protected:
	// Event handlers
	void OnActivated(CoreApplicationView const& /*applicationView*/, IActivatedEventArgs const& args)
	{
		CoreWindow::GetForCurrentThread().Activate();
	}

	void OnSuspending(IInspectable const& /*sender*/, SuspendingEventArgs const& args)
	{
		auto deferral = args.SuspendingOperation().GetDeferral();

		auto f = std::async(std::launch::async, [this, deferral]()
			{
				deferral.Complete();
			});
	}

	void OnResuming(IInspectable const& /*sender*/, IInspectable const& /*args*/)
	{
	}

	void OnWindowSizeChanged(CoreWindow const& sender, WindowSizeChangedEventArgs const& /*args*/)
	{
		editor.SetWindow(&sender);
	}

	void OnVisibilityChanged(CoreWindow const& /*sender*/, VisibilityChangedEventArgs const& args)
	{
		m_visible = args.Visible();
	}

	void OnAcceleratorKeyActivated(CoreDispatcher const&, AcceleratorKeyEventArgs const& args)
	{
		if (args.EventType() == CoreAcceleratorKeyEventType::SystemKeyDown
			&& args.VirtualKey() == VirtualKey::Enter
			&& args.KeyStatus().IsMenuKeyDown
			&& !args.KeyStatus().WasKeyDown)
		{
			// Implements the classic ALT+ENTER fullscreen toggle
			auto view = ApplicationView::GetForCurrentView();

			if (view.IsFullScreenMode())
				view.ExitFullScreenMode();
			else
				view.TryEnterFullScreenMode();

			args.Handled(true);
		}

		if (args.EventType() == CoreAcceleratorKeyEventType::Character && args.VirtualKey() != VirtualKey::Enter)
		{
			wchar_t c = (wchar_t)args.VirtualKey();

			if (c == '\b')
			{
				wi::gui::TextInputField::DeleteFromInput();
			}
			else
			{
				wi::gui::TextInputField::AddInput(c);
			}

		}
	}

	void OnDpiChanged(DisplayInformation const& sender, IInspectable const& /*args*/)
	{
		editor.SetWindow(&CoreWindow::GetForCurrentThread());
	}

	void OnDisplayContentsInvalidated(DisplayInformation const& /*sender*/, IInspectable const& /*args*/)
	{
	}

private:
	bool  m_exit = false;
	bool  m_visible = true;
	float m_DPI = 96;
	float m_logicalWidth = 800;
	float m_logicalHeight = 600;
	Editor editor;

	inline int ConvertDipsToPixels(float dips) const noexcept
	{
		return int(dips * m_DPI / 96.f + 0.5f);
	}

	inline float ConvertPixelsToDips(int pixels) const noexcept
	{
		return (float(pixels) * 96.f / m_DPI);
	}
};

class ViewProviderFactory : public winrt::implements<ViewProviderFactory, IFrameworkViewSource>
{
public:
	IFrameworkView CreateView()
	{
		return winrt::make<ViewProvider>();
	}
};


// Entry point
int WINAPI wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR    /*lpCmdLine*/,
	_In_ int       /*nCmdShow*/
)
{
	auto viewProviderFactory = winrt::make<ViewProviderFactory>();
	CoreApplication::Run(viewProviderFactory);
	return 0;
}
