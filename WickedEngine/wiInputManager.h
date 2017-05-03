#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"

#include <vector>
#include <map>

class wiXInput;
class wiDirectInput;
class wiRawInput;

class wiInputManager : public wiThreadSafeManager
{
public:
	wiInputManager();
	~wiInputManager();

	static wiInputManager* GetInstance();

	wiXInput* xinput;
	wiDirectInput* dinput;
	wiRawInput* rawinput;

	void addXInput(wiXInput* input);
	void addDirectInput(wiDirectInput* input);
	void addRawInput(wiRawInput* input);

	void Update();
	void CleanUp();

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
	typedef std::map<Input,DWORD,Input::LessComparer> InputCollection;
	InputCollection inputs;
	
	//check if a button is down
	bool down(DWORD button, InputType inputType = InputType::KEYBOARD, short playerindex = 0);
	//check if a button is pressed once
	bool press(DWORD button, InputType inputType = InputType::KEYBOARD, short playerindex = 0);
	//check if a button is held down
	bool hold(DWORD button, DWORD frames = 30, bool continuous = false, InputType inputType = InputType::KEYBOARD, short playerIndex = 0);
	//get pointer position (eg. mouse pointer) (.xy) + scroll delta (.z) + 1 unused (.w)
	XMFLOAT4 getpointer();
	//set pointer position (eg. mouse pointer) + scroll delta (.z)
	void setpointer(const XMFLOAT4& props);
	//hide pointer
	void hidepointer(bool value);

	struct Touch
	{
		enum TouchState
		{
			TOUCHSTATE_PRESSED,
			TOUCHSTATE_RELEASED,
			TOUCHSTATE_MOVED,
			TOUCHSTATE_COUNT,
		} state;
		// current position of touch
		XMFLOAT2 pos;
	};
	std::vector<Touch> getTouches();

private:
	std::vector<Touch> touches;

	friend void AddTouch(const wiInputManager::Touch& touch);

};

