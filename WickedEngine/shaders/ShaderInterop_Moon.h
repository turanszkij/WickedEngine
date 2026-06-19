#ifndef WI_SHADERINTEROP_MOON_H
#define WI_SHADERINTEROP_MOON_H
#include "ShaderInterop.h"

/**
 * @brief GPU-side representation of the moon, nested inside ShaderWeather and
 * uploaded as part of the per-frame constant buffer. Shared between C++ and
 * HLSL, so the layout must stay 16-byte aligned and obey constant-buffer
 * packing rules (no member may straddle a 16-byte boundary).
 */
struct alignas(16) ShaderMoon
{
	uint2 direction;        // Packed half3 toward moon (normalized)
	uint2 color;            // Packed half3 linear RGB

	float size;             // Angular radius (radians)
	float halo_size;        // Extra halo radius (radians)
	float halo_sharpness;   // Halo falloff exponent
	float halo_intensity;   // Halo brightness multiplier

	int texture;            // Bindless SRV index, -1 if unused
	float texture_mip_bias; // Mip bias for moon texture sampling
	float light_intensity;  // Moon directional light illuminance
	float eclipse_strength; // 0-1 earth shadow on moon (eclipse)

	uint light_index;       // Moon directional light entity index
	uint padding0;
	uint padding1;
	uint padding2;
};
#ifdef __cplusplus
static_assert(sizeof(ShaderMoon) == 64);
#endif // __cplusplus

#endif // WI_SHADERINTEROP_MOON_H
