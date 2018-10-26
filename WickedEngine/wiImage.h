#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#include "ShaderInterop.h"
#include "wiImageEffects.h"

namespace wiImage
{
	void LoadShaders();
	void BindPersistentState(GRAPHICSTHREAD threadID);
	
	void Draw(wiGraphicsTypes::Texture2D* texture, const wiImageEffects& effects,GRAPHICSTHREAD threadID);

	void DrawDeferred(wiGraphicsTypes::Texture2D* lightmap_diffuse, wiGraphicsTypes::Texture2D* lightmap_specular, 
		wiGraphicsTypes::Texture2D* ao, GRAPHICSTHREAD threadID, int stencilref = 0);

	void Initialize();
	void CleanUp();
};

