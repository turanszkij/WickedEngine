#include "InputManager.h"

XInput* InputManager::xinput=nullptr;
DirectInput* InputManager::dinput=nullptr;
InputManager::InputCollection InputManager::inputs;

void InputManager::addXInput(XInput* input){
	xinput=input;
}
void InputManager::addDirectInput(DirectInput* input){
	dinput=input;
}

void InputManager::CleanUp(){
	xinput->CleanUp();
	xinput=nullptr;

	dinput->Shutdown();
	dinput=nullptr;
}

void InputManager::Update(){
	if(dinput){
		dinput->Frame();
	}
	if(xinput){
		xinput->UpdateControllerState();
	}

	for(InputCollection::iterator iter=inputs.begin();iter!=inputs.end();){
		InputType type = iter->first.type;
		DWORD button = iter->first.button;
		short playerIndex = iter->first.playerIndex;

		bool todelete = false;

		switch (type)
		{
		case InputManager::DIRECTINPUT_JOYPAD:
			if(dinput!=nullptr && dinput->isButtonDown(playerIndex,button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		case InputManager::XINPUT_JOYPAD:
			if(xinput!=nullptr && xinput->isButtonDown(playerIndex,button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		case InputManager::KEYBOARD:
			if(dinput!=nullptr && dinput->IsKeyDown(button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		default:
			break;
		}

		if(todelete){
			inputs.erase(iter++);
		}
		else{
			++iter;
		}
	}
}

bool InputManager::press(DWORD button){
	return press(button,0,InputType::KEYBOARD);
}
bool InputManager::press(DWORD button, short playerIndex, InputType inputType){

	switch (inputType)
	{
	case InputManager::DIRECTINPUT_JOYPAD:
		if(dinput!=nullptr && !dinput->isButtonDown(playerIndex,button)){
			return false;
		}
		break;
	case InputManager::XINPUT_JOYPAD:
		if(xinput!=nullptr && !xinput->isButtonDown(playerIndex,button)){
			return false;
		}
		break;
	case InputManager::KEYBOARD:
		if(dinput!=nullptr && !dinput->IsKeyDown(button)){
			return false;
		}
		break;
	default:
		return false;
		break;
	}

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(InputCollection::value_type(input,0));
		return true;
	}
	return false;
}
bool InputManager::hold(DWORD button, DWORD frames){
	return hold(button,frames,0,InputType::KEYBOARD);
}
bool InputManager::hold(DWORD button, DWORD frames, short playerIndex, InputType inputType){

	switch (inputType)
	{
	case InputManager::DIRECTINPUT_JOYPAD:
		if(dinput!=nullptr && !dinput->isButtonDown(playerIndex,button)){
			return false;
		}
		break;
	case InputManager::XINPUT_JOYPAD:
		if(xinput!=nullptr && !xinput->isButtonDown(playerIndex,button)){
			return false;
		}
		break;
	case InputManager::KEYBOARD:
		if(dinput!=nullptr && !dinput->IsKeyDown(button)){
			return false;
		}
		break;
	default:
		return false;
		break;
	}

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(pair<Input,DWORD>(input,0));
		return false;
	}
	else if(iter->second==frames){
		return true;
	}
	return false;
}