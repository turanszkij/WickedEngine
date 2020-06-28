#ifndef WI_GRAPHICSDEVICE_SHAREDINTERNALS_H
#define WI_GRAPHICSDEVICE_SHAREDINTERNALS_H

// Descriptor layout counts per shader stage:
//	Rebuilding shaders and graphics devices are necessary after changing these values
#define GPU_RESOURCE_HEAP_CBV_COUNT		12
#define GPU_RESOURCE_HEAP_SRV_COUNT		48
#define GPU_RESOURCE_HEAP_UAV_COUNT		8
#define GPU_SAMPLER_HEAP_COUNT			16

#endif // WI_GRAPHICSDEVICE_SHAREDINTERNALS_H
