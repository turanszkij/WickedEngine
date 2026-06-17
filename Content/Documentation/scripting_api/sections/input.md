# Input

[← Back to Scripting API index](../index.md)

```lua
    --- Creates an Input object. Use the global `input` instead of constructing
    --- one.
    ---
    ---@return Input
    function Input() end

    --- Provides access to keyboard, mouse, touch and gamepad input. Use the
    --- global `input` object.
    ---
    ---@class Input
    local Input = {}

    --- Use this global object to access input functions.
    ---
    ---@type Input
    input = nil

    --- Check whether a button is currently being held down.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Down(code, playerindex) end

    --- Check whether a button has just been pushed that wasn't before.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Press(code, playerindex) end

    --- Check whether a button has just been released that was down before.
    ---
    ---@param code integer
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Release(code, playerindex) end

    --- Check whether a button was being held down for a specific duration
    --- (number of frames). If continuous == true, than it will also return true
    --- after the duration was reached.
    ---
    ---@param code integer
    ---@param duration? integer
    ---@param continuous? boolean
    ---@param playerindex? integer
    ---
    ---@return boolean
    function Input.Hold(code, duration, continuous, playerindex) end

    --- Get mouse pointer or primary touch position (x, y). Also returns mouse
    --- wheel delta movement (z), and pen pressure (w).
    ---
    ---@return Vector
    function Input.GetPointer() end

    --- Set mouse position.
    ---
    ---@param pos Vector
    function Input.SetPointer(pos) end

    --- Native delta mouse movement.
    ---
    ---@return Vector
    function Input.GetPointerDelta() end

    --- Hide Pointer.
    ---
    ---@param visible boolean
    function Input.HidePointer(visible) end

    --- Read analog data from gamepad. type parameter must be from
    --- GAMEPAD_ANALOG values.
    ---
    ---@param type integer
    ---@param playerindex? integer
    ---
    ---@return Vector
    function Input.GetAnalog(type, playerindex) end

    --- Returns the touches.
    ---
    ---@return Touch
    function Input.GetTouches() end

    --- Sets controller feedback such as vibration or LED color.
    ---
    ---@param feedback ControllerFeedback
    ---@param playerindex? integer
    function Input.SetControllerFeedback(feedback, playerindex) end

    --- Returns 0 (`BUTTON_NONE`) if nothing is pressed, or the first
    --- appropriate button code otherwise.
    ---
    ---@param playerindex? integer
    ---
    ---@return integer
    function Input.WhatIsPressed(playerindex) end

    --- Returns whether that button code is a gamepad button or not.
    ---
    ---@param button integer
    ---
    ---@return boolean
    function Input.IsGamepadButton(button) end

    --- Returns button code for a given string name.
    ---
    ---@param str string
    ---
    ---@return integer
    function Input.StringToButton(str) end

    --- Returns string name for the given button code. You can set a preference
    --- for controller type which can modify the string returned.
    ---
    ---@param button integer
    ---@param preference? integer
    ---
    ---@return any
    function Input.ButtonToString(button, preference) end

    --- Sets the current cursor type. Values can be of the cursor values, see
    --- below.
    ---
    ---@param cursor integer
    function Input.SetCursor(cursor) end

    --- Sets the specified cursor type to an image from a cursor file.
    ---
    ---@param cursor integer
    ---@param filename string
    function Input.SetCursorFromFile(cursor, filename) end

    --- Resets the specified cursor to the default one.
    ---
    ---@param cursor integer
    function Input.ResetCursor(cursor) end

    --- Resets all cursors to the defaults.
    function Input.ResetCursors() end
```

## ControllerFeedback

```lua
    --- Controller Feedback.
    ---
    ---@return ControllerFeedback
    function ControllerFeedback() end

    --- Describes controller feedback such as touch and LED color which can be
    --- replayed on a controller.
    ---
    ---@class ControllerFeedback
    local ControllerFeedback = {}

    --- Vibration amount of left motor (0: no vibration, 1: max vibration).
    ---
    ---@param value number
    function ControllerFeedback.SetVibrationLeft(value) end

    --- Vibration amount of right motor (0: no vibration, 1: max vibration).
    ---
    ---@param value number
    function ControllerFeedback.SetVibrationRight(value) end

    --- Sets the colored LED color if controller has one.
    ---
    ---@param color Vector
    function ControllerFeedback.SetLEDColor(color) end

    --- Sets the colored LED color if controller has one (ABGR hex color code).
    ---
    ---@param hexcolor integer
    function ControllerFeedback.SetLEDColor(hexcolor) end
```

## Touch

