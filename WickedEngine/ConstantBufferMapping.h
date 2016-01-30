#ifndef _CONSTANTBUFFER_MAPPING_H_
#define _CONSTANTBUFFER_MAPPING_H_

// Slot matchings:
////////////////////////////////////////////////////

// Persistent buffers:
// These are bound once and are alive forever
#define CBSLOT_RENDERER_WORLD			0
#define CBSLOT_RENDERER_FRAME			1
#define CBSLOT_RENDERER_CAMERA			2
#define CBSLOT_RENDERER_MATERIAL		3
#define CBSLOT_RENDERER_DIRLIGHT		4
#define CBSLOT_RENDERER_MISC			5
#define CBSLOT_RENDERER_SHADOW			6

#define CBSLOT_IMAGE_IMAGE				7
#define CBSLOT_IMAGE_POSTPROCESS		8

#define CBSLOT_FONT_FONT				9

#define CBSLOT_API						10


// On demand buffers:
// These are bound on demand and alive until another is bound at the same slot
#define CBSLOT_RENDERER_CUBEMAPRENDER	11
#define CBSLOT_RENDERER_POINTLIGHT		12
#define CBSLOT_RENDERER_SPOTLIGHT		13
#define CBSLOT_RENDERER_VOLUMELIGHT		11
#define CBSLOT_RENDERER_DECAL			11
#define CBSLOT_RENDERER_BONEBUFFER		11

#define CBSLOT_OTHER_EMITTEDPARTICLE	11
#define CBSLOT_OTHER_HAIRPARTICLE		11
#define CBSLOT_OTHER_LENSFLARE			11




///////////////////////////
// Helpers:
///////////////////////////

// CPP:
/////////

#define CONSTANTBUFFER_BINDSLOT __ConstantBuffer_BindSlot__
// Add this to a struct to match that with a bind slot:
#define CB_SETBINDSLOT(slot) static const int CONSTANTBUFFER_BINDSLOT = (slot);
// Get bindslot from a struct which is matched with a bind slot:
#define CB_GETBINDSLOT(structname) structname::CONSTANTBUFFER_BINDSLOT



// Shader:
//////////

// Automatically binds constantbuffers on the shader side:
// Needs macro expansion
#define CBUFFER_X(name, slot) cbuffer name : register(b ## slot)
#define CBUFFER(name, slot) CBUFFER_X(name, slot)


#endif // _CONSTANTBUFFER_MAPPING_H_
