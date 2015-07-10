#pragma once
#include "CommonInclude.h"

enum BLENDMODE{
	BLENDMODE_OPAQUE,
	BLENDMODE_ALPHA,
	BLENDMODE_ADDITIVE,
	BLENDMODE_MAX,
};
enum SAMPLEMODE{
	SAMPLEMODE_CLAMP,
	SAMPLEMODE_WRAP,
	SAMPLEMODE_MIRROR,
};
enum QUALITY{
	QUALITY_NEAREST,
	QUALITY_BILINEAR,
	QUALITY_ANISOTROPIC,
};
enum ImageType{
	SCREEN,
	WORLD,
};
enum Pivot{
	UPPERLEFT,
	CENTER,
};

class wiImageEffects{
public:
	XMFLOAT3 pos;
	XMFLOAT2 siz;
	XMFLOAT2 scale;
	XMFLOAT4 drawRec;
	XMFLOAT2 texOffset;
	XMFLOAT2 offset;
	XMFLOAT2 sunPos;
	XMFLOAT4 lookAt; //.w is for enabled
	float mirror;
	float blur,blurDir;
	float fade;
	float opacity;
	float rotation;
	bool extractNormalMap;
	float mipLevel;
		
	BLENDMODE blendFlag;
	ImageType typeFlag;
	SAMPLEMODE sampleFlag;
	QUALITY quality;
	Pivot pivotFlag;

	struct Bloom{
		bool  separate;
		float threshold;
		float saturation;

		void clear(){separate=false;threshold=saturation=0;}
		Bloom(){clear();}
	};
	Bloom bloom;
	struct Processing{
		bool active;
		bool motionBlur;
		bool outline;
		float dofStrength;
		bool fxaa;
		bool ssao;
		XMFLOAT2 ssss;
		bool ssr;
		bool linDepth;
		bool colorGrade;
			
		void clear(){active=motionBlur=outline=fxaa=ssao=linDepth=colorGrade=ssr=false;dofStrength=0;ssss=XMFLOAT2(0,0);}
		void setDOF(float value){dofStrength=value;active=value;}
		void setMotionBlur(bool value){motionBlur=value;active=value;}
		void setOutline(bool value){outline=value;active=value;}
		void setFXAA(bool value){fxaa=value;active=value;}
		void setSSAO(bool value){ssao=value;active=value;}
		void setLinDepth(bool value){linDepth=value;active=value;}
		void setColorGrade(bool value){colorGrade=value;active=value;}
		//direction*Properties
		void setSSSS(const XMFLOAT2& value){ssss=value;active=value.x||value.y;}
		void setSSR(bool value){ ssr = value; active = value; }
		Processing(){clear();}
	};
	Processing process;
	bool deferred;
	ID3D11ShaderResourceView *normalMap,*depthMap,*velocityMap,*refractionMap,*maskMap;
	void setMaskMap(ID3D11ShaderResourceView*view){maskMap=view;}
	void setRefractionMap(ID3D11ShaderResourceView*view){refractionMap=view;}
	void setVelocityMap(ID3D11ShaderResourceView*view){velocityMap=view;}
	void setDepthMap(ID3D11ShaderResourceView*view){depthMap=view;}
	void setNormalMap(ID3D11ShaderResourceView*view){normalMap=view;}

	void init(){
		pos=XMFLOAT3(0,0,0);
		siz=XMFLOAT2(1,1);
		scale=XMFLOAT2(1,1);
		drawRec=XMFLOAT4(0,0,0,0);
		texOffset=XMFLOAT2(0,0);
		offset=XMFLOAT2(0,0);
		sunPos=XMFLOAT2(0,0);
		lookAt=XMFLOAT4(0,0,0,0);
		mirror=1.0f;
		blur=blurDir=fade=opacity=rotation=0.0f;
		extractNormalMap=false;
		mipLevel=0.f;
		blendFlag=BLENDMODE_ALPHA;
		typeFlag=SCREEN;
		sampleFlag=SAMPLEMODE_MIRROR;
		quality=QUALITY_BILINEAR;
		pivotFlag=UPPERLEFT;
		bloom=Bloom();
		process=Processing();
		deferred=false;
		normalMap=nullptr;
		depthMap=nullptr;
		velocityMap=nullptr;
		refractionMap=nullptr;
		maskMap=nullptr;
	}


	wiImageEffects(){
		init();
	}
	wiImageEffects(float width, float height){
		init();
		siz=XMFLOAT2(width,height);
	}
	wiImageEffects(float posX, float posY, float width, float height){
		init();
		pos.x=posX;
		pos.y=posY;
		siz=XMFLOAT2(width,height);
	}
};