```lua
    --- Creates a Touch contact descriptor.
    ---
    ---@return Touch
    function Touch() end

    --- Describes a touch contact point
    ---
    ---@class Touch
    local Touch = {}

    --- Returns the touch state (one of the TOUCHSTATE_* values).
    ---
    ---@return integer
    function Touch.GetState() end

    --- Returns the touch position.
    ---
    ---@return Vector
    function Touch.GetPos() end
```

## TOUCHSTATE

```lua
    --- TOUCHSTATE PRESSED.
    ---
    ---@type integer
    TOUCHSTATE_PRESSED = 0

    --- TOUCHSTATE RELEASED.
    ---
    ---@type integer
    TOUCHSTATE_RELEASED = 1

    --- TOUCHSTATE MOVED.
    ---
    ---@type integer
    TOUCHSTATE_MOVED = 2
```

## Keyboard Key codes

You can also generate a key code by calling string.byte(char uppercaseLetter)
where the parameter represents the desired key of the keyboard.

```lua
    --- No button (sentinel value for "no input").
    ---
    ---@type integer
    BUTTON_NONE = 0

    --- KEYBOARD BUTTON UP.
    ---
    ---@type integer
    KEYBOARD_BUTTON_UP = 518

    --- KEYBOARD BUTTON DOWN.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DOWN = 519

    --- KEYBOARD BUTTON LEFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LEFT = 520

    --- KEYBOARD BUTTON RIGHT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RIGHT = 521

    --- KEYBOARD BUTTON SPACE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SPACE = 522

    --- KEYBOARD BUTTON RSHIFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RSHIFT = 523

    --- KEYBOARD BUTTON LSHIFT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LSHIFT = 524

    --- KEYBOARD BUTTON F1.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F1 = 525

    --- KEYBOARD BUTTON F2.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F2 = 526

    --- KEYBOARD BUTTON F3.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F3 = 527

    --- KEYBOARD BUTTON F4.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F4 = 528

    --- KEYBOARD BUTTON F5.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F5 = 529

    --- KEYBOARD BUTTON F6.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F6 = 530

    --- KEYBOARD BUTTON F7.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F7 = 531

    --- KEYBOARD BUTTON F8.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F8 = 532

    --- KEYBOARD BUTTON F9.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F9 = 533

    --- KEYBOARD BUTTON F10.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F10 = 534

    --- KEYBOARD BUTTON F11.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F11 = 535

    --- KEYBOARD BUTTON F12.
    ---
    ---@type integer
    KEYBOARD_BUTTON_F12 = 536

    --- KEYBOARD BUTTON ENTER.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ENTER = 537

    --- KEYBOARD BUTTON ESCAPE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ESCAPE = 538

    --- KEYBOARD BUTTON BACK (backspace).
    ---
    ---@type integer
    KEYBOARD_BUTTON_BACK = 543

    --- KEYBOARD BUTTON HOME.
    ---
    ---@type integer
    KEYBOARD_BUTTON_HOME = 539

    --- KEYBOARD BUTTON RCONTROL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_RCONTROL = 540

    --- KEYBOARD BUTTON LCONTROL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_LCONTROL = 541

    --- KEYBOARD BUTTON DELETE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DELETE = 542

    --- KEYBOARD BUTTON BACKSPACE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_BACKSPACE = 0

    --- KEYBOARD BUTTON PAGEDOWN.
    ---
    ---@type integer
    KEYBOARD_BUTTON_PAGEDOWN = 544

    --- KEYBOARD BUTTON PAGEUP.
    ---
    ---@type integer
    KEYBOARD_BUTTON_PAGEUP = 545

    --- KEYBOARD BUTTON NUMPAD0.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD0 = 546

    --- KEYBOARD BUTTON NUMPAD1.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD1 = 547

    --- KEYBOARD BUTTON NUMPAD2.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD2 = 548

    --- KEYBOARD BUTTON NUMPAD3.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD3 = 549

    --- KEYBOARD BUTTON NUMPAD4.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD4 = 550

    --- KEYBOARD BUTTON NUMPAD5.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD5 = 551

    --- KEYBOARD BUTTON NUMPAD6.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD6 = 552

    --- KEYBOARD BUTTON NUMPAD7.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD7 = 553

    --- KEYBOARD BUTTON NUMPAD8.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD8 = 554

    --- KEYBOARD BUTTON NUMPAD9.
    ---
    ---@type integer
    KEYBOARD_BUTTON_NUMPAD9 = 555

    --- KEYBOARD BUTTON MULTIPLY.
    ---
    ---@type integer
    KEYBOARD_BUTTON_MULTIPLY = 556

    --- KEYBOARD BUTTON ADD.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ADD = 557

    --- KEYBOARD BUTTON SEPARATOR.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SEPARATOR = 558

    --- KEYBOARD BUTTON SUBTRACT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_SUBTRACT = 559

    --- KEYBOARD BUTTON DECIMAL.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DECIMAL = 560

    --- KEYBOARD BUTTON DIVIDE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_DIVIDE = 561

    --- KEYBOARD BUTTON TAB.
    ---
    ---@type integer
    KEYBOARD_BUTTON_TAB = 562

    --- KEYBOARD BUTTON TILDE.
    ---
    ---@type integer
    KEYBOARD_BUTTON_TILDE = 563

    --- KEYBOARD BUTTON INSERT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_INSERT = 564

    --- KEYBOARD BUTTON ALT.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ALT = 565

    --- KEYBOARD BUTTON ALTGR.
    ---
    ---@type integer
    KEYBOARD_BUTTON_ALTGR = 566
```

