#ifndef TOON
#define TOON

#define TOON_SHADING


#ifdef TOON_SHADING
#define LEVEL1 0.35
//#define LEVEL2 0.3
//#define LEVEL3 0.2
#endif

inline void toon(inout float value){
//#ifdef TOON_SHADING
//	if (value > LEVEL1)
//		value = 1;
//#ifdef LEVEL2
//	else if (value > LEVEL2)
//		value *= 0.6;
//#endif //LEVEL2
//#ifdef LEVEL3
//	else if (value > LEVEL3)
//		value *= 0.3;
//#endif //LEVEL3
//	else
//		value = 0;
//#endif
	value=step(0.1,value);
}

#endif
