#pragma once
#include "wiRenderPath3D.h"
#include "wiVector.h"

namespace wi
{

	class RenderPath3D_PathTracing :
		public RenderPath3D
	{
	protected:
		int sam = -1;
		int target = 1024;
		wi::graphics::Texture traceResult;
		wi::graphics::Texture traceDepth;
		wi::graphics::Texture traceStencil;

		wi::vector<uint8_t> texturedata_src;
		wi::vector<uint8_t> texturedata_dst;
		wi::vector<uint8_t> texturedata_albedo;
		wi::vector<uint8_t> texturedata_normal;
		wi::graphics::Texture denoiserAlbedo;
		wi::graphics::Texture denoiserNormal;
		wi::graphics::Texture denoiserResult;
		wi::jobsystem::context denoiserContext;

		void ResizeBuffers() override;

	public:

		void Update(float dt) override;
		void Render() const override;
		void Compose(wi::graphics::CommandList cmd) const override;

		int getCurrentSampleCount() const { return sam; }
		void setTargetSampleCount(int value) { target = value; }
		float getProgress() const { return (float)sam / (float)target; }

		float denoiserProgress = 0;
		float getDenoiserProgress() const { return denoiserProgress; }
		bool isDenoiserAvailable() const;

		void resetProgress() { sam = -1; denoiserProgress = 0; volumetriccloudResources.frame = 0; }

		uint8_t instanceInclusionMask_PathTrace = 0xFF;
	};

}
