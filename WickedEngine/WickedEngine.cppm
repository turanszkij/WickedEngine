#include <wiECS.h>
module;

#include "WickedEngine.h"

export module wicked_engine;

export namespace wi
{
	using wi::Application;
	using wi::GPUBVH;
	using wi::EmittedParticleSystem;
	using wi::HairParticleSystem;
	using wi::Ocean;
	using wi::Sprite;
	using wi::SpriteFont;
	using wi::Archive;
	using wi::Color;
	using wi::FadeManager;
	using wi::Resource;
	using wi::SpinLock;
	using wi::Timer;
	using wi::Canvas;

	namespace ecs
	{
		using wi::ecs::Entity;
		using wi::ecs::INVALID_ENTITY;
	}

	namespace gui
	{
		using wi::gui::GUI;
		using wi::gui::EventArgs;
		using wi::gui::Widget;
		using wi::gui::Button;
		using wi::gui::Label;
		using wi::gui::TextInputField;
		using wi::gui::Slider;
		using wi::gui::CheckBox;
		using wi::gui::ComboBox;
		using wi::gui::Window;
		using wi::gui::ColorPicker;
		using wi::gui::TreeList;
	}

	namespace primitive
	{
		using wi::primitive::AABB;
		using wi::primitive::Sphere;
		using wi::primitive::Capsule;
		using wi::primitive::Ray;
		using wi::primitive::Frustum;
		using wi::primitive::Hitbox2D;
	}

	namespace font
	{
		using wi::font::Params;
		using wi::font::Alignment;
	}

	namespace image
	{
		using wi::image::Params;
		using wi::image::STENCILMODE;
		using wi::image::STENCILREFMODE;
		using wi::image::SAMPLEMODE;
		using wi::image::QUALITY;
	}

	namespace eventhandler
	{
		using wi::eventhandler::EVENT_THREAD_SAFE_POINT;
		using wi::eventhandler::EVENT_RELOAD_SHADERS;
		using wi::eventhandler::EVENT_SET_VSYNC;
	}

	namespace math
	{
		using wi::math::IDENTITY_MATRIX;
	}
}
