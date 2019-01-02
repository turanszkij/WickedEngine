#include "wiInputManager.h"
#include "wiXInput.h"
#include "wiDirectInput.h"
#include "wiRawInput.h"
#include "wiWindowRegistration.h"
#include "wiHelper.h"

#include <map>

using namespace std;

namespace wiInputManager
{

#ifndef WINSTORE_SUPPORT
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
#else
#define KEY_DOWN(vk_code) ((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) < 0)
#define KEY_TOGGLE(vk_code) (((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) & 1) != 0)
#endif //WINSTORE_SUPPORT
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))
	static float mousewheel_scrolled = 0.0f;

	wiSpinLock locker;

	std::map<Input, DWORD, Input::LessComparer> inputs;
	std::vector<Touch> touches;

	wiXInput* xinput = nullptr;
	wiDirectInput* dinput = nullptr;
	wiRawInput* rawinput = nullptr;

	void addXInput(wiXInput* input) { xinput = input; }
	void addDirectInput(wiDirectInput* input) { dinput = input; }
	void addRawInput(wiRawInput* input) { rawinput = input; }

	void Update()
	{
		locker.lock();

		if (dinput) {
			dinput->Frame();
		}
		if (xinput) {
			xinput->UpdateControllerState();
		}
		if (rawinput) {
			//rawinput->RetrieveBufferedData();
		}

		for (auto iter = inputs.begin(); iter != inputs.end();)
		{
			InputType type = iter->first.type;
			DWORD button = iter->first.button;
			short playerIndex = iter->first.playerIndex;

			bool todelete = false;

			if (down(button, type, playerIndex))
			{
				iter->second++;
			}
			else
			{
				todelete = true;
			}

			if (todelete)
			{
				inputs.erase(iter++);
			}
			else
			{
				++iter;
			}
		}

		touches.clear();

		mousewheel_scrolled = 0.0f;

		locker.unlock();
	}

	bool down(DWORD button, InputType inputType, short playerindex)
	{
		if (!wiWindowRegistration::IsWindowActive())
		{
			return false;
		}


		switch (inputType)
		{
		case KEYBOARD:
			return KEY_DOWN(static_cast<int>(button)) | KEY_TOGGLE(static_cast<int>(button));
			break;
		case XINPUT_JOYPAD:
			if (xinput != nullptr && xinput->isButtonDown(playerindex, button))
				return true;
			break;
		case DIRECTINPUT_JOYPAD:
			if (dinput != nullptr && (dinput->isButtonDown(playerindex, button) || dinput->getDirections(playerindex) == button)) {
				return true;
			}
			break;
		default:break;
		}
		return false;
	}
	bool press(DWORD button, InputType inputType, short playerindex)
	{
		if (!down(button, inputType, playerindex))
			return false;

		Input input = Input();
		input.button = button;
		input.type = inputType;
		input.playerIndex = playerindex;
		locker.lock();
		auto iter = inputs.find(input);
		if (iter == inputs.end())
		{
			inputs.insert(make_pair(input, 0));
			locker.unlock();
			return true;
		}
		if (iter->second <= 0)
		{
			locker.unlock();
			return true;
		}
		locker.unlock();
		return false;
	}
	bool hold(DWORD button, DWORD frames, bool continuous, InputType inputType, short playerIndex)
	{

		if (!down(button, inputType, playerIndex))
			return false;

		Input input = Input();
		input.button = button;
		input.type = inputType;
		input.playerIndex = playerIndex;
		locker.lock();
		auto iter = inputs.find(input);
		if (iter == inputs.end())
		{
			inputs.insert(make_pair(input, 0));
			locker.unlock();
			return false;
		}
		else if ((!continuous && iter->second == frames) || (continuous && iter->second >= frames))
		{
			locker.unlock();
			return true;
		}
		locker.unlock();
		return false;
	}
	XMFLOAT4 getpointer()
	{
#ifndef WINSTORE_SUPPORT
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(wiWindowRegistration::GetRegisteredWindow(), &p);
		return XMFLOAT4((float)p.x, (float)p.y, mousewheel_scrolled, 0);
#else
		auto& p = Windows::UI::Core::CoreWindow::GetForCurrentThread()->PointerPosition;
		return XMFLOAT4(p.X, p.Y, mousewheel_scrolled, 0);
#endif
	}
	void setpointer(const XMFLOAT4& props)
	{
#ifndef WINSTORE_SUPPORT
		POINT p;
		p.x = (LONG)props.x;
		p.y = (LONG)props.y;
		ClientToScreen(wiWindowRegistration::GetRegisteredWindow(), &p);
		SetCursorPos(p.x, p.y);
#endif

		mousewheel_scrolled = props.z;
	}
	void hidepointer(bool value)
	{
#ifndef WINSTORE_SUPPORT
		if (value)
		{
			while (ShowCursor(true) < 0) {};
		}
		else
		{
			while (ShowCursor(false) >= 0) {};
		}
#endif
	}


	void AddTouch(const Touch& touch)
	{
		locker.lock();
		touches.push_back(touch);
		locker.unlock();
	}

#ifdef WINSTORE_SUPPORT
	using namespace Windows::ApplicationModel;
	using namespace Windows::ApplicationModel::Core;
	using namespace Windows::ApplicationModel::Activation;
	using namespace Windows::UI::Core;
	using namespace Windows::UI::Input;
	using namespace Windows::System;
	using namespace Windows::Foundation;

	void _OnPointerPressed(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_PRESSED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		AddTouch(touch);
	}
	void _OnPointerReleased(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_RELEASED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		AddTouch(touch);
	}
	void _OnPointerMoved(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_MOVED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		AddTouch(touch);
	}
#endif // WINSTORE_SUPPORT

	const std::vector<Touch>& getTouches()
	{
		static bool isRegisteredTouch = false;
		if (!isRegisteredTouch)
		{
#ifdef WINSTORE_SUPPORT
			auto window = CoreWindow::GetForCurrentThread();

			window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerPressed);
			window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerReleased);
			window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerMoved);
#endif // WINSTORE_SUPPORT

			isRegisteredTouch = true;
		}

		return touches;
	}

}
