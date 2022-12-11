#ifndef WI_SHADERINTEROP_FONT_H
#define WI_SHADERINTEROP_FONT_H
// This file contains settings to apply to FSR2 at compilation time

#ifdef __cplusplus
#define FFX_CPU
#else
#define FFX_GPU
#define FFX_HLSL
#endif // __cplusplus

#ifndef FFX_HALF
#define FFX_HALF 0 // FP16 doesn't have perf benefit for me
#endif // FFX_HALF

#define FFX_FSR2_OPTION_INVERTED_DEPTH 1
#define FFX_FSR2_OPTION_HDR_COLOR_INPUT 1
#define FFX_FSR2_OPTION_APPLY_SHARPENING 1
#define FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS 1
#define FFX_FSR2_OPTION_JITTERED_MOTION_VECTORS 0
#define FFX_FSR2_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF 1
#define FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE 1
#define FFX_FSR2_OPTION_GUARANTEE_UPSAMPLE_WEIGHT_ON_NEW_SAMPLES 1
#define FFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF 1
#define FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF 0 // missing header errors when enabled. They can be also deleted and it will compile, but didn't seem to make a difference

#endif // WI_SHADERINTEROP_FONT_H
