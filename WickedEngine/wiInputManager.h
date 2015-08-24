#pragma once
#include "CommonInclude.h"

class wiXInput;
class wiDirectInput;
class wiRawInput;

class wiInputManager
{
public:
	static wiXInput* xinput;
	static wiDirectInput* dinput;
	static wiRawInput* rawinput;

	static void addXInput(wiXInput* input);
	static void addDirectInput(wiDirectInput* input);
	static void addRawInput(wiRawInput* input);

	static void Update();
	static void CleanUp();

	enum InputType{
		DIRECTINPUT_JOYPAD,
		XINPUT_JOYPAD,
		KEYBOARD,
		RAWINPUT_JOYPAD,
		INPUTTYPE_COUNT
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
	
	static bool down(DWORD button, InputType inputType = InputType::KEYBOARD, short playerindex = 0);
	static bool press(DWORD button, InputType inputType = InputType::KEYBOARD, short playerindex = 0);
	static bool hold(DWORD button, DWORD frames = 30, bool continuous = false, InputType inputType = InputType::KEYBOARD, short playerIndex = 0);
};

