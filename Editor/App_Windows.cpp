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

#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Graphics.Display.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.Input.h"
#include "winrt/Windows.UI.ViewManagement.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::ViewManagement;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Display;

class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView>
{
public:

	// IFrameworkView methods
	void Initialize(CoreApplicationView const& applicationView)
	{
		applicationView.Activated({ this, &ViewProvider::OnActivated });

		CoreApplication::Suspending({ this, &ViewProvider::OnSuspending });

		CoreApplication::Resuming({ this, &ViewProvider::OnResuming });

		main.infoDisplay.active = true;
		main.infoDisplay.watermark = true;
		main.infoDisplay.resolution = true;
		main.infoDisplay.fpsinfo = true;

		wiRenderer::SetShaderPath("shaders/");
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

		main.SetWindow(window);
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
				main.Run();

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
		if (args.Kind() == ActivationKind::Launch)
		{
			auto launchArgs = (const LaunchActivatedEventArgs*)(&args);
			wiStartupArguments::Parse(launchArgs->Arguments().data());
		}
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
		m_logicalWidth = sender.Bounds().Width;
		m_logicalHeight = sender.Bounds().Height;

		float dpiscale = wiRenderer::GetDevice()->GetDPIScaling();
		uint64_t data = 0;
		data |= int(m_logicalWidth * dpiscale);
		data |= int(m_logicalHeight * dpiscale) << 16;
		wiEvent::FireEvent(SYSTEM_EVENT_CHANGE_RESOLUTION, data);
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
	}

	void OnDpiChanged(DisplayInformation const& sender, IInspectable const& /*args*/)
	{
		m_DPI = sender.LogicalDpi();

		wiEvent::FireEvent(SYSTEM_EVENT_CHANGE_DPI, (int)m_DPI);
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
	Editor main;

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
