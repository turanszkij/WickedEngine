#ifndef VIEWPROPERTIES
#define VIEWPROPERTIES

//#define zNearP 0.1f
//#define zFarP 800.0f

cbuffer viewPropCB:register(b10)
{
	float zNearP;
	float zFarP;
	float paddingViewProp[2];
}

#endif