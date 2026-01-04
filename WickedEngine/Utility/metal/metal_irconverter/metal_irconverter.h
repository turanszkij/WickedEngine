//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// Copyright 2023-2025 Apple Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __APPLE__
#import <dispatch/dispatch.h>
#endif // __APPLE__

#ifdef _MSC_VER
#define IR_DEPRECATED(message) __declspec(deprecated(message))
#else
#define IR_DEPRECATED(message) __attribute__((deprecated(message)))
#endif // _MSC_VER

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#include "ir_comparison_function.h"
#include "ir_descriptor_range_flags.h"
#include "ir_filter.h"
#include "ir_format.h"
#include "ir_input_classification.h"
#include "ir_input_primitive.h"
#include "ir_input_topology.h"
#include "ir_tessellator_output_primitive.h"
#include "ir_root_descriptor_flags.h"
#include "ir_root_signature_flags.h"
#include "ir_shader_visibility.h"
#include "ir_tessellator_domain.h"
#include "ir_tessellator_partitioning.h"
#include "ir_texture_address_mode.h"
#include "ir_version.h"

struct IRCompiler;
struct IRObject;
struct IRRootSignature;
struct IRMetalLibBinary;
struct IRShaderReflection;
struct IRError;
struct IRRayTracingPipelineConfiguration;

typedef struct IRCompiler IRCompiler;
typedef struct IRObject IRObject;
typedef struct IRRootSignature IRRootSignature;
typedef struct IRMetalLibBinary IRMetalLibBinary;
typedef struct IRShaderReflection IRShaderReflection;
typedef struct IRError IRError;
typedef struct IRRayTracingPipelineConfiguration IRRayTracingPipelineConfiguration;

typedef enum IRShaderStage
{
    IRShaderStageInvalid,
    IRShaderStageVertex,
    IRShaderStageFragment,
    IRShaderStageHull,
    IRShaderStageDomain,
    IRShaderStageMesh,
    IRShaderStageAmplification,
    IRShaderStageGeometry,
    IRShaderStageCompute,
    IRShaderStageClosestHit,
    IRShaderStageIntersection,
    IRShaderStageAnyHit,
    IRShaderStageMiss,
    IRShaderStageRayGeneration,
    IRShaderStageCallable,
    IRShaderStageStreamOut,
    IRShaderStageStageIn,
} IRShaderStage;

typedef enum IRObjectType
{
    IRObjectTypeDXILBytecode,
    IRObjectTypeMetalIRObject,
} IRObjectType;

typedef enum IRResourceType
{
    IRResourceTypeTable,
    IRResourceTypeConstant,
    IRResourceTypeCBV,
    IRResourceTypeSRV,
    IRResourceTypeUAV,
    IRResourceTypeSampler,
    IRResourceTypeInvalid
} IRResourceType;

#define IRDescriptorRangeOffsetAppend 0xFFFFFFFF

typedef enum IRDescriptorRangeType
{
    IRDescriptorRangeTypeSRV     = 0,
    IRDescriptorRangeTypeUAV     = (IRDescriptorRangeTypeSRV + 1),
    IRDescriptorRangeTypeCBV     = (IRDescriptorRangeTypeUAV + 1),
    IRDescriptorRangeTypeSampler = (IRDescriptorRangeTypeCBV + 1)
} IRDescriptorRangeType;

typedef struct IRDescriptorRange
{
    IRDescriptorRangeType RangeType;
    uint32_t NumDescriptors;
    uint32_t BaseShaderRegister;
    uint32_t RegisterSpace;
    uint32_t OffsetInDescriptorsFromTableStart;
} IRDescriptorRange;

typedef struct IRRootDescriptorTable
{
    uint32_t NumDescriptorRanges;
    const IRDescriptorRange* pDescriptorRanges;
} IRRootDescriptorTable;

typedef struct IRVertexInputTable
{
    uint32_t NumDescriptorRanges;
    const IRDescriptorRange* pDescriptorRanges;
} IRVertexInputTable;

typedef struct IRRootConstants
{
    uint32_t ShaderRegister;
    uint32_t RegisterSpace;
    uint32_t Num32BitValues;
} IRRootConstants;

typedef struct IRRootDescriptor
{
    uint32_t ShaderRegister;
    uint32_t RegisterSpace;
} IRRootDescriptor;


typedef enum IRStripCutIndex
{
    IRStripCutIndexDisabled = 0,
    IRStripCutIndex0xFFFF = 1,
    IRStripCutIndex0xFFFFFFFF = 2,
} IRStripCutIndex;

typedef enum IRRootParameterType
{
    IRRootParameterTypeDescriptorTable = 0,
    IRRootParameterType32BitConstants  = (IRRootParameterTypeDescriptorTable + 1),
    IRRootParameterTypeCBV             = (IRRootParameterType32BitConstants + 1),
    IRRootParameterTypeSRV             = (IRRootParameterTypeCBV + 1),
    IRRootParameterTypeUAV             = (IRRootParameterTypeSRV + 1)
} IRRootParameterType;

typedef struct IRRootParameter
{
    IRRootParameterType ParameterType;
    union
    {
        IRRootDescriptorTable DescriptorTable;
        IRRootConstants Constants;
        IRRootDescriptor Descriptor;
    };
    IRShaderVisibility ShaderVisibility;
} IRRootParameter;

typedef enum IRStaticBorderColor
{
    IRStaticBorderColorTransparentBlack = 0,
    IRStaticBorderColorOpaqueBlack      = (IRStaticBorderColorTransparentBlack + 1),
    IRStaticBorderColorOpaqueWhite      = (IRStaticBorderColorOpaqueBlack + 1)
} IRStaticBorderColor;

typedef enum IRCompatibilityFlags
{
    IRCompatibilityFlagNone                              = 0,
    IRCompatibilityFlagBoundsCheck                       = (1 << 0),
    IRCompatibilityFlagVertexPositionInfToNan            = (1 << 1),
    IRCompatibilityFlagTextureMinLODClamp                = (1 << 2),
    IRCompatibilityFlagSamplerLODBias                    = (1 << 3),
    IRCompatibilityFlagPositionInvariance                = (1 << 4),
    IRCompatibilityFlagSampleNanToZero                   = (1 << 5),
    IRCompatibilityFlagTexWriteRoundingRTZ               = (1 << 6),
    IRCompatibilityFlagSuppress2DComputeDerivativeErrors = (1 << 7),
    IRCompatibilityFlagForceTextureArray                 = (1 << 8),
} IRCompatibilityFlags;

typedef struct IRStaticSamplerDescriptor
{
    IRFilter             Filter;
    IRTextureAddressMode AddressU;
    IRTextureAddressMode AddressV;
    IRTextureAddressMode AddressW;
    float                MipLODBias;
    uint32_t             MaxAnisotropy;
    IRComparisonFunction ComparisonFunc;
    IRStaticBorderColor  BorderColor;
    float                MinLOD;
    float                MaxLOD;
    uint32_t             ShaderRegister;
    uint32_t             RegisterSpace;
    IRShaderVisibility   ShaderVisibility;
} IRStaticSamplerDescriptor;

typedef enum IRRootSignatureVersion
{
    IRRootSignatureVersion_1   = 0x1,
    IRRootSignatureVersion_1_0 = 0x1,
    IRRootSignatureVersion_1_1 = 0x2
} IRRootSignatureVersion;

typedef struct IRRootSignatureDescriptor
{
    uint32_t NumParameters;
    const IRRootParameter* pParameters;
    uint32_t NumStaticSamplers;
    const IRStaticSamplerDescriptor* pStaticSamplers;
    IRRootSignatureFlags Flags;
} IRRootSignatureDescriptor;

typedef struct IRDescriptorRange1
{
    IRDescriptorRangeType RangeType;
    uint32_t NumDescriptors;
    uint32_t BaseShaderRegister;
    uint32_t RegisterSpace;
    IRDescriptorRangeFlags Flags;
    uint32_t OffsetInDescriptorsFromTableStart;
} IRDescriptorRange1;

