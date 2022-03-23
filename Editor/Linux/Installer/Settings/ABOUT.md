# Settings  

A brief explanation of Wicked Engine's settings.

## About

This folder contains settings files optionally copied over to your Wicked
Engine installation directory. 

### Configuration

The `config.ini` file is responsible for a few options. All changes in the
`config.ini` file must be done with [integers](https://en.wikipedia.org/wiki/Integer),
I.E whole numbers. A value of **0** is equal to **false** and a value of **1**
is equal to **true**. This means the numbers are stand-ins for the
[boolean](https://en.wikipedia.org/wiki/Boolean_data_type) data type.

#### Variables
There's a lot of variables to change around to customise your `config.ini` file. 
Here's their documentation:

- The `enabled` variable toggles whether to enable or ignore the `config.ini`.
- The `x` and `y`
variables, which influence where your window appears when you open Wicked 
Engine.
- The `w` and `h` variables control the width and height of your Wicked Engine window.
- The `fullscreen`variable controls whether your window goes full-screen when opening Wicked Engine.
- The `borderless` variable determines if your window should have borders or not.
- The `allow_hdr` variable toggles whether HDR should be used by Wicked Engine.

### Start-up 

Wicked Engine automatically loads the `startup.lua` script if found. This script
is just like any other Wicked Engine Lua script. Rather than having set variables
for you to control, the script simply allows you do whatever you want. Have fun!

#### Profiling
By default the `startup.lua` script contains `SetProfilerEnabled(false)`, which
sets the profiler to **false**. In order to enable the profiler on start-up,
simply change `SetProfilerEnabled(false)` to `SetProfilerEnabled(true)`!


