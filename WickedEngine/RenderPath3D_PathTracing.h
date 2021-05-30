#pragma once
#include "RenderPath3D.h"


class RenderPath3D_PathTracing :
	public RenderPath3D
{
protected:
	int sam = -1;
	int target = 1024;
	wiGraphics::Texture traceResult;

	std::vector<uint8_t> texturedata_src;
	std::vector<uint8_t> texturedata_dst;
	std::vector<uint8_t> texturedata_albedo;
	std::vector<uint8_t> texturedata_normal;
	wiGraphics::Texture denoiserAlbedo;
	wiGraphics::Texture denoiserNormal;
	wiGraphics::Texture denoiserResult;
	wiJobSystem::context denoiserContext;

	wiGraphics::RenderPass renderpass_debugbvh;

	void ResizeBuffers() override;

public:
	const wiGraphics::Texture* GetDepthStencil() const override { return nullptr; };

	void Update(float dt) override;
	void Render() const override;
	void Compose(wiGraphics::CommandList cmd) const override;

	int getCurrentSampleCount() const { return sam; }
	void setTargetSampleCount(int value) { target = value; }
	float getProgress() const { return (float)sam / (float)target; }

	float denoiserProgress = 0;
	float getDenoiserProgress() const { return denoiserProgress; }
	bool isDenoiserAvailable() const;

	void resetProgress() { sam = -1; denoiserProgress = 0; }
};