typedef struct IRRootDescriptorTable1
{
    uint32_t NumDescriptorRanges;
    IRDescriptorRange1* pDescriptorRanges;
} IRRootDescriptorTable1;

typedef struct IRRootDescriptor1
{
    uint32_t ShaderRegister;
    uint32_t RegisterSpace;
    IRRootDescriptorFlags Flags;
} IRRootDescriptor1;

typedef struct IRRootParameter1
{
    IRRootParameterType ParameterType;
    union
    {
        IRRootDescriptorTable1 DescriptorTable;
        IRRootConstants Constants;
        IRRootDescriptor1 Descriptor;
    };
    IRShaderVisibility ShaderVisibility;
} IRRootParameter1;

typedef struct IRRootSignatureDescriptor1
{
    uint32_t NumParameters;
    IRRootParameter1* pParameters;
    uint32_t NumStaticSamplers;
    IRStaticSamplerDescriptor* pStaticSamplers;
    IRRootSignatureFlags Flags;
} IRRootSignatureDescriptor1;

typedef struct IRVersionedRootSignatureDescriptor
{
    IRRootSignatureVersion version;
    union
    {
        IRRootSignatureDescriptor desc_1_0;
        IRRootSignatureDescriptor1 desc_1_1;
    };
} IRVersionedRootSignatureDescriptor;

typedef struct IRInputElementDescriptor1
{
    uint32_t                    semanticIndex;
    IRFormat                    format;
    uint32_t                    inputSlot;
    uint32_t                    alignedByteOffset;
    uint32_t                    instanceDataStepRate;
    IRInputClassification       inputSlotClass;
} IRInputElementDescriptor1;


typedef struct IRInputLayoutDescriptor1
{
    const char*                    semanticNames[31];
    IRInputElementDescriptor1 inputElementDescs[31];
    uint32_t                       numElements;
} IRInputLayoutDescriptor1;

typedef enum IRInputLayoutDescriptorVersion
{
    IRInputLayoutDescriptorVersion_1 = 0x1
} IRInputLayoutDescriptorVersion;

typedef struct IRVersionedInputLayoutDescriptor
{
    IRInputLayoutDescriptorVersion version;
    union
    {
        IRInputLayoutDescriptor1 desc_1_0;
    };
} IRVersionedInputLayoutDescriptor;

// From d3d12.h
typedef enum IRHitGroupType
{
    IRHitGroupTypeTriangles           = 0,
    IRHitGroupTypeProceduralPrimitive = 1
} IRHitGroupType;

// From d3d12.h
typedef enum IRRaytracingPipelineFlags
{
    IRRaytracingPipelineFlagNone                     = 0,
    IRRaytracingPipelineFlagSkipTriangles            = 0x100,
    IRRaytracingPipelineFlagSkipProceduralPrimitives = 0x200
} IRRaytracingPipelineFlags;

typedef enum IRRayGenerationCompilationMode
{
    IRRayGenerationCompilationKernel,
    IRRayGenerationCompilationVisibleFunction
} IRRayGenerationCompilationMode;

typedef enum IRIntersectionFunctionCompilationMode
{
    IRIntersectionFunctionCompilationVisibleFunction,
    IRIntersectionFunctionCompilationIntersectionFunction,
    IRIntersectionFunctionCompilationIntersectionFunctionBufferFunction,
} IRIntersectionFunctionCompilationMode;

enum IRErrorCode
{
    IRErrorCodeNoError,
    IRErrorCodeShaderRequiresRootSignature,
    IRErrorCodeUnrecognizedRootSignatureDescriptor,
    IRErrorCodeUnrecognizedParameterTypeInRootSignature,
    IRErrorCodeResourceNotReferencedByRootSignature,
    IRErrorCodeShaderIncompatibleWithDualSourceBlending,
    IRErrorCodeUnsupportedWaveSize,
    IRErrorCodeUnsupportedInstruction,
    IRErrorCodeCompilationError,
    IRErrorCodeFailedToSynthesizeStageInFunction,
    IRErrorCodeFailedToSynthesizeStreamOutFunction,
    IRErrorCodeFailedToSynthesizeIndirectIntersectionFunction,
    IRErrorCodeUnableToVerifyModule,
    IRErrorCodeUnableToLinkModule,
    IRErrorCodeUnrecognizedDXILHeader,
    IRErrorCodeInvalidRaytracingAttribute,
    IRErrorCodeNullHullShaderInputOutputMismatch,
    IRErrorCodeInvalidRaytracingUserAttributeSize,
    IRErrorCodeIncorrectHitgroupType,
    IRErrorCodeFP64Usage,
    IRErrorCodeUnknown
};

/**
 * Obtain the error code of an error.
 * @param error error object to query.
 * @return error code.
 */
uint32_t IRErrorGetCode(const IRError* error);

/**
 * Obtain any payload associated with an error.
 * @param error error object to query.
 * @return error payload. You must cast this pointer to the appropriate error payload struct for the error code.
 */
const void* IRErrorGetPayload(const IRError* error);

/**
 * Release resources associated with an error object.
 * @param error error objects for which to release its associated resources.
 */
void IRErrorDestroy(IRError* error);

/**
 * Create a new root signature from its descriptor.
 * @param descriptor description of the root signature to create.
 * @param error on return, if the compiler generates any errors, this optional out parameter contains error information. If an error occurs and this parameter is non-NULL, you must free it by calling IRErrorDestroy.
 * @return a new root signature suitable for configuring the top-level argument buffer layout of the produced MetalIR, or NULL upon encountering an error. You must destroy this object by calling IRRootSignatureDestroy.
 */
IRRootSignature* IRRootSignatureCreateFromDescriptor(const IRVersionedRootSignatureDescriptor* descriptor, IRError** error);

/**
 * Destroy a root signature object.
 * @param sig root signature object to destroy.
 */
void IRRootSignatureDestroy(IRRootSignature* sig);

typedef enum IRBytecodeOwnership
{
    /** Do not take ownership. */
    IRBytecodeOwnershipNone,
    
    /** Copy the bytecode. */
    IRBytecodeOwnershipCopy
} IRBytecodeOwnership;

/**
 * Create a DXIL object from DXIL bytecode.
 * @param bytecode bytecode representing legal DXIL. When using IRBytecodeOwnershipNone, you must ensure this buffer is not freed while the returned object is in use.
 * @param size size of the bytecode in bytes.
 * @param bytecodeOwnership determine whether the IRObject shall copy the bytecode or just hold a weak reference.
 * @return a new DXIL object representing the DXIL shader. You must destroy this object by calling IRObjectDestroy.
 */
IRObject* IRObjectCreateFromDXIL(const uint8_t* bytecode, size_t size, IRBytecodeOwnership bytecodeOwnership);

/**
 * Destroy a IRObject.
 * @param object IRObject to destroy.
 */
void IRObjectDestroy(IRObject* object);

/**
 * Query a IRObject's type.
 * @param object IRObject to query.
 * @return IRObjectTypeDXILBytecode if the object represents DXIL bytecode, IRObjectTypeMetalIRObject if the object represents MetalIR bytecode.
 */
IRObjectType IRObjectGetType(const IRObject* object);

/**
 * Query the shader stage of this IRObject.
 * @param object IRObject to query.
 * @return the shader stage of this IRObject.
 */
IRShaderStage IRObjectGetMetalIRShaderStage(const IRObject* object);

typedef enum IRCompilerValidationFlags
{
    IRCompilerValidationFlagNone                      = 0,
    IRCompilerValidationFlagValidateRawRootResources  = 1,
    IRCompilerValidationFlagValidateAllResourcesBound = (1 << 1),
    IRCompilerValidationFlagValidateDXIL              = (1 << 2),
    IRCompilerValidationFlagAll                       = ~0
} IRCompilerValidationFlags;

/**
 * Set a compiler's compile-time validation flags.
 * @param compiler compiler for which to set the flags.
 * @param validationFlags validation flags denoting the checks the compiler performs.
 */
void IRCompilerSetValidationFlags(IRCompiler* compiler, IRCompilerValidationFlags validationFlags);

/**
 * Create a new compiler instance.
 * @return a new compiler instance. You must destroy this object by calling IRCompilerDestroy.
 */
