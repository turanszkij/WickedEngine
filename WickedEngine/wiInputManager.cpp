#include "wiInputManager.h"
#include "wiXInput.h"
#include "wiDirectInput.h"
#include "wiRawInput.h"

wiXInput* wiInputManager::xinput = nullptr;
wiDirectInput* wiInputManager::dinput = nullptr;
wiRawInput* wiInputManager::rawinput = nullptr;
wiInputManager::InputCollection wiInputManager::inputs;

#ifndef WINSTORE_SUPPORT
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
#else
#define KEY_DOWN(vk_code) (false)
#define KEY_TOGGLE(vk_code) (false)
#endif //WINSTORE_SUPPORT
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))


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
	xinput->CleanUp();
	xinput=nullptr;

	dinput->Shutdown();
	dinput=nullptr;
}

void wiInputManager::Update(){
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
}

bool wiInputManager::down(DWORD button, InputType inputType, short playerindex)
{
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
bool wiInputManager::press(DWORD button, InputType inputType, short playerindex){

	if (!down(button, inputType, playerindex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex = playerindex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(InputCollection::value_type(input,0));
		return true;
	}
	return false;
}
bool wiInputManager::hold(DWORD button, DWORD frames, bool continuous, InputType inputType, short playerIndex){

	if (!down(button, inputType, playerIndex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(pair<Input,DWORD>(input,0));
		return false;
	}
	else if ((!continuous && iter->second == frames) || (continuous && iter->second >= frames)){
		return true;
	}
	return false;
}
XMFLOAT4 wiInputManager::pointer()
{
#ifndef WINSTORE_SUPPORT
	POINT p;
	GetCursorPos(&p);
	return XMFLOAT4((float)p.x, (float)p.y, 0, 0);
#endif
	return XMFLOAT4(0, 0, 0, 0);
}
void wiInputManager::setpointer(const XMFLOAT4& props)
{
#ifndef WINSTORE_SUPPORT
	SetCursorPos((int)props.x, (int)props.y);
#endif
}

