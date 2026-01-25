#include "wiInput_Apple.h"

#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>

#include <mutex>

namespace wi::input::apple
{
static std::mutex locker;
static GCController* controllers[8] = {};

void Initialize()
{
	@autoreleasepool {
		
		// Register new controller event:
		[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
														  object:nil
														   queue:[NSOperationQueue mainQueue]
													  usingBlock:^(NSNotification *note) {
			GCController *controller = note.object;
			std::scoped_lock lck(locker);
			for (int i = 0; i < arraysize(controllers); ++i)
			{
				if (controllers[i] == nullptr)
				{
					controllers[i] = controller;
					break;
				}
			}
		}];

		// Remove controller event:
		[[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
														  object:nil
														   queue:[NSOperationQueue mainQueue]
													  usingBlock:^(NSNotification *note) {
			GCController *controller = note.object;
			std::scoped_lock lck(locker);
			for (int i = 0; i < arraysize(controllers); ++i)
			{
				if (controllers[i] == controller)
				{
					controllers[i] = nullptr;
					break;
				}
			}
		}];

		// Register already connected controllers:
		for (GCController *controller in [GCController controllers])
		{
			std::scoped_lock lck(locker);
			for (int i = 0; i < arraysize(controllers); ++i)
			{
				if (controllers[i] == nullptr)
				{
					controllers[i] = controller;
					break;
				}
			}
		}
	}
}

int GetMaxControllerCount()
{
	return arraysize(controllers);
}

constexpr float deadzone(float x)
{
	if (x > -0.24f && x < 0.24f)
		return 0.f;
	return clamp(x, -1.0f, 1.0f);
}


constexpr uint64_t buttonToBit(int button)
{
	return 1ull << (button - GAMEPAD_RANGE_START - 1);
}
bool GetControllerState(wi::input::ControllerState* state, int index)
{
	std::scoped_lock lck(locker);
	if (index < 0 || index >= arraysize(controllers))
		return false;

	GCController* controller = controllers[index];
	if (controller == nullptr)
		return false;
	
	GCExtendedGamepad* pad = controller.extendedGamepad;
	if (pad == nullptr)
		return false;

	if (state)
	{
		*state = wi::input::ControllerState();
		
		if (pad.dpad.up.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_UP);
		if (pad.dpad.left.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_LEFT);
		if (pad.dpad.down.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_DOWN);
		if (pad.dpad.right.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_RIGHT);

		// It looks like this exactly matches the xbox layout:
		if (pad.buttonX.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_X);
		if (pad.buttonA.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_A);
		if (pad.buttonB.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_B);
		if (pad.buttonY.pressed)	state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_Y);

		if (pad.leftShoulder.pressed)  state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_L1);
		if (pad.rightShoulder.pressed) state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_R1);

		if (pad.leftThumbstickButton.pressed)  state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_L3);
		if (pad.rightThumbstickButton.pressed) state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_R3);

		if (pad.buttonMenu.pressed) state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_START);
		if (pad.buttonOptions.pressed) state->buttons |= buttonToBit(GAMEPAD_BUTTON_XBOX_BACK);

		state->thumbstick_L = XMFLOAT2(deadzone(pad.leftThumbstick.xAxis.value), deadzone(pad.leftThumbstick.yAxis.value));
		state->thumbstick_R = XMFLOAT2(deadzone(pad.rightThumbstick.xAxis.value), -deadzone(pad.rightThumbstick.yAxis.value));

		state->trigger_L = pad.leftTrigger.value;
		state->trigger_R = pad.rightTrigger.value;
	}

	return true;
}

void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index)
{
	std::scoped_lock lck(locker);
	if (index < 0 || index >= arraysize(controllers))
		return;

	GCController* controller = controllers[index];
	if (controller == nullptr)
		return;
	
	if (controller.light)
	{
		XMFLOAT3 color = data.led_color.toFloat3();
		controller.light.color = [[GCColor alloc] initWithRed:color.x green:color.y blue:color.z];
	}
	
	// TODO: vibration
}

}