IRCompiler* IRCompilerCreate(void);

/**
 * Destroy a compiler instance.
 * @param compiler compiler to destroy.
 */
void IRCompilerDestroy(IRCompiler* compiler);

/**
 * Allocate a new object and populate it with the results of compiling and linking IR bytecode.
 * @note Prior calls to `IRCompilerSetHitgroupType`, `IRCompilerSetRayTracingPipelineConfiguration`, `IRCompilerSetGlobalRootSignature`, `IRCompilerSetLocalRootSignature` influence the bytecode this function produces.
 * @note you need to call `IRCompilerSetRayTracingPipelineConfiguration` before compiling ray tracing shaders.
 * @param compiler compiler to use for the translation process.
 * @param entryPointName optional entry point name to compile when converting a library with multiple entry points.
 * @param input input IR object.
 * @param error on return, if the compiler generates any errors, this optional out parameter contains error information. If an error occurs and this parameter is non-NULL, you must free it by calling IRErrorDestroy.
 * @return an IR Object containing MetalIR compiled and linked from the input IR, or NULL if an error occurs. You must destroy this object by calling IRObjectDestroy.
 */
IRObject* IRCompilerAllocCompileAndLink(IRCompiler* compiler, const char* entryPointName, const IRObject* input, IRError** error);

/**
 * Allocate a new object and populate with the results of combining, compiling, and linking IR bytecode of an intersection and an any-hit function.
 * This function allows representing the interactions between intersection and any-hit shaders on Metal, where these functions are fused together.
 * @warning Ensure you call `IRCompilerSetHitgroupType` prior to this function to specify the intersection type.
 * @note One of parameters `intersectionFunctionBytecode` and `anyHitFunctionBytecode` needs to be non-NULL.
 * @note If both parameters are non-NULL, this function uses the `anyHitFunctionBytecode` the latter references.
 * @note Prior calls to `IRCompilerSetHitgroupType`, `IRCompilerSetRayTracingPipelineConfiguration`, `IRCompilerSetGlobalRootSignature`, `IRCompilerSetLocalRootSignature` influence the bytecode this function produces.
 * @note Call `IRCompilerSetRayTracingPipelineConfiguration` before compiling ray tracing shaders.
 * @param compiler compiler to use for the translation process.
 * @param intersectionFunctionEntryPointName optional entry point name corresponding to the intersection function.
 * @param intersectionFunctionBytecode optional input bytecode that provides the intersection function bytecode.
 * @param anyHitFunctionEntryPointName optional entry point name corresponding to the any-hit function.
 * @param anyHitFunctionBytecode optional input bytecode that provides the any-hit function
 * @param error on return, if the compiler generates any errors, this optional out parameter contains error information. If an error occurs and this parameter is non-NULL, you must free it by calling IRErrorDestroy.
 * @return an IR Object containing MetalIR compiled from the input IR, or NULL if an error occurs. You must destroy this object by calling IRObjectDestroy.
 */
IRObject* IRCompilerAllocCombineCompileAndLink(IRCompiler* compiler, const char* intersectionFunctionEntryPointName, const IRObject* intersectionFunctionBytecode, const char* anyHitFunctionEntryPointName, const IRObject* anyHitFunctionBytecode, IRError** error);

/**
 * Copy the metallib binary, containing MetalIR bytecode.
 * @param obj MetalIR object containing the bytecode from where to copy.
 * @param stage shader stage to copy.
 * @param lib metallib binary into which to copy the metallib.
 * @return true if the metallib binary contains bytecode for this shader stage, false otherwise.
 */
bool IRObjectGetMetalLibBinary(const IRObject* obj, IRShaderStage stage, IRMetalLibBinary* lib);

typedef enum IRStageInCodeGenerationMode
{
    IRStageInCodeGenerationModeUseMetalVertexFetch,
    IRStageInCodeGenerationModeUseSeparateStageInFunction
} IRStageInCodeGenerationMode;

/**
 * Configure whether the compiler should generate a Metal vertex fetch, or allow synthesizing a separate stage-in function.
 * Use Metal vertex fetch to specify a MTLVertexDescriptor to your pipelines at runtime. Request a separate stage-in function to link vertex fetch as a Metal linked function.
 * Using a separate stage-in function provides your shader with more flexibility to perform type conversions, however, it requires more work to set up.
 * @param compiler compiler to configure.
 * @param stageInCodeGenerationMode code generation mode for the stage-in function.
 */
void IRCompilerSetStageInGenerationMode(IRCompiler* compiler, IRStageInCodeGenerationMode stageInCodeGenerationMode);

/**
 * Synthesize a stage in function.
 * @param compiler compiler configuration to use.
 * @param vertexShaderReflection reflection object of a vertex stage containing the inputs used as a base for synthesis.
 * @param layout vertex input layout descriptor.
 * @param binary pointer to a binary into which to write the synthesized MetalIR.
 * @return true if the stageIn function could be successfully created, false otherwise.
 */
bool IRMetalLibSynthesizeStageInFunction(const IRCompiler* compiler, const IRShaderReflection* vertexShaderReflection, const IRVersionedInputLayoutDescriptor* layout, IRMetalLibBinary *binary);

/**
 * Copy reflection data stemming from the process of compiling DXIL to MetalIR.
 * @param obj MetalIR object containing the metadata to copy.
 * @param stage shader stage from where to obtain the reflection.
 * @param reflection reflection object into which to copy the reflection data.
 * @return true if the metallib binary contains bytecode for this shader stage, false otherwise.
 */
bool IRObjectGetReflection(const IRObject* obj, IRShaderStage stage, IRShaderReflection* reflection);


// Geometry and Tessellation emulation (these compiler options persist)

/**
 * Configure a compiler to emit a shader that consumes a top-level Argument Buffer layout matching a specific root signature.
 * @param compiler compiler to configure.
 * @param rootSignature root signature that defines the layout of the top-level Argument Buffer for any shaders you compile with this compiler.
 * Pass in NULL (the default) to have the compiler generate a linear resource layout instead. If you provide a non-NULL rootSignature, you must ensure it is not destroyed before the compiler.
 */
void IRCompilerSetGlobalRootSignature(IRCompiler* compiler, const IRRootSignature* rootSignature);

/**
 * Configure a compiler to produce a shader that consumes a local root signature matching a specific layout.
 * @param compiler compiler to configure.
 * @param rootSignature If you provide a non-NULL rootSignature, you must ensure it is not destroyed before the compiler.
 */
void IRCompilerSetLocalRootSignature(IRCompiler* compiler, const IRRootSignature* rootSignature);

/**
 * Configure the hit group type for all subsequent shaders a compiler compiles.
 * @param compiler compiler to configure
 * @param hitGroupType type of hitgroup the shader expects
 */
void IRCompilerSetHitgroupType(IRCompiler* compiler, IRHitGroupType hitGroupType);

#define IRIntrinsicMaskClosestHitAll         0x7FFFFFFF
#define IRIntrinsicMaskMissShaderAll         0x7FFF
#define IRIntrinsicMaskCallableShaderAll     0x703F
#define IRIntrinsicMaskAnyHitShaderAll       ((uint64_t)-1)
#define IRRayTracingUnlimitedRecursionDepth  (-1)

/**
 * Analyze an input IR and produce a mask representing all ray tracing intrinsics it references.
 * Use this function to gather all intrinsics for your closest hit, any hit, intersection, and callable shaders and produce optimal hit group arguments.
 */
uint64_t IRObjectGatherRaytracingIntrinsics(IRObject* input, const char* entryPoint);

