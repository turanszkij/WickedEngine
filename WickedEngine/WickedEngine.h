#ifndef WICKEDENGINE
#define WICKEDENGINE
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
#include "wiPrimitive.h"
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
#include "wiGraphics.h"
#include "wiGraphicsDevice.h"
#include "wiGUI.h"
#include "wiArchive.h"
#include "wiSpinLock.h"
#include "wiRectPacker.h"
#include "wiProfiler.h"
#include "wiOcean.h"
#include "wiFFTGenerator.h"
#include "wiArguments.h"
#include "wiGPUBVH.h"
#include "wiGPUSortLib.h"
#include "wiJobSystem.h"
#include "wiNetwork.h"
#include "wiEventHandler.h"
#include "wiShaderCompiler.h"
#include "wiCanvas.h"
#include "wiUnorderedMap.h"
#include "wiUnorderedSet.h"
#include "wiVector.h"
#include "wiNoise.h"
#include "wiConfig.h"
#include "wiTerrain.h"
#include "wiLocalization.h"
#include "wiVideo.h"
#include "wiVoxelGrid.h"
#include "wiPathQuery.h"
#include "wiTrailRenderer.h"

#ifdef PLATFORM_WINDOWS_DESKTOP
#pragma comment(lib,"WickedEngine_Windows.lib")
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_XBOX
#pragma comment(lib,"WickedEngine_XBOX.lib")
#endif // PLATFORM_XBOX

#ifdef PLATFORM_PS5
#pragma comment(lib,"WickedEngine_PS5.a")
#endif // PLATFORM_PS5


// After version 0.59.11, namespaces were refactored into nested namespaces under the wi:: root namespace.
// To allow compatibility with older user code, the backwards compatibility definitions are included below.
// If you need backwards compatibility features, define the following before including this file:
//#define WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59
#ifdef WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59

using namespace wi;
using namespace wi::enums;

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
namespace wiResourceManager = wi::resourcemanager;
namespace wiStartupArguments = wi::arguments;
namespace wiInput = wi::input;
namespace wiXInput = wi::input::xinput;
namespace wiRawInput = wi::input::rawinput;
namespace wiSDLInput = wi::input::sdlinput;
namespace wiAudio = wi::audio;
namespace wiNetwork = wi::network;
namespace wiPhysicsEngine = wi::physics;
namespace wiLua = wi::lua;
namespace wiECS = wi::ecs;
namespace wiEvent = wi::eventhandler;
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
using wiGUI = wi::gui::GUI;
using wiEventArgs = wi::gui::EventArgs;
using wiWidget = wi::gui::Widget;
using wiButton = wi::gui::Button;
using wiLabel = wi::gui::Label;
using wiTextInputField = wi::gui::TextInputField;
using wiSlider = wi::gui::Slider;
using wiCheckBox = wi::gui::CheckBox;
using wiComboBox = wi::gui::ComboBox;
using wiWindow = wi::gui::Window;
using wiColorPicker = wi::gui::ColorPicker;
using wiTreeList = wi::gui::TreeList;
using wiArchive = wi::Archive;
using wiColor = wi::Color;
using wiFadeManager = wi::FadeManager;
using wiResource = wi::Resource;
using wiSpinLock = wi::SpinLock;
using wiTimer = wi::Timer;
using wiCanvas = wi::Canvas;
using AABB = wi::primitive::AABB;
using SPHERE = wi::primitive::Sphere;
using CAPSULE = wi::primitive::Capsule;
using RAY = wi::primitive::Ray;
using Frustum = wi::primitive::Frustum;
using Hitbox2D = wi::primitive::Hitbox2D;

using wi::image::STENCILMODE;
using wi::image::STENCILREFMODE;
using wi::image::SAMPLEMODE;
using wi::image::QUALITY;
using wi::font::Alignment;

static constexpr int SYSTEM_EVENT_THREAD_SAFE_POINT = wi::eventhandler::EVENT_THREAD_SAFE_POINT;
static constexpr int SYSTEM_EVENT_RELOAD_SHADERS = wi::eventhandler::EVENT_RELOAD_SHADERS;
static constexpr int SYSTEM_EVENT_SET_VSYNC = wi::eventhandler::EVENT_SET_VSYNC;
static constexpr XMFLOAT4X4 IDENTITYMATRIX = wi::math::IDENTITY_MATRIX;

#endif // WICKEDENGINE_BACKWARDS_COMPATIBILITY_0_59



#endif // WICKEDENGINE
