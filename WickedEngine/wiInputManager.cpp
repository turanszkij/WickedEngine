#include "wiInputManager.h"
#include "wiXInput.h"
#include "wiDirectInput.h"
#include "wiRawInput.h"
#include "wiWindowRegistration.h"

#ifndef WINSTORE_SUPPORT
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
#else
#define KEY_DOWN(vk_code) ((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) < 0)
#define KEY_TOGGLE(vk_code) (((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) & 1) != 0)
#endif //WINSTORE_SUPPORT
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))

wiInputManager::wiInputManager()
{
	xinput = nullptr;
	dinput = nullptr;
	rawinput = nullptr;
	
	thread([&] {

		while (true)
		{
			Update();
			Sleep(1);
		}

	}).detach();
}
wiInputManager::~wiInputManager()
{
}

wiInputManager* wiInputManager::GetInstance()
{
	static wiInputManager* instance = nullptr;
	if (instance == nullptr)
	{
		instance = new wiInputManager();
	}
	return instance;
}


void wiInputManager::addXInput(wiXInput* input){
	xinput=input;
}
void wiInputManager::addDirectInput(wiDirectInput* input){
	dinput = input;
}
void wiInputManager::addRawInput(wiRawInput* input){
	rawinput = input;
}

void wiInputManager::CleanUp(){
	SAFE_DELETE(xinput);
	SAFE_DELETE(dinput);
	SAFE_DELETE(rawinput);
}

void wiInputManager::Update()
{
	LOCK();

	if(dinput){
		dinput->Frame();
	}
	if(xinput){
		xinput->UpdateControllerState();
	}
	if (rawinput){
		//rawinput->RetrieveBufferedData();
	}

	for(InputCollection::iterator iter=inputs.begin();iter!=inputs.end();){
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

		if(todelete){
			inputs.erase(iter++);
		}
		else{
			++iter;
		}
	}

	touches.clear();

	UNLOCK();
}

bool wiInputManager::down(DWORD button, InputType inputType, short playerindex)
{
	if (!wiWindowRegistration::GetInstance()->IsWindowActive())
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
		if (dinput != nullptr && ( dinput->isButtonDown(playerindex, button) || dinput->getDirections(playerindex)==button )){
			return true;
		}
		break;
	default:break;
	}
	return false;
}
bool wiInputManager::press(DWORD button, InputType inputType, short playerindex)
{
	if (!down(button, inputType, playerindex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex = playerindex;
	LOCK();
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(InputCollection::value_type(input,0));
		UNLOCK();
		return true;
	}
	if (iter->second <= 0) 
	{
		UNLOCK();
		return true;
	}
	UNLOCK();
	return false;
}
bool wiInputManager::hold(DWORD button, DWORD frames, bool continuous, InputType inputType, short playerIndex)
{

	if (!down(button, inputType, playerIndex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	LOCK();
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(pair<Input,DWORD>(input,0));
		UNLOCK();
		return false;
	}
	else if ((!continuous && iter->second == frames) || (continuous && iter->second >= frames))
	{
		UNLOCK();
		return true;
	}
	UNLOCK();
	return false;
}
XMFLOAT4 wiInputManager::getpointer()
{
#ifndef WINSTORE_SUPPORT
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(wiWindowRegistration::GetInstance()->GetRegisteredWindow(), &p);
	return XMFLOAT4((float)p.x, (float)p.y, 0, 0);
#else
	auto& p = Windows::UI::Core::CoreWindow::GetForCurrentThread()->PointerPosition;
	return XMFLOAT4(p.X, p.Y, 0, 0);
#endif
}
void wiInputManager::setpointer(const XMFLOAT4& props)
{
#ifndef WINSTORE_SUPPORT
	POINT p;
	p.x = (LONG)props.x;
	p.y = (LONG)props.y;
	ClientToScreen(wiWindowRegistration::GetInstance()->GetRegisteredWindow(), &p);
	SetCursorPos(p.x, p.y);
#endif
}
void wiInputManager::hidepointer(bool value)
{
#ifndef WINSTORE_SUPPORT
	ShowCursor(!value);
#endif
}


void AddTouch(const wiInputManager::Touch& touch)
{
	wiInputManager::GetInstance()->LOCK();
	wiInputManager::GetInstance()->touches.push_back(touch);
	wiInputManager::GetInstance()->UNLOCK();
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

	wiInputManager::Touch touch;
	touch.state = wiInputManager::Touch::TOUCHSTATE_PRESSED;
	touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
	AddTouch(touch);
}
void _OnPointerReleased(CoreWindow^ window, PointerEventArgs^ pointer)
{
	auto p = pointer->CurrentPoint;

	wiInputManager::Touch touch;
	touch.state = wiInputManager::Touch::TOUCHSTATE_RELEASED;
	touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
	AddTouch(touch);
}
void _OnPointerMoved(CoreWindow^ window, PointerEventArgs^ pointer)
{
	auto p = pointer->CurrentPoint;

	wiInputManager::Touch touch;
	touch.state = wiInputManager::Touch::TOUCHSTATE_MOVED;
	touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
	AddTouch(touch);
}
#endif // WINSTORE_SUPPORT

vector<wiInputManager::Touch> wiInputManager::getTouches()
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