/**
 * Configure a compiler with upfront information to generate an optimal interface between ray tracing functions.
 * Calling this function is optional, but when omitted, the compiler assumes a worst-case scenario, affecting runtime performance.
 * Use function `IRObjectGatherRaytracingIntrinsics` to collect the intrinsic usage mask for all closest hit, any hit, intersection, and callable shaders in the pipeline to build.
 * After calling this function, all subsequent shaders compiled need to conform to the masks provided, otherwise undefined behavior occurs.
 * Specifying a mask and then adding additional shaders to a pipeline that don't conform to it causes undefined behavior.
 * @param compiler compiler to configure
 * @param maxAttributeSizeInBytes the maximum number of ray tracing attributes (in bytes) that a pipeline consisting of these shaders uses.
 * @param raytracingPipelineFlags flags for the ray tracing pipeline your application builds from these shaders.
 * @param chs bitwise OR mask of all closest hit shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskClosestHitAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param miss bitwise OR mask of all miss shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskMissShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param anyHit bitwise OR mask of all any hit shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskAnyHitShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param callableArgs bitwise OR mask of all callable shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskCallableShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param maxRecursiveDepth stop point for recursion. Pass `IRRayTracingUnlimitedRecursionDepth` for no limit.
 * @param rayGenerationCompilationMode set the ray-generation shader compilation mode to compile either as a compute kernel, or as a visible function for a shader binding table.
 * @warning providing mask values other than the defaults or those returned by `IRObjectGatherRaytracingIntrinsics` may cause subsequent shader compilations to fail.
 */
IR_DEPRECATED("please use IRCompilerSetRayTracingPipelineConfiguration instead")
void IRCompilerSetRayTracingPipelineArguments(IRCompiler* compiler, uint32_t maxAttributeSizeInBytes, IRRaytracingPipelineFlags raytracingPipelineFlags, uint64_t chs, uint64_t miss, uint64_t anyHit, uint64_t callableArgs, int maxRecursiveDepth, IRRayGenerationCompilationMode rayGenerationCompilationMode, IRIntersectionFunctionCompilationMode intersectionFunctionCompilationMode);


/**
 * Create a new instance of a ray tracing pipeline configuration.
 * @return a newly-allocated IRRayTracingPipelineConfiguration instance.
 * @note call `IRRayTracingPipelineConfigurationDestroy` to release the instance's memory.
 */
IRRayTracingPipelineConfiguration* IRRayTracingPipelineConfigurationCreate(void);

/**
 * Destroy an instance of a ray tracing pipeline configuration.
 * @param configuration configuration object to destroy.
 * @note allocate the instance by calling `IRRayTracingPipelineConfigurationCreate`.
 */
void IRRayTracingPipelineConfigurationDestroy(IRRayTracingPipelineConfiguration* configuration);

/**
 * Set the maximum attribute size in bytes for a ray tracing pipeline configuration object.
 * @param configuration configuration object to modify.
 * @param maxAttributeSizeInBytes the maximum number of ray tracing attributes (in bytes) that a pipeline consisting of these shaders uses (defaults to 16).
 */
void IRRayTracingPipelineConfigurationSetMaxAttributeSizeInBytes(IRRayTracingPipelineConfiguration* configuration, uint32_t maxAttributeSizeInBytes);

/**
 * Set ray tracing pipeline flags for a ray tracing pipeline configuration object.
 * @param configuration configuration object to update.
 * @param raytracingPipelineFlags flags the compiler specifies for the subsequent compiled ray tracing pipelines (defaults to `IRRaytracingPipelineFlagNone`).
 */
void IRRayTracingPipelineConfigurationSetPipelineFlags(IRRayTracingPipelineConfiguration* configuration, IRRaytracingPipelineFlags raytracingPipelineFlags);

/**
 * Set the intrinsic masks to generate an optimized interface between ray tracing functions.
 * Calling this function is optional, but when omitted, the compiler assumes a worst-case scenario, affecting runtime performance.
 * After calling this function, all subsequent shaders compiled need to conform to the masks provided, otherwise undefined behavior occurs.
 * Use function `IRObjectGatherRaytracingIntrinsics` to collect the intrinsic usage mask for all closest hit, any hit, intersection, and callable shaders in the pipeline to build.
 * @param configuration configuration object to modify.
 * @param chs bitwise OR mask of all closest hit shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskClosestHitAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param miss bitwise OR mask of all miss shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskMissShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param anyHit bitwise OR mask of all any hit shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskAnyHitShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @param callableArgs bitwise OR mask of all callable shaders for a ray tracing pipeline your application builds using subsequent converted shaders (defaults to `IRIntrinsicMaskCallableShaderAll`). The value must match across all functions of all types used in this RT pipeline.
 * @warning providing mask values other than the defaults or those returned by `IRObjectGatherRaytracingIntrinsics` may cause subsequent shader compilations to fail.
 */
void IRRayTracingPipelineConfigurationSetIntrinsicMasks(IRRayTracingPipelineConfiguration* configuration, uint64_t chs, uint64_t miss, uint64_t anyHit, uint64_t callableArgs);

/**
 * Specify the stop point for recursion when compiling shaders for a ray tracing pipeline.
 * @param configuration configuration object to modify.
 * @param maxRecursiveDepth stop point for recursion. Pass `IRRayTracingUnlimitedRecursionDepth` for no limit (default).
 */
void IRRayTracingPipelineConfigurationSetMaxRecursiveDepth(IRRayTracingPipelineConfiguration* configuration, int maxRecursiveDepth);

/**
 * Set the compilation mode for ray generation functions.
 * @param configuration configuration object to modify.
 * @param rayGenMode compilation mode for ray generation functions.
 */
void IRRayTracingPipelineConfigurationSetRayGenerationCompilationMode(IRRayTracingPipelineConfiguration* configuration, IRRayGenerationCompilationMode rayGenMode);

/**
 * Set the compilation mode for ray tracing intersection functions.
 * @param configuration configuration object to modify.
 * @param intersectionFunctionMode compilation mode for intersection functions.
 */
void IRRayTracingPipelineConfigurationSetIntersectionFunctionCompilationMode(IRRayTracingPipelineConfiguration* configuration, IRIntersectionFunctionCompilationMode intersectionFunctionMode);

/**
 * Enable the compiler to annotate converted shaders with function group information, potentially accelerating ray tracing function calling.
 * @param configuration configuration object to modify.
 * @param enableIntersectionFunctionGroups set to true to enable emitting annotations, false to disable it.
 */
void IRRayTracingPipelineConfigurationEnableIntersectionFunctionGroups(IRRayTracingPipelineConfiguration* configuration, bool enableIntersectionFunctionGroups);

/**
 * Enable converted shaders to directly access ray intersection results from memory.
 * @param configuration configuration object to modify.
 * @param enableDirectStateAccess set to true to enable compiled shaders to directly fetch intersection result data from memory,
 * false to access data via parameter passing.
 * @note this feature requires the converted shader pipeline to run on macOS 15, iOS 18, or later.
 */
void IRRayTracingPipelineConfigurationEnableDirectStateAccess(IRRayTracingPipelineConfiguration* configuration, bool enableDirectStateAccess);

/**
 * Set the ray tracing pipeline configuration preferences for a compiler.
 * All subsequent shaders this compiler converts use this configuration.
 * @param compiler compiler object to configure.
 * @param configuration configuration to establish.
 * @note the configuration should remain consistent across all shaders that form a single ray tracing pipeline, otherwise
 * undefined behavior may occur.
 */
void IRCompilerSetRayTracingPipelineConfiguration(IRCompiler* compiler, const IRRayTracingPipelineConfiguration* configuration);

/**
 * Configure compiler compatibility flags.
 * Compatibility flags allow you to tailor code generation to the specific requirements of your shaders.
 * You typically enable compatibility flags to support a broader set of features and behaviors (such as out-of-bounds reads) when your shader needs them to operate correctly.
 * These flags, however, carry a performance cost.
 * Always use the minimum set of compatibility flags your shader needs to attain the highest runtime performance for IR code you compile.
 * @param compiler the compiler to configure
 * @param flags bitmask of compatibility flags to enable.
 */
void IRCompilerSetCompatibilityFlags(IRCompiler* compiler, IRCompatibilityFlags flags);

/**
 * Set primitive input topology
 * Provides the compiler with information about input topology this shader will be used with.
 * This information is required to correctly compile shaders that will render point primitive and may be used for other optimizations.
 * @param compiler the compiler to configure
 * @param inputTopology input topology
 */
void IRCompilerSetInputTopology(IRCompiler* compiler, IRInputTopology inputTopology);

