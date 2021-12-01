#ifndef WICKED_ENGINE
#define WICKED_ENGINE

// NOTE:
// The purpose of this file is to expose all engine features.
// It should be included in the engine's implementing application not the engine itself!
// It should be included in the precompiled header if available.

#include "CommonInclude.h"

// High-level interface:
#include "wiApplication.h"
#include "wiRenderPath.h"
#include "wiRenderPath2D.h"
#include "wiRenderPath3D.h"
#include "wiRenderPath3D_PathTracing.h"
#include "wiLoadingScreen.h"

// Engine-level systems
#include "wiVersion.h"
#include "wiPlatform.h"
#include "wiBacklog.h"
#include "wiIntersect.h"
#include "wiImage.h"
#include "wiFont.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiScene.h"
#include "wiECS.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiRenderer.h"
#include "wiMath.h"
#include "wiAudio.h"
#include "wiResourceManager.h"
#include "wiTimer.h"
#include "wiHelper.h"
#include "wiInput.h"
#include "wiRawInput.h"
#include "wiXInput.h"
#include "wiSDLInput.h"
#include "wiTextureHelper.h"
#include "wiRandom.h"
#include "wiColor.h"
#include "wiPhysics.h"
#include "wiEnums.h"
#include "wiInitializer.h"
#include "wiLua.h"
#include "wiLuna.h"
#include "wiGraphics.h"
#include "wiGraphicsDevice.h"
#include "wiGUI.h"
#include "wiWidget.h"
#include "wiArchive.h"
#include "wiSpinLock.h"
#include "wiRectPacker.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "wiFFTGenerator.h"
#include "wiStartupArguments.h"
#include "wiGPUBVH.h"
#include "wiGPUSortLib.h"
#include "wiJobSystem.h"
#include "wiNetwork.h"
#include "wiEvent.h"
#include "wiShaderCompiler.h"
#include "wiCanvas.h"
#include "wiUnorderedMap.h"
#include "wiUnorderedSet.h"
#include "wiVector.h"

#ifdef _WIN32
#ifdef PLATFORM_UWP
#pragma comment(lib,"WickedEngine_UWP.lib")
#else
#pragma comment(lib,"WickedEngine_Windows.lib")
#endif // PLATFORM_UWP
#endif // _WIN32


// After version 0.59.11, namespaces were refactored into nested namespaces under the wi:: root namespace.
// To allow compatibility with older user code, the backwards compatibility definitions are included below.
// If you need backwards compatibility features, define the following before including this file:
#define WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59_11
#ifdef WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59_11

using namespace wi;

namespace wiGraphics = wi::graphics;
namespace wiShaderCompiler = wi::shadercompiler;
namespace wiFFTGenerator = wi::fftgenerator;
namespace wiFont = wi::font;
namespace wiImage = wi::image;
namespace wiGPUSortLib = wi::gpusortlib;
namespace wiRenderer = wi::renderer;
namespace wiTextureHelper = wi::texturehelper;
namespace wiHelper = wi::helper;
namespace wiMath = wi::math;
namespace wiRandom = wi::random;
namespace wiRectPacker = wi::rectpacker;
namespace wiResourceManager = wi::resource_manager;
namespace wiStartupArguments = wi::startup_arguments;
namespace wiInput = wi::input;
namespace wiXInput = wi::input::xinput;
namespace wiRawInput = wi::input::rawinput;
namespace wiSDLInput = wi::input::sdlinput;
namespace wiAudio = wi::audio;
namespace wiNetwork = wi::network;
namespace wiPhysicsEngine = wi::physics;
namespace wiLua = wi::lua;
namespace wiECS = wi::ecs;
namespace wiEvent = wi::event;
namespace wiInitializer = wi::initializer;
namespace wiJobSystem = wi::jobsystem;
namespace wiPlatform = wi::platform;
namespace wiScene = wi::scene;
namespace wiBackLog = wi::backlog;
namespace wiProfiler = wi::profiler;
namespace wiVersion = wi::version;

using MainComponent = wi::Application;
using wiFontParams = wi::font::Params;
using wiImageParams = wi::image::Params;
using wiGPUBVH = wi::GPUBVH;
using wiEmittedParticle = wi::EmittedParticleSystem;
using wiHairParticle = wi::HairParticleSystem;
using wiOcean = wi::Ocean;
using wiSprite = wi::Sprite;
using wiSpriteFont = wi::SpriteFont;
using wiGUI = wi::GUI;
using wiEventArgs = wi::widget::EventArgs;
using wiWidget = wi::widget::Widget;
using wiButton = wi::widget::Button;
using wiLabel = wi::widget::Label;
using wiTextInputField = wi::widget::TextInputField;
using wiSlider = wi::widget::Slider;
using wiCheckBox = wi::widget::CheckBox;
using wiComboBox = wi::widget::ComboBox;
using wiWindow = wi::widget::Window;
using wiColorPicker = wi::widget::ColorPicker;
using wiTreeList = wi::widget::TreeList;
using wiArchive = wi::Archive;
using wiColor = wi::Color;
using wiFadeManager = wi::FadeManager;
using wiResource = wi::Resource;
using wiSpinLock = wi::SpinLock;
using wiTimer = wi::Timer;
using wiCanvas = wi::Canvas;

using wi::image::STENCILMODE;
using wi::image::STENCILREFMODE;
using wi::image::SAMPLEMODE;
using wi::image::QUALITY;
using wi::font::Alignment;

#endif // WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59_11



#endif // WICKED_ENGINE