## Mouse Key Codes

Helpers to check mouse wheel scrolling like buttons.

```lua
    --- MOUSE BUTTON LEFT.
    ---
    ---@type integer
    MOUSE_BUTTON_LEFT = 513

    --- MOUSE BUTTON RIGHT.
    ---
    ---@type integer
    MOUSE_BUTTON_RIGHT = 514

    --- MOUSE BUTTON MIDDLE.
    ---
    ---@type integer
    MOUSE_BUTTON_MIDDLE = 515

    --- MOUSE SCROLL AS BUTTON UP.
    ---
    ---@type integer
    MOUSE_SCROLL_AS_BUTTON_UP = 516

    --- MOUSE SCROLL AS BUTTON DOWN.
    ---
    ---@type integer
    MOUSE_SCROLL_AS_BUTTON_DOWN = 517
```

## Gamepad Key Codes

Generic codes work across controllers; the Xbox and PlayStation codes below
are aliases mapping to the generic ones.

### Generic button codes

```lua
    --- Gamepad D-pad up.
    ---
    ---@type integer
    GAMEPAD_BUTTON_UP = 257

    --- Gamepad D-pad down.
    ---
    ---@type integer
    GAMEPAD_BUTTON_DOWN = 259

    --- Gamepad D-pad left.
    ---
    ---@type integer
    GAMEPAD_BUTTON_LEFT = 258

    --- Gamepad D-pad right.
    ---
    ---@type integer
    GAMEPAD_BUTTON_RIGHT = 260

    --- Generic gamepad button 1.
    ---
    ---@type integer
    GAMEPAD_BUTTON_1 = 261

    --- Generic gamepad button 2.
    ---
    ---@type integer
    GAMEPAD_BUTTON_2 = 262

    --- Generic gamepad button 3.
    ---
    ---@type integer
    GAMEPAD_BUTTON_3 = 263

    --- Generic gamepad button 4.
    ---
    ---@type integer
    GAMEPAD_BUTTON_4 = 264

    --- Generic gamepad button 5.
    ---
    ---@type integer
    GAMEPAD_BUTTON_5 = 265

    --- Generic gamepad button 6.
    ---
    ---@type integer
    GAMEPAD_BUTTON_6 = 266

    --- Generic gamepad button 7.
    ---
    ---@type integer
    GAMEPAD_BUTTON_7 = 267

    --- Generic gamepad button 8.
    ---
    ---@type integer
    GAMEPAD_BUTTON_8 = 268

    --- Generic gamepad button 9.
    ---
    ---@type integer
    GAMEPAD_BUTTON_9 = 269

    --- Generic gamepad button 10.
    ---
    ---@type integer
    GAMEPAD_BUTTON_10 = 270

    --- Generic gamepad button 11.
    ---
    ---@type integer
    GAMEPAD_BUTTON_11 = 271

    --- Generic gamepad button 12.
    ---
    ---@type integer
    GAMEPAD_BUTTON_12 = 272

    --- Generic gamepad button 13.
    ---
    ---@type integer
    GAMEPAD_BUTTON_13 = 273

    --- Generic gamepad button 14.
    ---
    ---@type integer
    GAMEPAD_BUTTON_14 = 274

    -- Analog sticks and triggers exposed as digital buttons:

    --- Left thumbstick pushed up, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_UP = 275

    --- Left thumbstick pushed down, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_DOWN = 277

    --- Left thumbstick pushed left, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_LEFT = 276

    --- Left thumbstick pushed right, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_RIGHT = 278

    --- Right thumbstick pushed up, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_UP = 279

    --- Right thumbstick pushed down, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_DOWN = 281

    --- Right thumbstick pushed left, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_LEFT = 280

    --- Right thumbstick pushed right, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_RIGHT = 282

    --- Left trigger, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON = 283

    --- Right trigger, as a button.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON = 284
```

### Xbox button codes