/**
 * Enable geometry and tessellation emulation.
 * @param compiler compiler to configure with geometry and tessellation emulation.
 * @param enable pass in true to enable geometry and tessellation emulation, false to disable it.
 */
void IRCompilerEnableGeometryAndTessellationEmulation(IRCompiler* compiler, bool enable);


typedef enum IRDualSourceBlendingConfiguration
{
    IRDualSourceBlendingConfigurationDecideAtRuntime,
    IRDualSourceBlendingConfigurationForceEnabled,
    IRDualSourceBlendingConfigurationForceDisabled
} IRDualSourceBlendingConfiguration;

/**
 * Enable dual-source blending support.
 * When the configuration parameter is set to `IRDualSourceBlendingConfigurationDecideAtRuntime`, you must provide a function constant named "`dualSourceBlendingEnabled`" of type `uint8_t` at
 * pipeline creation time, specifying whether to enable dual source blending.
 * @param compiler compiler to configure.
 * @param configuration set to enabled or disable to enable and disable support. Set to decide-at-runtime to control support at PSO-creation time via function constants.
 * This parameter is reset after each compilation.
 */
void IRCompilerSetDualSourceBlendingConfiguration(IRCompiler* compiler, IRDualSourceBlendingConfiguration configuration);


typedef enum IRDepthFeedbackConfiguration
{
    IRDepthFeedbackConfigurationDecideAtRuntime,
    IRDepthFeedbackConfigurationForceEnabled,
    IRDepthFeedbackConfigurationForceDisabled
} IRDepthFeedbackConfiguration;

/**
 * Enable depth feedback support.
 * When the configuration parameter is set to `IRDepthWriteConfigurationDecideAtRuntime`, you must provide a function constant named "`depthFeedbackEnabled`" of type `uint8_t` at
 * pipeline creation time, specifying whether to enable depth feedback.
 * @param compiler compiler to configure.
 * @param configuration set to enabled or disable to enable and disable support. Set to decide-at-runtime to control support at PSO-creation time via function constants.
 * This parameter is reset after each compilation.
 */
void IRCompilerSetDepthFeedbackConfiguration(IRCompiler* compiler, IRDepthFeedbackConfiguration configuration);

/**
 * Set Int compatible render target mask.
 * Inform the compiler if the underlying render target is int compatible or not. The compiler will inject appropriate bitcast.
 * @param compiler compiler to configure.
 * @param intRTMask bitmask for each render target - 0 means that the RT is float compatible - 1 means that the RT is int compatible
 * This parameter is reset after each compilation.
 */
void IRCompilerSetIntRTMask(IRCompiler* compiler, uint8_t intRTMask);

/**
 * Set fragment shader output sample mask.
 * @param compiler compiler to configure.
 * @param sampleMask bitmask to inform which samples get updated in active render targets. Defaults to 0xffffffff to update all samples
 * This parameter is reset after each compilation.
 */
void IRCompilerSetSampleMask(IRCompiler* compiler, uint32_t sampleMask);

/**
 * Synthesize an ray dispatch function that indirectly dispatches ray generation shaders.
 * @param compiler compiler configuration to use.
 * @param binary pointer to a binary into which to write the synthesized MetalIR.
 * @return true if the intersection function could be successfully synthesized, false otherwise.
 */
bool IRMetalLibSynthesizeIndirectRayDispatchFunction(const IRCompiler* compiler, IRMetalLibBinary* binary);

/**
 * Synthesize a Metal ray tracing intersection function that calls into a visible function representing
 * a custom intersection function.
 * @param compiler compiler configuration to use. Current hit group and ray tracing configuration influences code generation.
 * @param binary pointer to a binary into which to write the synthesized MetalIR.
 * @return true if the intersection function could be successfully synthesized, false otherwise.
 */
bool IRMetalLibSynthesizeIndirectIntersectionFunction(const IRCompiler* compiler, IRMetalLibBinary* binary);

/**
 * Customize the name of the entry point functions generated by a compiler.
 * @param compiler compiler to configure.
 * @param newName name IRConverter assigns to the emitted entry point.
 */
void IRCompilerSetEntryPointName(IRCompiler* compiler, const char* newName);

typedef enum IRGPUFamily
{
    IRGPUFamilyApple6  = 1006,
    IRGPUFamilyApple7  = 1007,
    IRGPUFamilyApple8  = 1008,
    IRGPUFamilyApple9  = 1009,
    IRGPUFamilyApple10 = 1010,
    
    IRGPUFamilyMetal3  = 5001
} IRGPUFamily;

/**
 * Set the minimum GPU deployment target for MetalIR code generation.
 * Targeting a newer family may enable the compiler to emit MetalIR further optimized for newer GPUs, but may render it incompatible with older models.
 * @param compiler compiler to configure.
 * @param family minimum GPU family supported by code generation.
 */
void IRCompilerSetMinimumGPUFamily(IRCompiler* compiler, IRGPUFamily family);

/**
 * Set compiler settings to ignore root signature when needed
 * @param compiler compiler for which to set the flags.
 * @param ignoreEmbeddedRootSignature whether embeddedRootSignature should be ignored
 */
void IRCompilerIgnoreRootSignature(IRCompiler* compiler, bool ignoreEmbeddedRootSignature);

/**
 * Set compiler settings to ignore debug information.
 * @param compiler compiler for which to set the flags.
 * @param ignoreDebugInformation whether dxil debug information should be ignored. Defaults to false.
 */
void IRCompilerIgnoreDebugInformation(IRCompiler* compiler, bool ignoreDebugInformation);

typedef enum IROperatingSystem
{
    IROperatingSystem_macOS,
    IROperatingSystem_iOS,
    IROperatingSystem_tvOS,
    IROperatingSystem_iOSSimulator
} IROperatingSystem;

/**
 * Set the minimum operating system software version target for Metal IR code generation.
 * Targeting a newer software version may enable the compiler to emi MetalIR further optimized for newer macOS and iOS releases, but it may render it incompatible with older operating system versions.
 * Setting a minimum deployment target newer than your SDK may produce an `IRErrorCodeUnableToLinkModule` error.
 * @param compiler compiler to configure.
 * @param operatingSystem operating system name.
 * @param version operating system version, such as "13.0.0" or "16.0.0".
*/
void IRCompilerSetMinimumDeploymentTarget(IRCompiler* compiler, IROperatingSystem operatingSystem, const char* version);

/**
 * Set the register space that all function constant resources will use in the shader.
 * Any cbuffer CBV defined within this space is treated as a function constant of size uint4 and with the name "FunctionConstant\_regN" where "N" is the register index
 * These resources do not need to be present in the root signature, if they are present in the root signature the root signature properties for that resource are ignored
 * @param compiler compiler to configure.
 * @param spaceValue the register space that function constant cbuffer resources are defined in, set to UINT32_MAX to disable
 */
 
void IRCompilerSetFunctionConstantResourceSpace(IRCompiler* compiler, uint32_t spaceValue);

/**
 * Set the register space that all framebuffer fetch resources will use in the shader.
 * Any Texture2D SRV defined within this space is treated as a framebuffer fetch input to the pixel shader, where the register index indicates the framebuffer attachment index
 * These resources do not need to be present in the root signature, if they are present in the root signature the root signature properties for that resource are ignored
 * @param compiler compiler to configure.
 * @param spaceValue the register space that framebuffer fetch texture2d resources are defined in, set to UINT32_MAX to disable
 */
 
void IRCompilerSetFramebufferFetchResourceSpace(IRCompiler* compiler, uint32_t spaceValue);

// Metallib manipulation

/**
 * Create an empty metallib binary.
 * @return a new, empty metallib binary.
 */
IRMetalLibBinary* IRMetalLibBinaryCreate(void);

/**
 * Destroy a metallib binary.
 * @param lib library to destroy.
 */
void IRMetalLibBinaryDestroy(IRMetalLibBinary* lib);

/**
 * Copy the bytecode from a metallib library into a byte array.
 * @param lib metallib library from where to copy the bytecode.
 * @param outBytecode into which to write the bytecode. This parameter must be an array of at least IRMetalLibGetBytecodeSize bytes.
 * @return number of bytes written.
 */
