#ifndef WI_SAMPLER_MAPPING_H
#define WI_SAMPLER_MAPPING_H

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
#define SSLOT_CMP_DEPTH			13
#define SSLOT_OBJECTSHADER		14
#define SSLOT_RESERVED			15
#define SSLOT_COUNT_PERSISTENT	(SSLOT_RESERVED + 1 - SSLOT_COUNT_ONDEMAND)

// On demand samplers:
// These are bound on demand and alive until another is bound at the same slot
#define SSLOT_ONDEMAND0			0
#define SSLOT_ONDEMAND1			1
#define SSLOT_ONDEMAND2			2
#define SSLOT_ONDEMAND3			3
#define SSLOT_COUNT_ONDEMAND	(SSLOT_ONDEMAND3 + 1)

#define SSLOT_COUNT				(SSLOT_COUNT_PERSISTENT + SSLOT_COUNT_ONDEMAND)

#endif // WI_SAMPLER_MAPPING_H
