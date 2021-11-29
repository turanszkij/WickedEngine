#pragma once
#include "CommonInclude.h"
#include "wiMath.h"

namespace wi
{
	struct Color
	{
		uint32_t rgba = 0;

		constexpr Color(uint32_t rgba) :rgba(rgba) {}
		constexpr Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : rgba((r << 0) | (g << 8) | (b << 16) | (a << 24)) {}

		constexpr uint8_t getR() const { return (rgba >> 0) & 0xFF; }
		constexpr uint8_t getG() const { return (rgba >> 8) & 0xFF; }
		constexpr uint8_t getB() const { return (rgba >> 16) & 0xFF; }
		constexpr uint8_t getA() const { return (rgba >> 24) & 0xFF; }

		constexpr void setR(uint8_t value) { *this = Color(value, getG(), getB(), getA()); }
		constexpr void setG(uint8_t value) { *this = Color(getR(), value, getB(), getA()); }
		constexpr void setB(uint8_t value) { *this = Color(getR(), getG(), value, getA()); }
		constexpr void setA(uint8_t value) { *this = Color(getR(), getG(), getB(), value); }

		constexpr XMFLOAT3 toFloat3() const
		{
			return XMFLOAT3(
				((rgba >> 0) & 0xFF) / 255.0f,
				((rgba >> 8) & 0xFF) / 255.0f,
				((rgba >> 16) & 0xFF) / 255.0f
			);
		}
		constexpr XMFLOAT4 toFloat4() const
		{
			return XMFLOAT4(
				((rgba >> 0) & 0xFF) / 255.0f,
				((rgba >> 8) & 0xFF) / 255.0f,
				((rgba >> 16) & 0xFF) / 255.0f,
				((rgba >> 24) & 0xFF) / 255.0f
			);
		}
		constexpr operator XMFLOAT3() const { return toFloat3(); }
		constexpr operator XMFLOAT4() const { return toFloat4(); }
		constexpr operator uint32_t() const { return rgba; }

		static constexpr Color fromFloat4(const XMFLOAT4& value)
		{
			return Color((uint8_t)(value.x * 255), (uint8_t)(value.y * 255), (uint8_t)(value.z * 255), (uint8_t)(value.w * 255));
		}
		static constexpr Color fromFloat3(const XMFLOAT3& value)
		{
			return Color((uint8_t)(value.x * 255), (uint8_t)(value.y * 255), (uint8_t)(value.z * 255));
		}

		static constexpr Color lerp(Color a, Color b, float i)
		{
			return fromFloat4(wi::math::Lerp(a.toFloat4(), b.toFloat4(), i));
		}

		static constexpr Color Red() { return Color(255, 0, 0, 255); }
		static constexpr Color Green() { return Color(0, 255, 0, 255); }
		static constexpr Color Blue() { return Color(0, 0, 255, 255); }
		static constexpr Color Black() { return Color(0, 0, 0, 255); }
		static constexpr Color White() { return Color(255, 255, 255, 255); }
		static constexpr Color Yellow() { return Color(255, 255, 0, 255); }
		static constexpr Color Purple() { return Color(255, 0, 255, 255); }
		static constexpr Color Cyan() { return Color(0, 255, 255, 255); }
		static constexpr Color Transparent() { return Color(0, 0, 0, 0); }
		static constexpr Color Gray() { return Color(127, 127, 127, 255); }
		static constexpr Color Ghost() { return Color(127, 127, 127, 127); }
		static constexpr Color Booger() { return Color(127, 127, 127, 200); }
	};
}

