#include "globals.hlsli"
#include "imageHF.hlsli"


VertextoPixel main(uint vI : SV_VERTEXID)
{
	float2 inTex = float2(vI%2,vI%4/2);

	VertextoPixel Out = (VertextoPixel)0;

	float3 inPos=float3(0,0,0);

	if(xOffsetMirFade.z<0)
		inTex.x=1-inTex.x;
		

	if(xDrawRec.x==0 && xDrawRec.y==0 && xDrawRec.z==0 && xDrawRec.w==0){
		if(!xBlurOpaPiv.w){//upperleft pivot
			inPos.x-=xDimensions.x*0.5f;
			inPos.y+=xDimensions.y*0.5f;
		}
		else if(xBlurOpaPiv.w==1){//center pivot
			inPos.x-=xDimensions.z*0.5f;
			inPos.y+=xDimensions.w*0.5f;
		}
		if(inTex.x==1)		    inPos.x+=xDimensions.z*xOffsetMirFade.z;
		if(inTex.y==1)		    inPos.y-=xDimensions.w;
	}
	else{
		if(!xBlurOpaPiv.w){
			inPos.x-=xDimensions.x*0.5f;
			inPos.y+=xDimensions.y*0.5f;
		}
		else if(xBlurOpaPiv.w==1){
			inPos.x-=xDrawRec.z*0.5f;
			inPos.y+=xDrawRec.w*0.5f;
		}
		if(inTex.x==1)		    inPos.x+=xDrawRec.z*xOffsetMirFade.z;
		if(inTex.y==1)		    inPos.y-=xDrawRec.w;

		inTex*=float2(xDrawRec.z/xDimensions.z,xDrawRec.w/xDimensions.w);
		inTex+=float2(xDrawRec.x/xDimensions.z,xDrawRec.y/xDimensions.w);
	}
	inPos+=float3(xOffsetMirFade.x,xOffsetMirFade.y,0);
	Out.pos = Out.dis = mul( mul(float4(inPos,1),xTrans), xViewProjection);
	
	Out.tex.xy=inTex;
	Out.tex.zw=xTexOffset.xy;
	Out.mip = xTexOffset.z;

	return Out;
}

//VertextoPixel main(uint vI : SV_VERTEXID)
//{
//	float2 inTex = float2(vI%2,vI%4/2);
//
//	VertextoPixel Out = (VertextoPixel)0;
//
//	float3 inPos=float3(0,0,0);
//		
//
//	if(xDrawRec.x==0 && xDrawRec.y==0 && xDrawRec.z==0 && xDrawRec.w==0){
//		if(!xBlurOpaPiv.w){
//			inPos.x-=xDimensions.x*0.5f;
//			inPos.y+=xDimensions.y*0.5f;
//		}
//		else if(xBlurOpaPiv.w==1){
//			inPos.x-=xDimensions.z*0.5f;
//			inPos.y+=xDimensions.w*0.5f;
//		}
//		if(inTex.x==1)		    inPos.x+=xDimensions.z*xOffsetMirFade.z;
//		if(inTex.y==1)		    inPos.y-=xDimensions.w;
//	}
//	else if(xDrawRec.x!=0 || xDrawRec.y!=0 || xDrawRec.z!=0 || xDrawRec.w!=0){
//		if(!xBlurOpaPiv.w){
//			inPos.x-=xDimensions.x*0.5f;
//			inPos.y+=xDimensions.y*0.5f;
//		}
//		else if(xBlurOpaPiv.w==1){
//			inPos.x-=xDrawRec.z*0.5f;
//			inPos.y+=xDrawRec.w*0.5f;
//		}
//		if(inTex.x==1)		    inPos.x+=xDrawRec.z*xOffsetMirFade.z;
//		if(inTex.y==1)		    inPos.y-=xDrawRec.w;
//
//		inTex*=float2(xDrawRec.z/xDimensions.z,xDrawRec.w/xDimensions.w);
//		inTex+=float2(xDrawRec.x/xDimensions.z,xDrawRec.y/xDimensions.w);
//	}
//	inPos+=float3(xOffsetMirFade.x,xOffsetMirFade.y,0);
//	Out.pos = Out.dis = mul( mul(float4(inPos,1),xTrans),g_xCamera_VP );
//	
//	Out.tex.xy=inTex;
//	Out.tex.zw=xTexOffset.xy;
//
//	return Out;
//}