size_t IRMetalLibGetBytecode(const IRMetalLibBinary* lib, uint8_t* outBytecode);

/**
 * Obtain the number of bytes needed to store the bytecode in a metallib.
 * @param lib metallib to query.
 * @return size in bytes needed to store the metallib's bytecode.
 */
size_t IRMetalLibGetBytecodeSize(const IRMetalLibBinary* lib);

#ifdef __APPLE__
/**
 * Obtain a direct pointer into a metallib's bytecode, avoiding a copy operation.
 * @param lib metallib library containing the bytecode.
 * @return a direct pointer to the metallib's bytecode. Do not release this object.
 */
dispatch_data_t IRMetalLibGetBytecodeData(const IRMetalLibBinary* lib);
#endif // __APPLE__

/**
 * Serialize a MetalIR object's shader bytecode to disk.
 * @param outputPath path into which to write the serialized MetalIR.
 * @param obj IRObject containing the bytecode to serialize.
 * @param stage shader stage to serialize.
 */
bool IRObjectSerialize(const char* outputPath, const IRObject* obj, IRShaderStage stage);

/**
 * Create an empty reflection object.
 * @return a new empty reflection object.
 */
IRShaderReflection* IRShaderReflectionCreate(void);

/**
 * Release a reflection object.
 * @param reflection reflection object to release.
 */
void IRShaderReflectionDestroy(IRShaderReflection* reflection);

/**
 * Obtain the name of the entry point from a reflection object.
 * @return pointer to the name of the entry point. Do not free this pointer, its lifecycle is managed by, and coincides with, the reflection object's.
 */
const char* IRShaderReflectionGetEntryPointFunctionName(const IRShaderReflection* reflection);

/**
 * Determine whether the shader requires supplemental information to operate correctly.
 * @param reflection reflection object to evaluate.
 * @return true if the compiled shader requires supplemental information in the form of function constants, false otherwise.
 **/
bool IRShaderReflectionNeedsFunctionConstants(const IRShaderReflection* reflection);

/**
 * Obtain the number of function constants in the reflection object.
 * @param reflection reflection object to query.
 * @return function constant count. May be zero if the shader stage to which this reflection corresponds has no function constants.
 **/
size_t IRShaderReflectionGetFunctionConstantCount(const IRShaderReflection* reflection);

/**
 * Function constant types. Values match MTLDataType enum.
 */
typedef enum IRFunctionConstantType
{
    IRFunctionConstantTypeBool   = 53,
    IRFunctionConstantTypeInt    = 29,
    IRFunctionConstantTypeInt4   = 32,
    IRFunctionConstantTypeFloat  = 3,
    
} IRFunctionConstantType;

typedef struct IRFunctionConstant
{
    const char* name;
    IRFunctionConstantType type;
} IRFunctionConstant;

/**
 * Copy function constant reflection data from a reflection object to an array.
 * You are responsible for calling IRShaderReflectionReleaseFunctionConstants on the array to ensure copied function constant reflection data are released.
 * @param reflection reflection object to query.
 * @param functionConstants function constants array into which to write the reflection data. This object needs to be able to store at least
 * IRShaderReflectionFunctionConstantCount function constants.
 **/
void IRShaderReflectionCopyFunctionConstants(const IRShaderReflection* reflection, IRFunctionConstant* functionConstants);

/**
 * Release function constant data.
 * @param functionConstants array of function constants.
 * @param functionConstantCount number of function constants in the array.
 */
void IRShaderReflectionReleaseFunctionConstants(IRFunctionConstant* functionConstants, size_t functionConstantCount);

typedef enum IRReflectionVersion
{
    IRReflectionVersion_1_0 = 1
} IRReflectionVersion;

// Compute stage info

typedef struct IRCSInfo_1_0
{
    uint32_t tg_size[3];
} IRCSInfo_1_0;

typedef struct IRVersionedCSInfo
{
    IRReflectionVersion version;
    union
    {
        IRCSInfo_1_0 info_1_0;
    };
} IRVersionedCSInfo;

// Vertex stage info

typedef struct IRVertexInputInfo_1_0
{
    const char* name;
    uint8_t attributeIndex;
} IRVertexInputInfo_1_0;

typedef struct IRVSInfo_1_0
{
    int instance_id_index;
    int vertex_id_index;
    uint32_t vertex_output_size_in_bytes;
    bool needs_draw_params;
    IRVertexInputInfo_1_0* vertex_inputs;
    size_t num_vertex_inputs;
} IRVSInfo_1_0;

typedef struct IRVersionedVSInfo
{
    IRReflectionVersion version;
    union
    {
        IRVSInfo_1_0 info_1_0;
    };
} IRVersionedVSInfo;

// Fragment stage info

typedef struct IRFSInfo_1_0
{
    int num_render_targets;
    uint8_t rt_index_int;
    bool discards;
} IRFSInfo_1_0;

typedef struct IRVersionedFSInfo
{
    IRReflectionVersion version;
    union
    {
        IRFSInfo_1_0 info_1_0;
    };
} IRVersionedFSInfo;

// Geometry stage info

typedef struct IRVertexOutputInfo_1_0
{
    const char* name;
    uint8_t attributeIndex;
} IRVertexOutputInfo_1_0;

typedef struct IRGSInfo_1_0
{
    IRVertexOutputInfo_1_0* vertex_outputs;
    size_t num_vertex_outputs;
    
    bool is_passthrough;
    
    //Only valid if the GS shader is passthrough
    int32_t rt_array_index_record_id;
    int32_t viewport_array_index_record_id;
    
    IRInputPrimitive input_primitive;
    uint32_t max_input_primitives_per_mesh_threadgroup;
    uint32_t max_payload_size_in_bytes;
    uint32_t instance_count;
} IRGSInfo_1_0;

typedef struct IRVersionedGSInfo
{
    IRReflectionVersion version;
    union
    {
        IRGSInfo_1_0 info_1_0;
    };
} IRVersionedGSInfo;

// Hull stage info

typedef struct IRHSInfo_1_0
{
    uint32_t max_patches_per_object_threadgroup;
    uint32_t max_object_threads_per_patch;
    uint32_t patch_constants_size;
    const char* patch_constant_function;
    uint32_t static_payload_size;
    uint32_t payload_size_per_patch;
    
    uint32_t input_control_point_count;
    uint32_t output_control_point_count;
    uint32_t output_control_point_size;
    
    IRTessellatorDomain tessellator_domain;
    IRTessellatorPartitioning tessellator_partitioning;
    IRTessellatorOutputPrimitive tessellator_output_primitive;
    
    bool tessellation_type_half;
    float max_tessellation_factor;
} IRHSInfo_1_0;

typedef struct IRVersionedHSInfo
{
    IRReflectionVersion version;
    union
    {
        IRHSInfo_1_0 info_1_0;
    };
} IRVersionedHSInfo;

// Domain stage info

typedef struct IRDSInfo_1_0
{
    IRTessellatorDomain tessellator_domain;
    uint32_t max_input_prims_per_mesh_threadgroup;
    uint32_t input_control_point_count;
    uint32_t input_control_point_size;
    uint32_t patch_constants_size;
    bool tessellation_type_half;
} IRDSInfo_1_0;

typedef struct IRVersionedDSInfo
{
    IRReflectionVersion version;
    union
    {
        IRDSInfo_1_0 info_1_0;
    };
} IRVersionedDSInfo;

// Mesh stage info

typedef enum IRMeshShaderPrimitiveTopology
{
    IRMeshShaderPrimitiveTopologyPoint     = 0,
    IRMeshShaderPrimitiveTopologyLine      = 1,
    IRMeshShaderPrimitiveTopologyTriangle  = 2,
    IRMeshShaderPrimitiveTopologyUndefined = 3
} IRMeshShaderPrimitiveTopology;

typedef struct IRMSInfo_1_0
{
    uint32_t max_vertex_output_count;
    uint32_t max_primitive_output_count;
    IRMeshShaderPrimitiveTopology primitive_topology;
    uint32_t max_payload_size_in_bytes;
    uint32_t num_threads[3];
} IRMSInfo_1_0;

