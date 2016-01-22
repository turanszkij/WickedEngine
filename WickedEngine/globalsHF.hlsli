// TODO: REMOVE THIS FILE

#ifndef SHADER_GLOBALS
#define SHADER_GLOBALS

#define RT_UNSHADED			0.01
#define RT_TOON				0.02
#define RT_EPSILON			0.001

bool isUnshaded(float val){return abs(val-RT_UNSHADED)<RT_EPSILON;}
bool isToon(float val){return abs(val-RT_TOON)<RT_EPSILON;}


static const float inf = 100000;

#endif