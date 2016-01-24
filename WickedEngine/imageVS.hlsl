#include "globals.hlsli"
#include "imageHF.hlsli"


VertextoPixel main(uint vI : SV_VERTEXID)
{
	float2 inTex = float2(vI%2,vI%4/2);

	VertextoPixel Out = (VertextoPixel)0;

	float4 inPos=float4(0,0,0,1);

	if (xMirror<0)
		inTex.x = 1 - inTex.x;

	inPos.x -= xDimensions.x*0.5f;
	inPos.y += xDimensions.y*0.5f;
	if(inTex.x==1)		    inPos.x+=xDimensions.z*xMirror;
	if(inTex.y==1)		    inPos.y-=xDimensions.w;
	
	Out.pos = Out.dis = mul(mul(inPos,xTrans), xViewProjection); 
	
	Out.tex.xy = inTex;

	////if(xDrawRec.x==0 && xDrawRec.y==0 && xDrawRec.z==0 && xDrawRec.w==0){
	////	if(!xPivot){//upperleft pivot
	////		inPos.x-=xDimensions.x*0.5f;
	////		inPos.y+=xDimensions.y*0.5f;
	////	}
	////	else if(xPivot==1){//center pivot
	////		inPos.x-=xDimensions.z*0.5f;
	////		inPos.y+=xDimensions.w*0.5f;
	////	}
	////	if(inTex.x==1)		    inPos.x+=xDimensions.z*xMirror;
	////	if(inTex.y==1)		    inPos.y-=xDimensions.w;
	////}
	////else{
	////	if(!xPivot){
	////		inPos.x-=xDimensions.x*0.5f;
	////		inPos.y+=xDimensions.y*0.5f;
	////	}
	////	else if(xPivot==1){
	////		inPos.x-=xDrawRec.z*0.5f;
	////		inPos.y+=xDrawRec.w*0.5f;
	////	}
	////	if(inTex.x==1)		    inPos.x+=xDrawRec.z*xMirror;
	////	if(inTex.y==1)		    inPos.y-=xDrawRec.w;

	////	inTex*=float2(xDrawRec.z/xDimensions.z,xDrawRec.w/xDimensions.w);
	////	inTex+=float2(xDrawRec.x/xDimensions.z,xDrawRec.y/xDimensions.w);
	////}

	//inPos.xy+=float2(xOffsetX,xOffsetY);
	//Out.pos = Out.dis = mul( mul(inPos,xTrans), xViewProjection);
	
	//Out.tex.xy=inTex;

	return Out;
}

