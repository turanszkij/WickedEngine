#ifndef _SAMPLER_MAPPING_H_
#define _SAMPLER_MAPPING_H_

// Slot matchings:
////////////////////////////////////////////////////

// Persistent samplers
// These are bound once and are alive forever
#define SSLOT_LINEAR_CLAMP		4
#define SSLOT_LINEAR_WRAP		5
#define SSLOT_LINEAR_MIRROR		6
#define SSLOT_POINT_CLAMP		7
#define SSLOT_POINT_WRAP		8
#define SSLOT_POINT_MIRROR		9
#define SSLOT_ANISO_CLAMP		10
#define SSLOT_ANISO_WRAP		11
#define SSLOT_ANISO_MIRROR		12
#define SSLOT_SHADOWCOMP		13
#define SSLOT_RESERVED0			14
#define SSLOT_RESERVED1			15

// On demand samplers:
// These are bound on demand and alive until another is bound at the same slot
#define SSLOT_ONDEMAND0			0
#define SSLOT_ONDEMAND1			1
#define SSLOT_ONDEMAND2			2
#define SSLOT_ONDEMAND3			3

///////////////////////////
// Helpers:
///////////////////////////

// Shader:
//////////

// Automatically binds samplers on the shader side:
// Needs macro expansion
#define SAMPLERSTATE_X(name, slot) SamplerState name : register(s ## slot);
#define SAMPLERSTATE(name, slot) SAMPLERSTATE_X(name, slot)
#define SAMPLERCOMPARISONSTATE_X(name, slot) SamplerComparisonState name : register(s ## slot);
#define SAMPLERCOMPARISONSTATE(name, slot) SAMPLERCOMPARISONSTATE_X(name, slot)

#endif // _SAMPLER_MAPPING_H_
