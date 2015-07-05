#pragma once
#include "WickedEngine.h"

class XInput;
class DirectInput;

class wiInputManager
{
public:
	static XInput* xinput;
	static DirectInput* dinput;

	static void addXInput(XInput* input);
	static void addDirectInput(DirectInput* input);

	static void Update();
	static void CleanUp();

	enum InputType{
		DIRECTINPUT_JOYPAD,
		XINPUT_JOYPAD,
		KEYBOARD,
	};
	struct Input{
		InputType type;
		DWORD button;
		short playerIndex;

		Input(){
			type=InputType::DIRECTINPUT_JOYPAD;
			button=(DWORD)0;
			playerIndex=0;
		}
		bool operator<(const Input other){
			return (button!=other.button || type!=other.type || playerIndex!=other.playerIndex);
		}
		struct LessComparer{
			bool operator()(Input const& a,Input const& b) const{
				return (a.button<b.button || a.type<b.type || a.playerIndex<b.playerIndex);
			}
		};
	};
	typedef map<Input,DWORD,Input::LessComparer> InputCollection;
	static InputCollection inputs;
	
	
	static bool press(DWORD button);
	static bool press(DWORD button, short playerIndex, InputType inputType = InputType::DIRECTINPUT_JOYPAD);
	static bool hold(DWORD button, DWORD frames);
	static bool hold(DWORD button, DWORD frames, short playerIndex, InputType inputType = InputType::DIRECTINPUT_JOYPAD);
};