typedef struct IRVersionedMSInfo
{
    IRReflectionVersion version;
    union
    {
        IRMSInfo_1_0 info_1_0;
    };
} IRVersionedMSInfo;

// Amplification stage info

typedef struct IRASInfo_1_0
{
    uint32_t num_threads[3];
    uint32_t max_payload_size_in_bytes;
} IRASInfo_1_0;

typedef struct IRVersionedASInfo
{
    IRReflectionVersion version;
    union
    {
        IRASInfo_1_0 info_1_0;
    };
} IRVersionedASInfo;

typedef struct IRRTInfo_1_0
{
    bool is_indirect_intersection_function;
} IRRTInfo_1_0;

// Ray tracing stage info
typedef struct IRVersionedRTInfo
{
    IRReflectionVersion version;
    union
    {
        IRRTInfo_1_0 info_1_0;
    };
} IRVersionedRTInfo;

/**
 * Copy shader reflection for a compute stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param csinfo pointer to a versioned CS Info struct into which to copy the reflection data for the stage. You must release the contents of this stuct by calling IRShaderReflectionReleaseComputeInfo.
 * @return true if the reflection object contains compute reflection information for the specified version.
 */
bool IRShaderReflectionCopyComputeInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedCSInfo* csinfo);

/**
 * Copy shader reflection for a vertex stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param vsinfo pointer to a versioned VS Info struct into which to copy the reflection data for the stage.  You must release the contents of this stuct by calling IRShaderReflectionReleaseVertexInfo.
 * @return true if the reflection object contains vertex stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyVertexInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedVSInfo* vsinfo);

/**
 * Copy shader reflection for a fragment stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param fsinfo pointer to a versioned FS Info struct into which to copy the reflection data for the stage.  You must release the contents of this stuct by calling IRShaderReflectionReleaseFragmentInfo.
 * @return true if the reflection object contains fragment stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyFragmentInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedFSInfo* fsinfo);

/**
 * Copy shader reflection for a geometry stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param gsinfo pointer to a versioned GS Info struct into which to copy the reflection data for the stage. You must release the contents of this stuct by calling IRShaderReflectionReleaseGeometryInfo.
 * @return true if the reflection object contains geometry stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyGeometryInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedGSInfo* gsinfo);

/**
 * Copy shader reflection for a hull shader stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param hsinfo pointer to a versioned HS Info struct into which to copy the reflection data for the stage. You must release the contents of this stuct by calling IRShaderReflectionReleaseHullInfo.
 * @return true if the reflection object contains hull shader stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyHullInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedHSInfo* hsinfo);

/**
 * Copy shader reflection for a domain shader stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param dsinfo pointer to a versioned DS Info struct into which to copy the reflection data for the stage.  You must release the contents of this stuct by calling IRShaderReflectionReleaseDomainInfo.
 * @return true if the reflection object contains domain shader stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyDomainInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedDSInfo* dsinfo);

/**
 * Copy shader reflection for a mesh shader stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param msinfo pointer to a versioned MS Info struct into which to copy the reflection data for the stage.  You must release the contents of this stuct by calling IRShaderReflectionReleaseMeshInfo.
 * @return true if the reflection object contains domain shader stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyMeshInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedMSInfo* msinfo);

/**
 * Copy shader reflection for an amplification shader stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param asinfo pointer to a versioned AS Info struct into which to copy the reflection data for the stage.  You must release the contents of this stuct by calling IRShaderReflectionReleaseAmplificationInfo.
 * @return true if the reflection object contains domain shader stage reflection information for the specified version.
 */
bool IRShaderReflectionCopyAmplificationInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedASInfo* asinfo);

/**
 * Copy shader reflection for a ray tracing shader stage.
 * @param reflection reflection object to query.
 * @param version version of the reflection data to obtain.
 * @param rtinfo pointer to a versioned RT Info struct into which to copy the reflection data for the stage.  You must release the contents of this struct by calling IRShaderReflectionReleaseRaytracingInfo.
 * @return true if the reflection object contains reflection for a ray tracing stage for the specified version.
 */
bool IRShaderReflectionCopyRaytracingInfo(const IRShaderReflection* reflection, IRReflectionVersion version, IRVersionedRTInfo* rtinfo);

/**
 * Release versioned compute information.
 * @param csinfo pointer to the compute shader reflection information to release.
 * @return false if the csinfo version is an unrecognized version or the csinfo pointer is null.
 */
bool IRShaderReflectionReleaseComputeInfo(IRVersionedCSInfo* csinfo);

/**
 * Release versioned vertex stage information.
 * @param vsinfo pointer to the vertex shader reflection information to release.
 * @return false if the vsinfo version is an unrecognized version or the vsinfo pointer is null.
 */
bool IRShaderReflectionReleaseVertexInfo(IRVersionedVSInfo* vsinfo);

/**
 * Release versioned fragment stage information.
 * @param fsinfo pointer to the fragment shader reflection information to release.
 * @return false if the fsinfo version is an unrecognized version or the fsinfo pointer is null.
 */
bool IRShaderReflectionReleaseFragmentInfo(IRVersionedFSInfo* fsinfo);

/**
 * Release versioned geometry stage information.
 * @param gsinfo pointer to the geometry shader reflection information to release.
 * @return false if the gsinfo version is an unrecognized version or the gsinfo pointer is null.
 */
bool IRShaderReflectionReleaseGeometryInfo(IRVersionedGSInfo* gsinfo);

/**
 * Release versioned hull stage information.
 * @param hsinfo pointer to the hull shader reflection information to release.
 * @return false if the hsinfo version is an unrecognized version or the hsinfo pointer is null.
 */
bool IRShaderReflectionReleaseHullInfo(IRVersionedHSInfo* hsinfo);

/**
 * Release versioned domain stage information.
 * @param dsinfo pointer to the domain shader reflection information to release.
 * @return false if the dsinfo version is an unrecognized version or the dsinfo pointer is null.
 */
bool IRShaderReflectionReleaseDomainInfo(IRVersionedDSInfo* dsinfo);

/**
 * Release versioned mesh stage information.
 * @param msinfo pointer to the mesh shader reflection information to release.
 * @return false if the msinfo version is an unrecognized version or the msinfo pointer is null.
 */
bool IRShaderReflectionReleaseMeshInfo(IRVersionedMSInfo* msinfo);

/**
 * Release versioned amplification stage information.
 * @param asinfo pointer to the amplification shader reflection information to release.
 * @return false if the asinfo version is an unrecognized version or the asinfo pointer is null.
 */
bool IRShaderReflectionReleaseAmplificationInfo(IRVersionedASInfo* asinfo);

/**
 * Release versioned ray tracing stage information.
 * @param rtinfo pointer to the ray tracing shader reflection information to release.
 * @return false if the rtinfo version is an unrecognized version or the rtinfo pointer is null.
 */
bool IRShaderReflectionReleaseRaytracingInfo(IRVersionedRTInfo* rtinfo);

/**
 * Represents a shader resource location from reflection data.
 */
typedef struct IRResourceLocation
{
    IRResourceType resourceType;      /**< Resource type. */
    uint32_t space;                   /**< DXIL space of this resource. */
    uint32_t slot;                    /**< DXIL slot of this resource. */
    uint32_t topLevelOffset;          /**< Offset in bytes into the top-level argument buffer. */
    uint64_t sizeBytes;               /**< Size of the entry in the argument buffer in bytes. */
    const char* resourceName;         /**< Name of the resource. String is non-owned and points into the parent reflection object. May be NULL. */
} IRResourceLocation;

/**
 * Obtain the number of resources referenced by the top-level argument buffer
 * @param reflection the reflection object for which to obtain the resource count.
 * @return number of resources the top-level argument buffer references.
 */
size_t IRShaderReflectionGetResourceCount(const IRShaderReflection* reflection);