```lua
    --- Xbox X button (alias of GAMEPAD_BUTTON_1).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_X = 261

    --- Xbox A button (alias of GAMEPAD_BUTTON_2).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_A = 262

    --- Xbox B button (alias of GAMEPAD_BUTTON_3).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_B = 263

    --- Xbox Y button (alias of GAMEPAD_BUTTON_4).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_Y = 264

    --- Xbox L1 button (alias of GAMEPAD_BUTTON_5).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_L1 = 265

    --- Xbox LT button (alias of GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_LT = 283

    --- Xbox R1 button (alias of GAMEPAD_BUTTON_6).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_R1 = 266

    --- Xbox RT button (alias of GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_RT = 284

    --- Xbox L3 button (alias of GAMEPAD_BUTTON_7).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_L3 = 267

    --- Xbox R3 button (alias of GAMEPAD_BUTTON_8).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_R3 = 268

    --- Xbox BACK button (alias of GAMEPAD_BUTTON_9).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_BACK = 269

    --- Xbox START button (alias of GAMEPAD_BUTTON_10).
    ---
    ---@type integer
    GAMEPAD_BUTTON_XBOX_START = 270
```

### PlayStation button codes

```lua
    --- PlayStation SQUARE button (alias of GAMEPAD_BUTTON_1).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SQUARE = 261

    --- PlayStation CROSS button (alias of GAMEPAD_BUTTON_2).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_CROSS = 262

    --- PlayStation CIRCLE button (alias of GAMEPAD_BUTTON_3).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_CIRCLE = 263

    --- PlayStation TRIANGLE button (alias of GAMEPAD_BUTTON_4).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE = 264

    --- PlayStation L1 button (alias of GAMEPAD_BUTTON_5).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L1 = 265

    --- PlayStation L2 button (alias of GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L2 = 283

    --- PlayStation R1 button (alias of GAMEPAD_BUTTON_6).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R1 = 266

    --- PlayStation R2 button (alias of GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R2 = 284

    --- PlayStation L3 button (alias of GAMEPAD_BUTTON_7).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_L3 = 267

    --- PlayStation R3 button (alias of GAMEPAD_BUTTON_8).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_R3 = 268

    --- PlayStation SHARE button (alias of GAMEPAD_BUTTON_9).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SHARE = 269

    --- PlayStation OPTION button (alias of GAMEPAD_BUTTON_10).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_OPTION = 270

    --- PlayStation SELECT button (alias of GAMEPAD_BUTTON_PLAYSTATION_SHARE).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_SELECT = 269

    --- PlayStation START button (alias of GAMEPAD_BUTTON_PLAYSTATION_OPTION).
    ---
    ---@type integer
    GAMEPAD_BUTTON_PLAYSTATION_START = 270
```

## Gamepad Analog Codes

```lua
    --- GAMEPAD ANALOG THUMBSTICK L.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_L = 0

    --- GAMEPAD ANALOG THUMBSTICK R.
    ---
    ---@type integer
    GAMEPAD_ANALOG_THUMBSTICK_R = 1

    --- GAMEPAD ANALOG TRIGGER L.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_L = 2

    --- GAMEPAD ANALOG TRIGGER R.
    ---
    ---@type integer
    GAMEPAD_ANALOG_TRIGGER_R = 3
```

## Controller preference

```lua
    --- CONTROLLER PREFERENCE GENERIC.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_GENERIC = 0

    --- CONTROLLER PREFERENCE PLAYSTATION.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_PLAYSTATION = 1

    --- CONTROLLER PREFERENCE XBOX.
    ---
    ---@type integer
    CONTROLLER_PREFERENCE_XBOX = 2
```

## Cursor codes

```lua
    --- CURSOR DEFAULT.
    ---
    ---@type integer
    CURSOR_DEFAULT = 0

    --- CURSOR TEXTINPUT.
    ---
    ---@type integer
    CURSOR_TEXTINPUT = 1

    --- CURSOR RESIZEALL.
    ---
    ---@type integer
    CURSOR_RESIZEALL = 2

    --- CURSOR RESIZE NS.
    ---
    ---@type integer
    CURSOR_RESIZE_NS = 3

    --- CURSOR RESIZE EW.
    ---
    ---@type integer
    CURSOR_RESIZE_EW = 4

    --- CURSOR RESIZE NESW.
    ---
    ---@type integer
    CURSOR_RESIZE_NESW = 5

    --- CURSOR RESIZE NWSE.
    ---
    ---@type integer
    CURSOR_RESIZE_NWSE = 6

    --- CURSOR HAND.
    ---
    ---@type integer
    CURSOR_HAND = 7

    --- CURSOR NOTALLOWED.
    ---
    ---@type integer
    CURSOR_NOTALLOWED = 8

    --- CURSOR CROSS.
    ---
    ---@type integer
    CURSOR_CROSS = 9
```
