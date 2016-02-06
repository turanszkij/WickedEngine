#ifndef _TEXTUREMAPPING_H_
#define _TEXTUREMAPPING_H_

// Persistent

// On demand


///////////////////////////
// Helpers:
///////////////////////////

// Shader:
//////////

// Automatically binds samplers on the shader side:
// Needs macro expansion
#define TEXTURE2D_X(name, type, slot) Texture2D< type > name : register(t ## slot);
#define TEXTURE2D(name, type, slot) TEXTURE2D_X(name, type, slot)

#endif // _TEXTUREMAPPING_H_