/**
 * Get the locations within the top-level Argument Buffer for all top-level resources.
 * @param reflection the reflection object resulting from the compilation process.
 * @param resourceLocations parameter into which to write resource locations. This array must contain enough storage to write IRShaderReflectionGetResourceCount() elements.
 * @note string references within the resource locations are pointers into the reflection object. The reflection object's lifecycle must be preserved while accessing these strings. Names may be NULL.
 */
void IRShaderReflectionGetResourceLocations(const IRShaderReflection* reflection, IRResourceLocation* resourceLocations);

/**
 * Obtain the number of resources referenced in a top-level Argument Buffer using a hierarchical layout.
 * @param rootSignature root signature corresponding to the hierarchical layout definition.
 * @return number of resources the top-level argument buffer references.
 */
size_t IRRootSignatureGetResourceCount(const IRRootSignature* rootSignature);

/**
 * Get the locations of resources in the top-level Argument Buffer.
 * @param rootSignature root signature corresponding to the hierarchical layout definition.
 * @param resourceLocations parameter into which to write resource locations. This array must contain enough storage to write IRRootSignatureGetResourceCount() elements.
 */
void IRRootSignatureGetResourceLocations(const IRRootSignature* rootSignature, IRResourceLocation* resourceLocations);

/**
 * Serialize reflection information into JSON.
 * @param reflection reflection object.
 * @return null-terminated string containing JSON. You need to release this string by calling IRShaderReflectionFreeString.
 * @deprecated use IRShaderReflectionCopyJSONString instead.
 */
IR_DEPRECATED("use IRShaderReflectionCopyJSONString instead.")
const char* IRShaderReflectionAllocStringAndSerialize(IRShaderReflection* reflection);

/**
 * Serialize reflection information into JSON.
 * @param reflection reflection object.
 * @return null-terminated string containing JSON. You need to release this string by calling IRShaderReflectionFreeString.
 */
const char* IRShaderReflectionCopyJSONString(const IRShaderReflection* reflection);

/**
 * Release a string allocated by IRShaderReflectionAllocStringAndSerialize.
 * @param serialized string to release.
 * @deprecated use IRShaderReflectionReleaseString instead.
 */
IR_DEPRECATED("use IRShaderReflectionReleaseString instead.")
void IRShaderReflectionFreeString(const char* serialized);

/**
 * Release a string allocated by IRShaderReflectionAllocStringAndSerialize.
 * @param serialized string to release.
 */
void IRShaderReflectionReleaseString(const char* serialized);

/**
 * Deserialize a JSON string into a reflection object.
 * @param blob null-terminated JSON string containing reflection information.
 * @param reflection reflection object into which to deserialize.
 * @deprecated use IRShaderReflectionCreateFromJSON instead.
 */
IR_DEPRECATED("use IRShaderReflectionCreateFromJSON instead.")
void IRShaderReflectionDeserialize(const char* blob, IRShaderReflection* reflection);

/**
 * Deserialize a JSON string into a reflection object.
 * @param json null-terminated JSON string containing reflection information.
 * @return a newly-allocated shader reflection object that you need to release by calling IRShaderReflectionDestroy,
 * or NULL on error.
 */
IRShaderReflection* IRShaderReflectionCreateFromJSON(const char* json);

/**
 * Serialize a root signature descriptor into a string representation.
 * @param rootSignatureDescriptor root signature descriptor to serialize.
 * @return a string representation of the root signature descriptor. You need to release this string by calling IRVersionedRootSignatureDescriptorFreeString.
 * @deprecated use IRVersionedRootSignatureDescriptorCopyJSONString instead.
 */
IR_DEPRECATED("use IRVersionedRootSignatureDescriptorCopyJSONString instead.")
const char* IRVersionedRootSignatureDescriptorAllocStringAndSerialize(IRVersionedRootSignatureDescriptor* rootSignatureDescriptor);

/**
 * Serialize a root signature descriptor into a string representation.
 * @param rootSignatureDescriptor root signature descriptor to serialize.
 * @return a string representation of the root signature descriptor. You need to release this string by calling IRVersionedRootSignatureDescriptorFreeString.
 */
const char* IRVersionedRootSignatureDescriptorCopyJSONString(const IRVersionedRootSignatureDescriptor* rootSignatureDescriptor);

/**
 * Release a string allocated by IRVersionedRootSignatureDescriptorAllocStringAndSerialize.
 * @param serialized string to release.
 * @deprecated use IRVersionedRootSignatureDescriptorReleaseString instead.
 */
IR_DEPRECATED("use IRVersionedRootSignatureDescriptorReleaseString instead.")
void IRVersionedRootSignatureDescriptorFreeString(const char* serialized);

/**
 * Release a string allocated by IRVersionedRootSignatureDescriptorAllocStringAndSerialize.
 * @param serialized string to release.
 */
void IRVersionedRootSignatureDescriptorReleaseString(const char* serialized);

/**
 * Deserialize a string representation of a root signature into a root signature object.
 * @param serialized a string representation of a root signature.
 * @param rootSignatureDescriptor root signature object into which to deserialize the root signature.
 * @return true if deserialization is successful, false otherwise.
 * @warning this function may allocate memory, call IRVersionedRootSignatureDescriptorReleaseArrays to deallocate any allocated memory.
 * @deprecated use IRVersionedRootSignatureDescriptorCreateFromJSON instead.
 */
IR_DEPRECATED("use IRVersionedRootSignatureDescriptorCreateFromJSON instead.")
bool IRVersionedRootSignatureDescriptorDeserialize(const char* serialized, IRVersionedRootSignatureDescriptor* rootSignatureDescriptor);

/**
 * Deserialize a string representation of a root signature into a root signature object.
 * @param serialized a string representation of a root signature.
 * @return a newly-allocated root signature object that you need to release, or NULL on error.
 */
IRVersionedRootSignatureDescriptor* IRVersionedRootSignatureDescriptorCreateFromJSON(const char* serialized);

/**
 * Deserialize a versioned root signature descriptor from an array of bytes (blob).
 * @param blob an array of bytes. This can correspond to a DXIL container, or a raw versioned root signature descriptor.
 * @param blobSize number of bytes in the blob.
 * @param error optional out parameter containing details about any deserialization errors.
 * @return a deserialized versioned root signature descriptor, or NULL in case of an error.
 * @note you need to release the returned instance by calling IRVersionedRootSignatureDescriptorRelease.
 */
IRVersionedRootSignatureDescriptor* IRVersionedRootSignatureDescriptorCreateFromBlob(const uint8_t* blob, uint32_t blobSize, IRError** error);

/**
 * Release any arrays allocated by IRVersionedRootSignatureDescriptorDeserialize.
 * @param rootSignatureDescriptor root signature descriptor to release.
 */
void IRVersionedRootSignatureDescriptorRelease(IRVersionedRootSignatureDescriptor* rootSignatureDescriptor);

/**
 * Serialize an input layout descriptor version 1 into a string.
 * @param inputLayoutDescriptor descriptor to serialize.
 * @return a string representation of the input layout descriptor. You need to release this string by calling IRInputLayoutDescriptor1FreeString.
 */
const char* IRInputLayoutDescriptor1CopyJSONString(const IRInputLayoutDescriptor1* inputLayoutDescriptor);

/**
 * Release a string allocated by IRInputLayoutDescriptor1CopyJSONString.
 * @param serialized string to release.
 */
void IRInputLayoutDescriptor1ReleaseString(const char* serialized);

/**
 * Deserialize a string representation of an input layout descriptor version 1 into an IRInputLayoutDescriptor1 structure.
 * @param serialized a string representation of an input layout descriptor version 1.
 * @return a newly-allocated input layout descriptor version 1 object that you need to release by calling IRInputLayoutDescriptor1Release,
 * NULL if an error occurs.
 */
IRInputLayoutDescriptor1* IRInputLayoutDescriptor1CreateFromJSON(const char* serialized);

/**
 * Release an IRInputDescriptor1 instance allocated by IRInputLayoutDescriptor1CreateFromJSON.
 * @param inputLayoutDescriptor input layout descriptor to release.
 */
void IRInputLayoutDescriptor1Release(IRInputLayoutDescriptor1* inputLayoutDescriptor);

#ifdef __cplusplus
}
#endif
