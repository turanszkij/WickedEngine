#pragma once
#include "CommonInclude.h"
#include "wiMath.h"

struct wiColor
{
	uint32_t rgba = 0;

	constexpr wiColor(uint32_t rgba) :rgba(rgba) {}
	constexpr wiColor(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : rgba((r << 0) | (g << 8) | (b << 16) | (a << 24)) {}

	constexpr uint8_t getR() const { return (rgba >> 0) & 0xFF; }
	constexpr uint8_t getG() const { return (rgba >> 8) & 0xFF; }
	constexpr uint8_t getB() const { return (rgba >> 16) & 0xFF; }
	constexpr uint8_t getA() const { return (rgba >> 24) & 0xFF; }

	constexpr void setR(uint8_t value) { *this = wiColor(value, getG(), getB(), getA()); }
	constexpr void setG(uint8_t value) { *this = wiColor(getR(), value, getB(), getA()); }
	constexpr void setB(uint8_t value) { *this = wiColor(getR(), getG(), value, getA()); }
	constexpr void setA(uint8_t value) { *this = wiColor(getR(), getG(), getB(), value); }

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

	static constexpr wiColor fromFloat4(const XMFLOAT4& value)
	{
		return wiColor((uint8_t)(value.x * 255), (uint8_t)(value.y * 255), (uint8_t)(value.z * 255), (uint8_t)(value.w * 255));
	}
	static constexpr wiColor fromFloat3(const XMFLOAT3& value)
	{
		return wiColor((uint8_t)(value.x * 255), (uint8_t)(value.y * 255), (uint8_t)(value.z * 255));
	}

	static constexpr wiColor lerp(wiColor a, wiColor b, float i)
	{
		return fromFloat4(wiMath::Lerp(a.toFloat4(), b.toFloat4(), i));
	}

	static constexpr wiColor Red() { return wiColor(255, 0, 0, 255); }
	static constexpr wiColor Green() { return wiColor(0, 255, 0, 255); }
	static constexpr wiColor Blue() { return wiColor(0, 0, 255, 255); }
	static constexpr wiColor Black() { return wiColor(0, 0, 0, 255); }
	static constexpr wiColor White() { return wiColor(255, 255, 255, 255); }
	static constexpr wiColor Yellow() { return wiColor(255, 255, 0, 255); }
	static constexpr wiColor Purple() { return wiColor(255, 0, 255, 255); }
	static constexpr wiColor Cyan() { return wiColor(0, 255, 255, 255); }
	static constexpr wiColor Transparent() { return wiColor(0, 0, 0, 0); }
	static constexpr wiColor Gray() { return wiColor(127, 127, 127, 255); }
	static constexpr wiColor Ghost() { return wiColor(127, 127, 127, 127); }
	static constexpr wiColor Booger() { return wiColor(127, 127, 127, 200); }
};


