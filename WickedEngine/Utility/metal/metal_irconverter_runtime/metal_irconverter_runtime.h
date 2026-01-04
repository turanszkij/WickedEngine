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

#if (!defined MTL_EXTERN) && (!defined _MTL_EXTERN)
#error This file requires Metal.h or Metal.hpp to be included before it.
#endif

#ifdef IR_RUNTIME_METALCPP
typedef NS::Error*                          nserror_t;
typedef NS::String*                         nsstring_t;
typedef MTL::Texture*                       texture_t;
typedef MTL::Buffer*                        buffer_t;
typedef MTL::SamplerState*                  sampler_t;
typedef MTL::RenderCommandEncoder*          renderencoder_t;
typedef MTL::PrimitiveType                  primitivetype_t;
typedef MTL::FunctionConstantValues*        functionconstantvalues_t;
typedef MTL::IndexType                      indextype_t;
typedef MTL::Size                           mtlsize_t;
typedef MTL::RenderPipelineState*           renderpipelinestate_t;
typedef MTL::Library*                       library_t;
typedef MTL::MeshRenderPipelineDescriptor*  meshpipelinedescriptor_t;
typedef MTL::Device*                        device_t;
typedef MTL::ResourceID                     resourceid_t;
typedef NS::UInteger                        uinteger_t;
#else
typedef NSError*                            nserror_t;
typedef NSString*                           nsstring_t;
typedef id<MTLTexture>                      texture_t;
typedef id<MTLBuffer>                       buffer_t;
typedef id<MTLSamplerState>                 sampler_t;
typedef id<MTLRenderCommandEncoder>         renderencoder_t;
typedef MTLPrimitiveType                    primitivetype_t;
typedef MTLFunctionConstantValues*          functionconstantvalues_t;
typedef MTLIndexType                        indextype_t;
typedef MTLSize                             mtlsize_t;
typedef id<MTLRenderPipelineState>          renderpipelinestate_t;
typedef id<MTLLibrary>                      library_t;
typedef MTLMeshRenderPipelineDescriptor*    meshpipelinedescriptor_t;
typedef id<MTLDevice>                       device_t;
typedef MTLResourceID                       resourceid_t;
typedef NSUInteger                          uinteger_t;
#endif

#ifdef __cplusplus
#include <cstdint>
#include <sstream>
#include <cstring>
extern "C" {
#else
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#endif

#define IR_MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#define IR_INLINE       __attribute__((always_inline))
#define IR_OVERLOADABLE __attribute__((overloadable))

extern const uint64_t kIRArgumentBufferBindPoint;
extern const uint64_t kIRDescriptorHeapBindPoint;
extern const uint64_t kIRSamplerHeapBindPoint;
extern const uint64_t kIRArgumentBufferHullDomainBindPoint;
extern const uint64_t kIRArgumentBufferDrawArgumentsBindPoint;
extern const uint64_t kIRArgumentBufferUniformsBindPoint;
extern const uint64_t kIRVertexBufferBindPoint;
extern const uint64_t kIRStageInAttributeStartIndex;

extern const char*    kIRIndirectTriangleIntersectionFunctionName;
extern const char*    kIRIndirectProceduralIntersectionFunctionName;

extern const char*    kIRTrianglePassthroughGeometryShader;
extern const char*    kIRLinePassthroughGeometryShader;
extern const char*    kIRPointPassthroughGeometryShader;

extern const char*    kIRFunctionGroupRayGeneration;
extern const char*    kIRFunctionGroupClosestHit;
extern const char*    kIRFunctionGroupMiss;

extern const uint16_t kIRNonIndexedDraw;

typedef struct IRDescriptorTableEntry
{
    uint64_t gpuVA;
    uint64_t textureViewID;
    uint64_t metadata;
} IRDescriptorTableEntry;

#include "ir_tessellator_tables.h"
#include "ir_raytracing.h"

typedef enum IRRuntimeResourceType
{
    IRRuntimeResourceTypeSRV = 0,
    IRRuntimeResourceTypeUAV = 1,
    IRRuntimeResourceTypeCBV = 2,
    IRRuntimeResourceTypeSMP = 3,
    IRRuntimeResourceTypeCount
} IRRuntimeResourceType;

typedef struct IRBufferView
{
    buffer_t       buffer;
    uint64_t       bufferOffset;
    uint64_t       bufferSize;
    texture_t      textureBufferView;
    uint32_t       textureViewOffsetInElements;
    bool           typedBuffer;
} IRBufferView;

typedef enum IRRuntimePrimitiveType
{
    IRRuntimePrimitiveTypePoint           = 0,
    IRRuntimePrimitiveTypeLine            = 1,
    IRRuntimePrimitiveTypeLineStrip       = 2,
    IRRuntimePrimitiveTypeTriangle        = 3,
    IRRuntimePrimitiveTypeTriangleStrip   = 4,
    IRRuntimePrimitiveTypeLineWithAdj     = 5,
    IRRuntimePrimitiveTypeTriangleWithAdj = 6,
    IRRuntimePrimitiveTypeLineStripWithAdj = 7,
    IRRuntimePrimitiveType1ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType2ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType3ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType4ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType5ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType6ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType7ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType8ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType9ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType10ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType11ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType12ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType13ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType14ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType15ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType16ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType17ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType18ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType19ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType20ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType21ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType22ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType23ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType24ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType25ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType26ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType27ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType28ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType29ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType30ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType31ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle,
    IRRuntimePrimitiveType32ControlPointPatchlist = IRRuntimePrimitiveTypeTriangle
} IRRuntimePrimitiveType;

typedef struct IRRuntimeGeometryPipelineConfig
{
    uint32_t gsVertexSizeInBytes;
    uint32_t gsMaxInputPrimitivesPerMeshThreadgroup;
} IRRuntimeGeometryPipelineConfig;

typedef enum IRRuntimeTessellatorOutputPrimitive
{
    IRRuntimeTessellatorOutputUndefined   = 0,
    IRRuntimeTessellatorOutputPoint       = 1,
    IRRuntimeTessellatorOutputLine        = 2,
    IRRuntimeTessellatorOutputTriangleCW  = 3,
    IRRuntimeTessellatorOutputTriangleCCW = 4,
} IRRuntimeTessellatorOutputPrimitive;

typedef struct IRRuntimeTessellationPipelineConfig
{
    IRRuntimeTessellatorOutputPrimitive outputPrimitiveType;
    uint32_t                            vsOutputSizeInBytes;
    uint32_t                            gsMaxInputPrimitivesPerMeshThreadgroup;
    uint32_t                            hsMaxPatchesPerObjectThreadgroup;
    uint32_t                            hsInputControlPointCount;
    uint32_t                            hsMaxObjectThreadsPerThreadgroup;
    float                               hsMaxTessellationFactor;
    uint32_t                            gsInstanceCount;
} IRRuntimeTessellationPipelineConfig;

typedef struct IRGeometryEmulationPipelineDescriptor
{
    library_t                       stageInLibrary;
    library_t                       vertexLibrary;
    const char*                     vertexFunctionName;
    library_t                       geometryLibrary;
    const char*                     geometryFunctionName;
    library_t                       fragmentLibrary;
    const char*                     fragmentFunctionName;
    meshpipelinedescriptor_t        basePipelineDescriptor;
    IRRuntimeGeometryPipelineConfig pipelineConfig;
} IRGeometryEmulationPipelineDescriptor;

typedef struct IRGeometryTessellationEmulationPipelineDescriptor
{
    library_t                           stageInLibrary;
    library_t                           vertexLibrary;
    const char*                         vertexFunctionName;
    library_t                           hullLibrary;
    const char*                         hullFunctionName;
    library_t                           domainLibrary;
    const char*                         domainFunctionName;
    library_t                           geometryLibrary;
    const char*                         geometryFunctionName;
    library_t                           fragmentLibrary;
    const char*                         fragmentFunctionName;
    meshpipelinedescriptor_t            basePipelineDescriptor;
    IRRuntimeTessellationPipelineConfig pipelineConfig;
} IRGeometryTessellationEmulationPipelineDescriptor;

typedef struct IRRuntimeVertexBuffer
{
    uint64_t addr;
    uint32_t length;
    uint32_t stride;
} IRRuntimeVertexBuffer;

typedef IRRuntimeVertexBuffer IRRuntimeVertexBuffers[31];

typedef struct IRRuntimeDrawArgument
{
    uint vertexCountPerInstance;
    uint instanceCount;
    uint startVertexLocation;
    uint startInstanceLocation;
} IRRuntimeDrawArgument;

typedef struct IRRuntimeDrawIndexedArgument
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int  baseVertexLocation;
    uint startInstanceLocation;
} IRRuntimeDrawIndexedArgument;

typedef struct IRRuntimeDrawParams
{
    union
    {
        IRRuntimeDrawArgument draw;
        IRRuntimeDrawIndexedArgument drawIndexed;
    };
} IRRuntimeDrawParams;

typedef struct IRRuntimeDrawInfo
{
    // Vertex pipelines only require the index type.
    uint16_t indexType; // set to kIRNonIndexedDraw to indicate a non-indexed draw call
    
    // Required by all mesh shader-based pipelines.
    uint8_t primitiveTopology;
    uint8_t threadsPerPatch;
    uint16_t maxInputPrimitivesPerMeshThreadgroup;
    uint16_t objectThreadgroupVertexStride;
    uint16_t meshThreadgroupPrimitiveStride;
    
    uint16_t gsInstanceCount;
    uint16_t patchesPerObjectThreadgroup;
    uint16_t inputControlPointsPerPatch;
    
    uint64_t indexBuffer; // position aligned to 8 bytes
} IRRuntimeDrawInfo;

typedef union IRRuntimeFunctionConstantValue
{
    struct { int32_t i0, i1, i2, i3; };
    struct { int16_t s0, s1, s2, s3, s4, s5, s6, s7; };
    struct { int64_t l0, l1; };
    struct { float f0, f1, f2, f3; };
} IRRuntimeFunctionConstantValue;
    
/**
 * Create a BufferView instance representing an append/consume buffer.
 * Append/consume buffers provide storage and an atomic insert/remove operation. This function takes an
 * input buffer for storage, creates the atomic counter, and bundles them together in a BufferView,
 * providing a convenient abstraction to use the input Metal buffer as an append/consume buffer.
 * @param device device that allocates the atomic counter.
 * @param appendBuffer a Metal buffer to use for backing storage of the append/consume buffer.
 * @param appendBufferOffset an optional buffer into the appendBuffer. May be zero.
 * @param initialCounterValue initial value for the atomic counter of the append/consume buffer.
 * @param outBufferView output parameter into which this function writes the append buffer implementation details.
 * Use function IRDescriptorTableSetBufferView() to bind an append/consume buffer to a descriptor table.
 */
void IRRuntimeCreateAppendBufferView(device_t device,
                                     buffer_t appendBuffer,
                                     uint64_t appendBufferOffset,
                                     uint32_t initialCounterValue,
                                     IRBufferView* outBufferView);

/**
 * Obtain the count of an append/consume buffer.
 * @note the backing MTLBuffer needs to have MTLStorageModeShared storage mode.
 * @param bufferView buffer view representing the append/consume buffer for which to retrieve the counter.
 * @return the current count of the append/consume buffer. This function doesn't cause a GPU-CPU sync.
 */
uint32_t IRRuntimeGetAppendBufferCount(const IRBufferView* bufferView);

/**
 * Obtain encoded metadata from a buffer view description.
 * @param view the view description to encode into the produced metadata.
**/
uint64_t IRDescriptorTableGetBufferMetadata(const IRBufferView* view);

/**
 * Encode a buffer into the argument buffer.
 * @param entry the pointer to the descriptor table entry to encode the buffer reference into.
 * @param gpu_va the GPU address of the buffer to encode.
 * @param metadata the metadata corresponding to the buffer view.
**/
void IRDescriptorTableSetBuffer(IRDescriptorTableEntry* entry, uint64_t gpu_va, uint64_t metadata);

/**
 * Encode a buffer into the argument buffer.
 * Use this function to encode a buffer that may also include a reference as a texture from a shader.
 * @param entry the pointer to the descriptor table entry to encode the buffer reference into.
 * @param bufferView the buffer view description.
**/
void IRDescriptorTableSetBufferView(IRDescriptorTableEntry* entry, const IRBufferView* bufferView);

/**
 * Encode a texture into the argument buffer.
 * @param entry the pointer to the descriptor table entry to encode the texture reference into.
 * @param argument the Metal texture to encode.
 * @param minLODClamp the minimum LOD clamp for the texture.
 * @param metadata the metadata needs to be zero.
**/
void IRDescriptorTableSetTexture(IRDescriptorTableEntry* entry, texture_t argument, float minLODClamp, uint32_t metadata);

/**
 * Encode a sampler into the argument buffer.
 * The sampler needs to support argument buffers (supportsArgumentBuffers=YES).
 * @param entry the pointer to the descriptor table entry to encode the sampler reference into.
 * @param argument the Metal sampler to encode.
 * @param lodBias the mip LOD bias for the sampler.
**/
void IRDescriptorTableSetSampler(IRDescriptorTableEntry* entry, sampler_t argument, float lodBias);

/**
 * Draw primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the primitive type to draw.
 * @param vertexStart the vertex start.
 * @param vertexCount the vertex count.
 * @param instanceCount the number of instances to draw.
 * @param baseInstance the first instance number.
**/
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount, uint64_t instanceCount, uint64_t baseInstance) IR_OVERLOADABLE;

/**
 * Draw primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the primitive type to draw.
 * @param vertexStart the vertex start.
 * @param vertexCount the vertex count.
 * @param instanceCount the number of instances to draw.
**/
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount, uint64_t instanceCount) IR_OVERLOADABLE;

/**
 * Draw primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the primitive type to draw.
 * @param vertexStart the vertex start.
 * @param vertexCount the vertex count.
**/
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount) IR_OVERLOADABLE;

/**
 * Draw primitives indirect providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the type of primitives that the vertices assemble into.
 * @param indirectBuffer the buffer from which the device reads draw call arguments as the MTLDrawPrimitivesIndirectArguments structure lays out.
 * @param indirectBufferOffset the byte offset within indirectBuffer to start reading arguments from. This needs to be a multiple of 4 bytes.
**/
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, buffer_t indirectBuffer, uint64_t indirectBufferOffset) IR_OVERLOADABLE;

/**
 * Draw indexed primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the type of primitive that the vertices assemble into.
 * @param indexCount for each instance the number of indices to read from the index buffer.
 * @param indexType the data type of the indices.
 * @param indexBuffer a buffer that contains indices to the vertices.
 * @param indexBufferOffset the byte offset within indexBuffer to start reading indices from.
 * @param instanceCount the number of instances to draw.
 * @param baseVertex the first vertex to draw.
 * @param baseInstance the first instance to draw.
**/
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, uint64_t instanceCount, int64_t baseVertex, uint64_t baseInstance) IR_OVERLOADABLE;

/**
 * Draw indexed primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the type of primitive that the vertices assemble into.
 * @param indexCount for each instance the number of indices to read from the index buffer.
 * @param indexType the data type of the indices.
 * @param indexBuffer a buffer that contains indices to the vertices.
 * @param indexBufferOffset byte offset within indexBuffer to start reading indices from.
 * @param instanceCount the number of instances to draw.
**/
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, uint64_t instanceCount) IR_OVERLOADABLE;

/**
 * Draw indexed primitives providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the type of primitive that the vertices are assembled into.
 * @param indexCount for each instance the number of indices to read from the index buffer.
 * @param indexType the data type of the indices.
 * @param indexBuffer a buffer that contains indices to the vertices.
 * @param indexBufferOffset the byte offset within indexBuffer to start reading indices from.
**/
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset) IR_OVERLOADABLE;

/**
 * Draw indexed primitives indirect providing draw call parameter information to the vertex stage.
 * @param enc the render command encoder.
 * @param primitiveType the type of the primitive that the vertices assemble into.
 * @param indexBuffer the buffer that contains indices to the vertices.
 * @param indexBufferOffset the byte offset within indexBuffer to start reading indices from.
 * @param indirectBuffer the buffer object from which the device reads draw call arguments as the MTLDrawIndexedPrimitivesIndirectArguments structure lays out.
 * @param indirectBufferOffset the byte offset within indirectBuffer to start reading arguments from. This needs to be a multiple of 4 bytes.
**/
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, buffer_t indirectBuffer, uint64_t indirectBufferOffset ) IR_OVERLOADABLE;

/**
 * Draw indexed primitives using an emulated geometry pipeline.
 * You need to bind your vertex arrays and strides before issuing this call.
 * Bind a buffer with IRRuntimeVertexBuffers at index 0 for the object stage,
 * You need to manually flag residency for all referenced vertex buffers and for the index buffer.
 */
void IRRuntimeDrawIndexedPrimitivesGeometryEmulation(renderencoder_t enc,
                                                     IRRuntimePrimitiveType primitiveType,
                                                     indextype_t indexType,
                                                     buffer_t indexBuffer,
                                                     IRRuntimeGeometryPipelineConfig geometryPipelineConfig,
                                                     uint32_t instanceCount,
                                                     uint32_t indexCountPerInstance,
                                                     uint32_t startIndex,
                                                     int baseVertex,
                                                     uint32_t baseInstance);

/**
 * Draw non-indexed primitives using an emulated geometry pipeline.
 * You need to bind your vertex arrays and strides before issuing this call.
 * Bind a buffer with IRRuntimeVertexBuffers at index 0 for the object stage,
 * You need to manually flag residency for all referenced vertex buffers.
 */
void IRRuntimeDrawPrimitivesGeometryEmulation(renderencoder_t enc,
                                              IRRuntimePrimitiveType primitiveType,
                                              IRRuntimeGeometryPipelineConfig geometryPipelineConfig,
                                              uint32_t instanceCount,
                                              uint32_t vertexCountPerInstance,
                                              uint32_t baseVertex,
                                              uint32_t baseInstance);

/**
 * * Draw indexed primitives using an emulated geometry/tessellation pipeline.
 * You need to bind your vertex arrays and strides before issuing this call.
 * Bind a buffer with IRRuntimeVertexBuffers at index 0 for the object stage,
 * You need to manually flag residency for all referenced vertex buffers and for the index buffer.
 */
void IRRuntimeDrawIndexedPatchesTessellationEmulation(renderencoder_t enc,
                                                      IRRuntimePrimitiveType primitiveTopology,
                                                      indextype_t indexType,
                                                      buffer_t indexBuffer,
                                                      IRRuntimeTessellationPipelineConfig tessellationPipelineConfig,
                                                      uint32_t instanceCount,
                                                      uint32_t indexCountPerInstance,
                                                      uint32_t baseInstance,
                                                      int32_t  baseVertex,
                                                      uint32_t startIndex);

/**
 * Draw non-indexed primitives using an emulated geometry/tessellation pipeline.
 * You need to bind your vertex arrays and strides before issuing this call.
 * Bind a buffer with IRRuntimeVertexBuffers at index 0 for the object stage,
 * You need to manually flag residency for all referenced vertex buffers.
 */
void IRRuntimeDrawPatchesTessellationEmulation(renderencoder_t enc,
                                               IRRuntimePrimitiveType primitiveTopology,
                                               IRRuntimeTessellationPipelineConfig tessellationPipelineConfig,
                                               uint32_t instanceCount,
                                               uint32_t vertexCountPerInstance,
                                               uint32_t baseInstance,
                                               uint32_t baseVertex);

/**
 * Validate that the hull domain and tessellation stages are compatible from their reflection data.
 * @param hsTessellatorOutputPrimitive the tessellator's output primitive which needs to match the geometry stage's input primitive. You can cast this parameter from IRTessellatorOutputPrimitive.
 * @param gsInputPrimitive the input primitive type of the geometry stage. This value needs to match the hsTessellatorOutputPrimitive. You can cast this parameter from IRInputPrimitive.
 * @param hsOutputControlPointSize the size of the point output from the hull stage. This value needs to match the input point size of the domain stage.
 * @param dsInputControlPointSize the size of the input point to the domain stage. This value needs to match the point size output from the hull stage.
 * @param hsPatchConstantsSize the size of the constants output from the hull stage. This value needs to match the size of the input constants to the domain stage.
 * @param dsPatchConstantsSize the size of the input constants to the domain stage. This value needs to match the size of the constants output from the hull stage.
 * @param hsOutputControlPointCount the number of control points output from the hull stage. This value needs to match the number of input control points to the domain stage.
 * @param dsInputControlPointCount the number of input control points to the domain stage. This value needs to match the number of control points output from the hull stage.
 */
bool IRRuntimeValidateTessellationPipeline(IRRuntimeTessellatorOutputPrimitive hsTessellatorOutputPrimitive,
                                           IRRuntimePrimitiveType gsInputPrimitive,
                                           uint32_t hsOutputControlPointSize,
                                           uint32_t dsInputControlPointSize,
                                           uint32_t hsPatchConstantsSize,
                                           uint32_t dsPatchConstantsSize,
                                           uint32_t hsOutputControlPointCount,
                                           uint32_t dsInputControlPointCount);

/**
 * Create a new mesh pipeline suitable for emulating a render pipeline with a geometry stage.
 * @param device the device to use for creating the new pipeline.
 * @param descriptor an object describing the origin libraries and function names to create the pipeline.
 * @param error an output error object containing details about any error encountered during the creation process.
 * @return a new mesh render pipeline or nil on error.
 */
renderpipelinestate_t IRRuntimeNewGeometryEmulationPipeline(device_t device,
                                                            const IRGeometryEmulationPipelineDescriptor* descriptor,
                                                            nserror_t* error) API_AVAILABLE(macosx(14), ios(17));


/**
 * Create a new mesh pipeline suitable for emulating a render pipeline with a hull stage, a domain stage, and a geometry stage.
 * @note You may optionally not provide a geometry shader as part of your tessellation pipeline by setting the decriptor parameter's
 * geometryLibrary member to NULL, and providing a geometryFunctionName of kIRTrianglePassthroughGeometryShader, kIRLinePassthroughGeometryShader,
 * or kIRPointPassthroughGeometryShader, depending on the primitive topology of draw calls that use the pipeline.
 * @param device the device to use for creating the new pipeline.
 * @param descriptor an object describing the origin libraries and function names to create the pipeline.
 * @param error an output error object containing details about any error encountered during the creation process.
 * @return a new mesh render pipeline or nil on error.
 */
renderpipelinestate_t IRRuntimeNewGeometryTessellationEmulationPipeline(device_t device,
                                                                        const IRGeometryTessellationEmulationPipelineDescriptor* descriptor,
                                                                        nserror_t* error) API_AVAILABLE(macosx(14), ios(17));
        
/**
 * Sets a value for a function constant at a specific index.
 * @param values the target function constant values object
 * @param index the index of the function constant (must be between 0 and 65535)
 * @param value the constant value
 */
void IRRuntimeSetFunctionConstantValue(functionconstantvalues_t values, uint16_t index, IRRuntimeFunctionConstantValue *value);

#ifdef IR_PRIVATE_IMPLEMENTATION

#ifndef IR_RUNTIME_METALCPP
#if !__has_feature(objc_arc)
#error The implementation of this file needs to be generated in a module with ARC enabled when in Objective-C mode.
#endif
#endif


const uint64_t kIRArgumentBufferBindPoint                   = 2;
const uint64_t kIRArgumentBufferHullDomainBindPoint         = 3;
const uint64_t kIRDescriptorHeapBindPoint                   = 0;
const uint64_t kIRSamplerHeapBindPoint                      = 1;
const uint64_t kIRArgumentBufferDrawArgumentsBindPoint      = 4;
const uint64_t kIRArgumentBufferUniformsBindPoint           = 5;
const uint64_t kIRVertexBufferBindPoint                     = 6;
const uint64_t kIRStageInAttributeStartIndex                = 11;

const char* kIRIndirectTriangleIntersectionFunctionName     = "irconverter.wrapper.intersection.function.triangle";
const char* kIRIndirectProceduralIntersectionFunctionName   = "irconverter.wrapper.intersection.function.procedural";

const char*    kIRTrianglePassthroughGeometryShader         = "irconverter_domain_shader_triangle_passthrough";
const char*    kIRLinePassthroughGeometryShader             = "irconverter_domain_shader_line_passthrough";
const char*    kIRPointPassthroughGeometryShader            = "irconverter_domain_shader_point_passthrough";

const uint16_t kIRNonIndexedDraw                            = 0;

const char*    kIRFunctionGroupRayGeneration                = "rayGen";
const char*    kIRFunctionGroupClosestHit                   = "closestHit";
const char*    kIRFunctionGroupMiss                         = "miss";

const uint64_t kIRBufSizeOffset     = 0;
const uint64_t kIRBufSizeMask       = 0xffffffff;
const uint64_t kIRTexViewOffset     = 32;
const uint64_t kIRTexViewMask       = 0xff;
const uint64_t kIRTypedBufferOffset = 63;


IR_INLINE
void IRRuntimeCreateAppendBufferView(device_t device, buffer_t appendBuffer, uint64_t appendBufferOffset, uint32_t initialCounterValue, IRBufferView* outBufferView)
{
    if (outBufferView)
    {
        uint64_t bufferSize = 4+15; // 4 bytes aligned to 16
#ifdef IR_RUNTIME_METALCPP
        buffer_t texViewBuffer = device->newBuffer(bufferSize, MTL::ResourceStorageModeShared)->autorelease();
        uint64_t bufferOffset = texViewBuffer->gpuAddress() % 16;
        uint32_t* countPtr = (uint32_t *)(((uint8_t *)texViewBuffer->contents()) + bufferOffset);
        *countPtr = initialCounterValue;
        
        // If you modify this code to suballocate the buffer, align the buffer texture to 16 bytes
        MTL::TextureDescriptor* pTexDesc = MTL::TextureDescriptor::alloc()->init();
        pTexDesc->setPixelFormat(MTL::PixelFormatR32Uint);
        pTexDesc->setWidth(1);
        pTexDesc->setHeight(1);
        pTexDesc->setResourceOptions(texViewBuffer->resourceOptions());
        pTexDesc->setUsage(MTL::TextureUsageShaderWrite | MTL::TextureUsagePixelFormatView);
        pTexDesc->setTextureType(MTL::TextureTypeTextureBuffer);
        MTL::Texture* pTexture = texViewBuffer->newTexture(pTexDesc, 0, 4);
        
        *outBufferView = (IRBufferView){
            .buffer = appendBuffer,
            .bufferOffset = appendBufferOffset,
            .bufferSize = appendBuffer->length(),
            .textureBufferView = pTexture,
            .textureViewOffsetInElements = (uint32_t)(bufferOffset / 4),
            .typedBuffer = 0
        };
        pTexDesc->release();
#else
        buffer_t texViewBuffer = [device newBufferWithLength:bufferSize options:MTLResourceStorageModeShared];
        uint64_t bufferOffset = texViewBuffer.gpuAddress % 16;
        uint32_t* countPtr = (uint32_t *)(((uint8_t *)[texViewBuffer contents]) + bufferOffset);
        *countPtr = initialCounterValue;
        
        // If you modify this code to suballocate the buffer, align the buffer texture to 16 bytes
        MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc] init];
        texDesc.pixelFormat = MTLPixelFormatR32Uint;
        texDesc.width = 1;
        texDesc.height = 1;
        texDesc.resourceOptions = texViewBuffer.resourceOptions;
        texDesc.usage = MTLTextureUsageShaderWrite | MTLTextureUsagePixelFormatView;
        texDesc.textureType = MTLTextureTypeTextureBuffer;
        id<MTLTexture> texture = [texViewBuffer newTextureWithDescriptor:texDesc offset:0 bytesPerRow:4];
        
        *outBufferView = (IRBufferView){
            .buffer = appendBuffer,
            .bufferOffset = appendBufferOffset,
            .bufferSize = appendBuffer.length,
            .textureBufferView = texture,
            .textureViewOffsetInElements = (uint32_t)(bufferOffset / 4),
            .typedBuffer = 0
        };
#endif // IR_RUNTIME_METALCPP
    }
}

IR_INLINE
uint32_t IRRuntimeGetAppendBufferCount(const IRBufferView* bufferView)
{
    uint64_t bufferOffset = bufferView->textureViewOffsetInElements * 4;
#ifdef IR_RUNTIME_METALCPP
    uint32_t* countPtr = (uint32_t *)(((uint8_t *)bufferView->textureBufferView->buffer()->contents()) + bufferOffset);
#else
    uint32_t* countPtr = (uint32_t *)(((uint8_t *)bufferView->textureBufferView.buffer.contents) + bufferOffset);
#endif // IR_RUNTIME_METALCPP
    return *countPtr;
}


IR_INLINE
uint64_t IRDescriptorTableGetBufferMetadata(const IRBufferView* view)
{
    uint64_t md = (view->bufferSize & kIRBufSizeMask) << kIRBufSizeOffset;
    
    md |= ((uint64_t)view->textureViewOffsetInElements & kIRTexViewMask) << kIRTexViewOffset;
    
    md |= (uint64_t)view->typedBuffer << kIRTypedBufferOffset;
    
    return md;
}

IR_INLINE
void IRDescriptorTableSetBuffer(IRDescriptorTableEntry* entry, uint64_t gpu_va, uint64_t metadata)
{
    entry->gpuVA = gpu_va;
    entry->textureViewID = 0;
    entry->metadata = metadata;
}

IR_INLINE
void IRDescriptorTableSetBufferView(IRDescriptorTableEntry* entry, const IRBufferView* bufferView)
{
    #ifdef IR_RUNTIME_METALCPP
    entry->gpuVA = bufferView->buffer->gpuAddress() + bufferView->bufferOffset;
    entry->textureViewID = bufferView->textureBufferView ? bufferView->textureBufferView->gpuResourceID()._impl : 0;
    #else
    entry->gpuVA = bufferView->buffer.gpuAddress + bufferView->bufferOffset;
    entry->textureViewID = bufferView->textureBufferView ? bufferView->textureBufferView.gpuResourceID._impl : 0;
    #endif
    entry->metadata = IRDescriptorTableGetBufferMetadata(bufferView);
}

IR_INLINE
void IRDescriptorTableSetTexture(IRDescriptorTableEntry* entry, texture_t argument, float minLODClamp, uint32_t metadata)
{
    entry->gpuVA = 0;
    #ifdef IR_RUNTIME_METALCPP
    entry->textureViewID = argument->gpuResourceID()._impl;
    #else
    entry->textureViewID = argument.gpuResourceID._impl;
    #endif
    entry->metadata = (uint64_t)(*((uint32_t*)&minLODClamp) | ((uint64_t)metadata) << 32);
}

IR_INLINE
void IRDescriptorTableSetSampler(IRDescriptorTableEntry* entry, sampler_t argument, float lodBias)
{
    #ifdef IR_RUNTIME_METALCPP
    entry->gpuVA = argument->gpuResourceID()._impl;
    #else
    entry->gpuVA = argument.gpuResourceID._impl;
    #endif
    entry->textureViewID = 0;
    entry->metadata  = 0;
    
    uint32_t encodedLodBias;
    memcpy(&encodedLodBias, &lodBias, sizeof(float));
    entry->metadata = encodedLodBias;
}

static IR_INLINE
uint16_t IRMetalIndexToIRIndex(indextype_t indexType)
{
    return (uint16_t)(indexType+1);
}

IR_INLINE
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount, uint64_t instanceCount, uint64_t baseInstance) IR_OVERLOADABLE
{
    IRRuntimeDrawArgument da = { (uint32_t)vertexCount, (uint32_t)instanceCount, (uint32_t)vertexStart, (uint32_t)baseInstance };
    IRRuntimeDrawParams dp = { .draw = da };

    #ifdef IR_RUNTIME_METALCPP
    enc->setVertexBytes( &dp, sizeof( IRRuntimeDrawParams ), kIRArgumentBufferDrawArgumentsBindPoint );
    enc->setVertexBytes( &kIRNonIndexedDraw, sizeof( uint16_t ), kIRArgumentBufferUniformsBindPoint );
    enc->drawPrimitives( primitiveType, vertexStart, vertexCount, instanceCount, baseInstance );
    #else
    [enc setVertexBytes:&dp length:sizeof( IRRuntimeDrawParams ) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setVertexBytes:&kIRNonIndexedDraw length:sizeof( uint16_t ) atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc drawPrimitives:primitiveType vertexStart:vertexStart vertexCount:vertexCount instanceCount:instanceCount baseInstance:baseInstance];
    #endif
}

IR_INLINE
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount, uint64_t instanceCount) IR_OVERLOADABLE
{
    IRRuntimeDrawPrimitives( enc, primitiveType, vertexStart, vertexCount, instanceCount, 0 );
}

IR_INLINE
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t vertexStart, uint64_t vertexCount) IR_OVERLOADABLE
{
    IRRuntimeDrawPrimitives( enc, primitiveType, vertexStart, vertexCount, 1, 0 );
}

IR_INLINE
void IRRuntimeDrawPrimitives(renderencoder_t enc, primitivetype_t primitiveType, buffer_t indirectBuffer, uint64_t indirectBufferOffset) IR_OVERLOADABLE
{
    #ifdef IR_RUNTIME_METALCPP
    enc->setVertexBuffer( indirectBuffer, indirectBufferOffset, kIRArgumentBufferDrawArgumentsBindPoint );
    enc->setVertexBytes( &kIRNonIndexedDraw, sizeof( uint16_t ), kIRArgumentBufferUniformsBindPoint );
    enc->drawPrimitives( primitiveType, indirectBuffer, indirectBufferOffset );
    #else
    [enc setVertexBuffer:indirectBuffer offset:indirectBufferOffset atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setVertexBytes:&kIRNonIndexedDraw length:sizeof( uint16_t ) atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc drawPrimitives:primitiveType indirectBuffer:indirectBuffer indirectBufferOffset:indirectBufferOffset];
    #endif
}

IR_INLINE
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, uint64_t instanceCount, int64_t baseVertex, uint64_t baseInstance) IR_OVERLOADABLE
{
    IRRuntimeDrawIndexedArgument da = (IRRuntimeDrawIndexedArgument){
        .indexCountPerInstance = (uint32_t)indexCount,
        .instanceCount = (uint32_t)instanceCount,
        .startIndexLocation = (uint32_t)indexBufferOffset,
        .baseVertexLocation = (int32_t)baseVertex,
        .startInstanceLocation = (uint32_t)baseInstance
    };

    IRRuntimeDrawParams dp = { .drawIndexed = da };
    const uint16_t IRIndexType = IRMetalIndexToIRIndex(indexType);

    #ifdef IR_RUNTIME_METALCPP
    enc->setVertexBytes( &dp, sizeof( IRRuntimeDrawParams ), kIRArgumentBufferDrawArgumentsBindPoint );
    enc->setVertexBytes( &IRIndexType, sizeof( uint16_t ), kIRArgumentBufferUniformsBindPoint );
    enc->drawIndexedPrimitives( primitiveType, indexCount, indexType, indexBuffer, indexBufferOffset, instanceCount, baseVertex, baseInstance );
    #else
    [enc setVertexBytes:&dp length:sizeof( IRRuntimeDrawParams ) atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setVertexBytes:&IRIndexType length:sizeof( uint16_t ) atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc drawIndexedPrimitives:primitiveType indexCount:indexCount indexType:indexType indexBuffer:indexBuffer indexBufferOffset:indexBufferOffset instanceCount:instanceCount baseVertex:baseVertex baseInstance:baseInstance];
    #endif
}

IR_INLINE
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, uint64_t instanceCount) IR_OVERLOADABLE
{
    IRRuntimeDrawIndexedPrimitives(enc, primitiveType, indexCount, indexType, indexBuffer, indexBufferOffset, instanceCount, 0, 0);
}

IR_INLINE
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, uint64_t indexCount, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset) IR_OVERLOADABLE
{
    IRRuntimeDrawIndexedPrimitives(enc, primitiveType, indexCount, indexType, indexBuffer, indexBufferOffset, 1, 0, 0);
}

IR_INLINE
void IRRuntimeDrawIndexedPrimitives(renderencoder_t enc, primitivetype_t primitiveType, indextype_t indexType, buffer_t indexBuffer, uint64_t indexBufferOffset, buffer_t indirectBuffer, uint64_t indirectBufferOffset ) IR_OVERLOADABLE
{
    const uint16_t IRIndexType = IRMetalIndexToIRIndex(indexType);

    #ifdef IR_RUNTIME_METALCPP
    enc->setVertexBuffer( indirectBuffer, indirectBufferOffset, kIRArgumentBufferDrawArgumentsBindPoint );
    enc->setVertexBytes( &IRIndexType, sizeof( uint16_t ), kIRArgumentBufferUniformsBindPoint );
    enc->drawIndexedPrimitives( primitiveType, indexType, indexBuffer, indexBufferOffset, indirectBuffer, indirectBufferOffset );
    #else
    [enc setVertexBuffer:indirectBuffer offset:indirectBufferOffset atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setVertexBytes:&IRIndexType length:sizeof( uint16_t ) atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc drawIndexedPrimitives:primitiveType indexType:indexType indexBuffer:indexBuffer indexBufferOffset:indexBufferOffset indirectBuffer:indirectBuffer indirectBufferOffset:indirectBufferOffset];
    #endif
}

IR_INLINE
static uint32_t IRRuntimePrimitiveTypeVertexCount(IRRuntimePrimitiveType primitiveType)
{
    switch (primitiveType)
    {
        case IRRuntimePrimitiveTypePoint:           return 1; break;
        case IRRuntimePrimitiveTypeLine:            return 2; break;
        case IRRuntimePrimitiveTypeTriangle:        return 3; break;
        case IRRuntimePrimitiveTypeTriangleWithAdj: return 6; break;
        case IRRuntimePrimitiveTypeLineWithAdj:     return 4; break;
        case IRRuntimePrimitiveTypeTriangleStrip:   return 3; break;
        case IRRuntimePrimitiveTypeLineStrip:       return 2; break;
        case IRRuntimePrimitiveTypeLineStripWithAdj:return 4; break;
        default: return 0;
    }
    return 0;
}

IR_INLINE
static uint32_t IRRuntimePrimitiveTypeVertexOverlap(IRRuntimePrimitiveType primitiveType)
{
    switch (primitiveType)
    {
        case IRRuntimePrimitiveTypePoint:           return 0; break;
        case IRRuntimePrimitiveTypeLine:            return 0; break;
        case IRRuntimePrimitiveTypeTriangle:        return 0; break;
        case IRRuntimePrimitiveTypeTriangleWithAdj: return 0; break;
        case IRRuntimePrimitiveTypeLineWithAdj:     return 0; break;
        case IRRuntimePrimitiveTypeTriangleStrip:   return 2; break;
        case IRRuntimePrimitiveTypeLineStrip:       return 1; break;
        case IRRuntimePrimitiveTypeLineStripWithAdj:return 3; break;
        default: return 0;
    }
    return 0;
}

IR_INLINE
static IRRuntimeDrawInfo IRRuntimeCalculateDrawInfoForGSEmulation(IRRuntimePrimitiveType primitiveType, indextype_t indexType, uint32_t vertexSizeInBytes, uint32_t maxInputPrimitivesPerMeshThreadgroup, uint32_t instanceCount)
{
    const uint32_t primitiveVertexCount = IRRuntimePrimitiveTypeVertexCount(primitiveType);
    const uint32_t alignment = primitiveVertexCount;
    
    const uint32_t totalPayloadBytes = 16384;
    const uint32_t payloadBytesForMetadata = 32;
    const uint32_t payloadBytesForVertexData = totalPayloadBytes - payloadBytesForMetadata;
    
    const uint32_t maxVertexCountLimitedByPayloadMemory = (((payloadBytesForVertexData/vertexSizeInBytes)) / alignment) * alignment;
    
    const uint32_t maxMeshThreadgroupsPerObjectThreadgroup = 1024;
    const uint32_t maxPrimCountLimitedByAmplificationRate = maxMeshThreadgroupsPerObjectThreadgroup * maxInputPrimitivesPerMeshThreadgroup;
    uint32_t maxPrimsPerObjectThreadgroup = IR_MIN(maxVertexCountLimitedByPayloadMemory/primitiveVertexCount, maxPrimCountLimitedByAmplificationRate);
    
    const uint32_t maxThreadsPerThreadgroup = 256;
    maxPrimsPerObjectThreadgroup = IR_MIN(maxPrimsPerObjectThreadgroup, maxThreadsPerThreadgroup / primitiveVertexCount);
    
    return (IRRuntimeDrawInfo){
        .indexType                            = (uint8_t)(indexType + 1),
        
        .primitiveTopology                    = (uint8_t)primitiveType,
        .threadsPerPatch                      = (uint8_t)primitiveVertexCount,
        .maxInputPrimitivesPerMeshThreadgroup = (uint16_t)maxInputPrimitivesPerMeshThreadgroup,
        .objectThreadgroupVertexStride        = (uint16_t)(maxPrimsPerObjectThreadgroup * primitiveVertexCount),
        .meshThreadgroupPrimitiveStride          = (uint16_t)maxInputPrimitivesPerMeshThreadgroup,
        
        .gsInstanceCount                      = (uint16_t)instanceCount,
        .patchesPerObjectThreadgroup          = (uint16_t)maxPrimsPerObjectThreadgroup,
        .inputControlPointsPerPatch           = (uint8_t)primitiveVertexCount,
        
        .indexBuffer                          = 0 /* to be filled out by the caller */
    };
}

IR_INLINE
static mtlsize_t IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(uint32_t vertexOrIndexCount,
                                                                                  uint32_t objectThreadgroupVertexStride,
                                                                                  IRRuntimePrimitiveType primitiveType,
                                                                                  uint32_t instanceCount)
{
    uint32_t nonProvokingVertices = 0;
    if (primitiveType == (uint32_t)IRRuntimePrimitiveTypeLineStrip || primitiveType == (uint32_t)IRRuntimePrimitiveTypeTriangleStrip || primitiveType == (uint32_t)IRRuntimePrimitiveTypeLineStripWithAdj)
    {
        // For strips, last k vertices aren't able to spawn a full primitive.
        nonProvokingVertices = IRRuntimePrimitiveTypeVertexOverlap(primitiveType);
    }
    
    mtlsize_t siz;
    siz.width = (vertexOrIndexCount - nonProvokingVertices + objectThreadgroupVertexStride - 1) / objectThreadgroupVertexStride;
    siz.height = instanceCount;
    siz.depth = 1;
    
    return siz;
}

IR_INLINE
static void IRRuntimeCalculateThreadgroupSizeForGeometry(IRRuntimePrimitiveType primitiveType, uint32_t maxInputPrimitivesPerMeshThreadgroup, uint32_t objectThreadgroupVertexStride, uint32_t* objectThreadgroupSize, uint32_t* meshThreadgroupSize)
{
    // The payload memory limits the vertex count that the object threadgroup can process.
    *objectThreadgroupSize = objectThreadgroupVertexStride + IRRuntimePrimitiveTypeVertexOverlap(primitiveType);
    // Geometry shader limits how many primitives a single mesh threadgroup can process
    *meshThreadgroupSize = maxInputPrimitivesPerMeshThreadgroup;
}

IR_INLINE
static void IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(uint32_t patchesPerObjectThreadgroup, uint32_t maxObjectThreadsPerPatch, uint32_t maxInputPrimitivesPerMeshThreadgroup, uint32_t* objectThreadgroupSize, uint32_t* meshThreadgroupSize)
{
    // The hull shader defines how many threads to run per patch. It also limits how many patches run per object threadgroup.
    *objectThreadgroupSize = patchesPerObjectThreadgroup * maxObjectThreadsPerPatch;
    // The geometry shader limits how many primitives a single mesh threadgroup processes.
    *meshThreadgroupSize = maxInputPrimitivesPerMeshThreadgroup;
}

IR_INLINE
void IRRuntimeDrawIndexedPrimitivesGeometryEmulation(renderencoder_t enc,
                                                     IRRuntimePrimitiveType primitiveType,
                                                     indextype_t indexType,
                                                     buffer_t indexBuffer,
                                                     IRRuntimeGeometryPipelineConfig geometryPipelineConfig,
                                                     uint32_t instanceCount,
                                                     uint32_t indexCountPerInstance,
                                                     uint32_t startIndex,
                                                     int baseVertex,
                                                     uint32_t baseInstance)
{
    IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(primitiveType,
                                                                          indexType,
                                                                          geometryPipelineConfig.gsVertexSizeInBytes,
                                                                          geometryPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                                          instanceCount);
    
    mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(indexCountPerInstance,
                                                                                                          drawInfo.objectThreadgroupVertexStride,
                                                                                                          primitiveType,
                                                                                                          instanceCount);
    
    uint32_t objectThreadgroupSize,meshThreadgroupSize;
    IRRuntimeCalculateThreadgroupSizeForGeometry(primitiveType,
                                                 geometryPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                 drawInfo.objectThreadgroupVertexStride,
                                                 &objectThreadgroupSize,
                                                 &meshThreadgroupSize);
    
    IRRuntimeDrawParams drawParams;
    drawParams.drawIndexed = (IRRuntimeDrawIndexedArgument){
        .indexCountPerInstance = indexCountPerInstance,
        .instanceCount = instanceCount,
        .startIndexLocation = startIndex,
        .baseVertexLocation = baseVertex,
        .startInstanceLocation = baseInstance
    };
    
    
    #ifdef IR_RUNTIME_METALCPP
    
    drawInfo.indexBuffer = indexBuffer->gpuAddress();
    
    enc->setObjectBytes(&drawInfo,            sizeof(IRRuntimeDrawInfo),       kIRArgumentBufferUniformsBindPoint);
    enc->setMeshBytes(&drawInfo,              sizeof(IRRuntimeDrawInfo),       kIRArgumentBufferUniformsBindPoint);
    enc->setObjectBytes(&drawParams,          sizeof(IRRuntimeDrawParams),     kIRArgumentBufferDrawArgumentsBindPoint);
    enc->setMeshBytes(&drawParams,            sizeof(IRRuntimeDrawParams),     kIRArgumentBufferDrawArgumentsBindPoint);
    
    enc->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
    
    #else
    
    drawInfo.indexBuffer = indexBuffer.gpuAddress;
    
    [enc setObjectBytes:&drawInfo           length:sizeof(IRRuntimeDrawInfo)        atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setMeshBytes:&drawInfo             length:sizeof(IRRuntimeDrawInfo)        atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setObjectBytes:&drawParams         length:sizeof(IRRuntimeDrawParams)      atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setMeshBytes:&drawParams           length:sizeof(IRRuntimeDrawParams)      atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    
    [enc drawMeshThreadgroups:objectThreadgroupCount
  threadsPerObjectThreadgroup:MTLSizeMake(objectThreadgroupSize, 1, 1)
    threadsPerMeshThreadgroup:MTLSizeMake(meshThreadgroupSize, 1, 1)];
    
    #endif
}


IR_INLINE
void IRRuntimeDrawPrimitivesGeometryEmulation(renderencoder_t enc,
                                              IRRuntimePrimitiveType primitiveType,
                                              IRRuntimeGeometryPipelineConfig geometryPipelineConfig,
                                              uint32_t instanceCount,
                                              uint32_t vertexCountPerInstance,
                                              uint32_t baseVertex,
                                              uint32_t baseInstance)
{
    IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(primitiveType,
                                                                          (indextype_t)-1,
                                                                          geometryPipelineConfig.gsVertexSizeInBytes,
                                                                          geometryPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                                          instanceCount);
    drawInfo.indexType = kIRNonIndexedDraw;
    
    mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(vertexCountPerInstance,
                                                                                                          drawInfo.objectThreadgroupVertexStride,
                                                                                                          primitiveType,
                                                                                                          instanceCount);
    
    uint32_t objectThreadgroupSize,meshThreadgroupSize;
    IRRuntimeCalculateThreadgroupSizeForGeometry(primitiveType,
                                                 geometryPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                 drawInfo.objectThreadgroupVertexStride,
                                                 &objectThreadgroupSize,
                                                 &meshThreadgroupSize);
    
    IRRuntimeDrawParams drawParams;
    drawParams.draw = (IRRuntimeDrawArgument){
        .vertexCountPerInstance = vertexCountPerInstance,
        .instanceCount = instanceCount,
        .startVertexLocation = baseVertex,
        .startInstanceLocation = baseInstance
    };
    
    
    #ifdef IR_RUNTIME_METALCPP
    
    enc->setObjectBytes(&drawInfo,            sizeof(IRRuntimeDrawInfo),       kIRArgumentBufferUniformsBindPoint);
    enc->setMeshBytes(&drawInfo,              sizeof(IRRuntimeDrawInfo),       kIRArgumentBufferUniformsBindPoint);
    enc->setObjectBytes(&drawParams,          sizeof(IRRuntimeDrawParams),     kIRArgumentBufferDrawArgumentsBindPoint);
    enc->setMeshBytes(&drawParams,            sizeof(IRRuntimeDrawParams),     kIRArgumentBufferDrawArgumentsBindPoint);
    
    enc->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
    
    #else
    
    [enc setObjectBytes:&drawInfo           length:sizeof(IRRuntimeDrawInfo)        atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setMeshBytes:&drawInfo             length:sizeof(IRRuntimeDrawInfo)        atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setObjectBytes:&drawParams         length:sizeof(IRRuntimeDrawParams)      atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setMeshBytes:&drawParams           length:sizeof(IRRuntimeDrawParams)      atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    
    [enc drawMeshThreadgroups:objectThreadgroupCount
  threadsPerObjectThreadgroup:MTLSizeMake(objectThreadgroupSize, 1, 1)
    threadsPerMeshThreadgroup:MTLSizeMake(meshThreadgroupSize, 1, 1)];
    
    #endif
}

IR_INLINE
static uint16_t IRTessellatorThreadgroupVertexOverlap(IRRuntimeTessellatorOutputPrimitive tessellatorOutputPrimitive)
{
    switch (tessellatorOutputPrimitive)
    {
        case IRRuntimeTessellatorOutputPoint: return 0;
        case IRRuntimeTessellatorOutputLine:  return 1;
        default:                              return 2;
    }
    return 2;
}

IR_INLINE
static IRRuntimeDrawInfo IRRuntimeCalculateDrawInfoForGSTSEmulation(IRRuntimePrimitiveType primitiveType,
                                                                    indextype_t indexType,
                                                                    IRRuntimeTessellatorOutputPrimitive tessellatorOutputPrimitive,
                                                                    uint32_t gsMaxInputPrimitivesPerMeshThreadgroup,
                                                                    uint32_t hsPatchesPerObjectThreadgroup,
                                                                    uint32_t hsInputControlPointsPerPatch,
                                                                    uint32_t hsObjectThreadsPerPatch,
                                                                    uint32_t gsInstanceCount
                                                                    )
{
    IRRuntimeDrawInfo drawInfo;
    
    const uint16_t meshThreadgroupVertexOverlap   = IRTessellatorThreadgroupVertexOverlap(tessellatorOutputPrimitive);
    drawInfo.meshThreadgroupPrimitiveStride       = (uint16_t)gsMaxInputPrimitivesPerMeshThreadgroup - meshThreadgroupVertexOverlap;
    drawInfo.objectThreadgroupVertexStride        = (uint16_t)(hsPatchesPerObjectThreadgroup * hsInputControlPointsPerPatch);
    drawInfo.patchesPerObjectThreadgroup          = (uint16_t)hsPatchesPerObjectThreadgroup;
    drawInfo.threadsPerPatch                      = (uint8_t) hsObjectThreadsPerPatch;
    drawInfo.inputControlPointsPerPatch           = (uint16_t)hsInputControlPointsPerPatch;
    drawInfo.gsInstanceCount                      = (uint16_t)gsInstanceCount;
    drawInfo.maxInputPrimitivesPerMeshThreadgroup = (uint16_t)gsMaxInputPrimitivesPerMeshThreadgroup;
    
    drawInfo.indexType                            = (uint8_t)indexType + 1;
    drawInfo.primitiveTopology                    = primitiveType;
    drawInfo.indexBuffer                          = 0;
    
    return drawInfo;
}

IR_INLINE
void IRRuntimeDrawIndexedPatchesTessellationEmulation(renderencoder_t enc,
                                                      IRRuntimePrimitiveType primitiveTopology,
                                                      indextype_t indexType,
                                                      buffer_t indexBuffer,
                                                      IRRuntimeTessellationPipelineConfig tessellationPipelineConfig,
                                                      uint32_t instanceCount,
                                                      uint32_t indexCountPerInstance,
                                                      uint32_t baseInstance,
                                                      int32_t  baseVertex,
                                                      uint32_t startIndex)
{
    IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSTSEmulation(
                                                    /* primitiveType */                          primitiveTopology,
                                                    /* indexType */                              indexType,
                                                    /* tessellatorOutputPrimitive */             tessellationPipelineConfig.outputPrimitiveType,
                                                    /* gsMaxInputPrimitivesPerMeshThreadgroup */ tessellationPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                    /* hsPatchesPerObjectThreadgroup */          tessellationPipelineConfig.hsMaxPatchesPerObjectThreadgroup,
                                                    /* hsInputControlPointsPerPatch */           tessellationPipelineConfig.hsInputControlPointCount,
                                                    /* hsObjectThreadsPerPatch */                tessellationPipelineConfig.hsMaxObjectThreadsPerThreadgroup,
                                                    /* gsInstanceCount */                        tessellationPipelineConfig.gsInstanceCount);
    
    
    mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(indexCountPerInstance,
                                                                                                          drawInfo.objectThreadgroupVertexStride,
                                                                                                          primitiveTopology,
                                                                                                          instanceCount);
    
    uint32_t objectThreadgroupSize, meshThreadgroupSize;
    IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(tessellationPipelineConfig.hsMaxPatchesPerObjectThreadgroup,
                                                                tessellationPipelineConfig.hsMaxObjectThreadsPerThreadgroup,
                                                                tessellationPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                                &objectThreadgroupSize,
                                                                &meshThreadgroupSize);

    
    IRRuntimeDrawParams drawParams;
    drawParams.drawIndexed = (IRRuntimeDrawIndexedArgument){
        .indexCountPerInstance = indexCountPerInstance,
        .instanceCount = instanceCount,
        .startIndexLocation = startIndex,
        .baseVertexLocation = baseVertex,
        .startInstanceLocation = baseInstance
    };
    
    uint32_t threadgroupMem = 15360;
    
#ifdef IR_RUNTIME_METALCPP
    drawInfo.indexBuffer = indexBuffer->gpuAddress();
    
    enc->setObjectBytes(&drawInfo,              sizeof(IRRuntimeDrawInfo),         kIRArgumentBufferUniformsBindPoint);
    enc->setMeshBytes(&drawInfo,                sizeof(IRRuntimeDrawInfo),         kIRArgumentBufferUniformsBindPoint);
    enc->setObjectBytes(&drawParams,            sizeof(IRRuntimeDrawParams),       kIRArgumentBufferDrawArgumentsBindPoint);
    enc->setMeshBytes(&drawParams,              sizeof(IRRuntimeDrawParams),       kIRArgumentBufferDrawArgumentsBindPoint);
    
    enc->setObjectThreadgroupMemoryLength(threadgroupMem, 0);

    enc->drawMeshThreadgroups(objectThreadgroupCount,
                              MTL::Size::Make(objectThreadgroupSize, 1, 1),
                              MTL::Size::Make(meshThreadgroupSize, 1, 1));
#else
    drawInfo.indexBuffer = indexBuffer.gpuAddress;
    
    
    [enc setObjectBytes:&drawInfo                length:sizeof(IRRuntimeDrawInfo)         atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setMeshBytes:&drawInfo                  length:sizeof(IRRuntimeDrawInfo)         atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setObjectBytes:&drawParams              length:sizeof(IRRuntimeDrawParams)       atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setMeshBytes:&drawParams                length:sizeof(IRRuntimeDrawParams)       atIndex:kIRArgumentBufferDrawArgumentsBindPoint];

    [enc setObjectThreadgroupMemoryLength:threadgroupMem atIndex:0];
    
    [enc drawMeshThreadgroups:objectThreadgroupCount
  threadsPerObjectThreadgroup:MTLSizeMake(objectThreadgroupSize, 1, 1)
    threadsPerMeshThreadgroup:MTLSizeMake(meshThreadgroupSize, 1, 1)];
#endif // IR_RUNTIME_METALCPP
}

IR_INLINE
void IRRuntimeDrawPatchesTessellationEmulation(renderencoder_t enc,
                                               IRRuntimePrimitiveType primitiveTopology,
                                               IRRuntimeTessellationPipelineConfig tessellationPipelineConfig,
                                               uint32_t instanceCount,
                                               uint32_t vertexCountPerInstance,
                                               uint32_t baseInstance,
                                               uint32_t baseVertex)
{
    IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSTSEmulation(
                                                    /* primitiveType */                          primitiveTopology,
                                                    /* indexType */                              (indextype_t)-1,
                                                    /* tessellatorOutputPrimitive */             tessellationPipelineConfig.outputPrimitiveType,
                                                    /* gsMaxInputPrimitivesPerMeshThreadgroup */ tessellationPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                    /* hsPatchesPerObjectThreadgroup */          tessellationPipelineConfig.hsMaxPatchesPerObjectThreadgroup,
                                                    /* hsInputControlPointsPerPatch */           tessellationPipelineConfig.hsInputControlPointCount,
                                                    /* hsObjectThreadsPerPatch */                tessellationPipelineConfig.hsMaxObjectThreadsPerThreadgroup,
                                                    /* gsInstanceCount */                        tessellationPipelineConfig.gsInstanceCount);
    drawInfo.indexType = kIRNonIndexedDraw;
    
    
    mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(vertexCountPerInstance,
                                                                                                          drawInfo.objectThreadgroupVertexStride,
                                                                                                          primitiveTopology,
                                                                                                          instanceCount);
    
    uint32_t objectThreadgroupSize, meshThreadgroupSize;
    IRRuntimeCalculateThreadgroupSizeForTessellationAndGeometry(tessellationPipelineConfig.hsMaxPatchesPerObjectThreadgroup,
                                                                tessellationPipelineConfig.hsMaxObjectThreadsPerThreadgroup,
                                                                tessellationPipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
                                                                &objectThreadgroupSize,
                                                                &meshThreadgroupSize);

    
    IRRuntimeDrawParams drawParams;
    drawParams.draw = (IRRuntimeDrawArgument){
        .vertexCountPerInstance = vertexCountPerInstance,
        .instanceCount = instanceCount,
        .startVertexLocation = baseVertex,
        .startInstanceLocation = baseInstance
        
    };
    
    uint32_t threadgroupMem = 15360;
    
#ifdef IR_RUNTIME_METALCPP
    
    enc->setObjectBytes(&drawInfo,              sizeof(IRRuntimeDrawInfo),         kIRArgumentBufferUniformsBindPoint);
    enc->setMeshBytes(&drawInfo,                sizeof(IRRuntimeDrawInfo),         kIRArgumentBufferUniformsBindPoint);
    enc->setObjectBytes(&drawParams,            sizeof(IRRuntimeDrawParams),       kIRArgumentBufferDrawArgumentsBindPoint);
    enc->setMeshBytes(&drawParams,              sizeof(IRRuntimeDrawParams),       kIRArgumentBufferDrawArgumentsBindPoint);
    
    enc->setObjectThreadgroupMemoryLength(threadgroupMem, 0);

    enc->drawMeshThreadgroups(objectThreadgroupCount,
                              MTL::Size::Make(objectThreadgroupSize, 1, 1),
                              MTL::Size::Make(meshThreadgroupSize, 1, 1));
#else
    
    [enc setObjectBytes:&drawInfo                length:sizeof(IRRuntimeDrawInfo)         atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setMeshBytes:&drawInfo                  length:sizeof(IRRuntimeDrawInfo)         atIndex:kIRArgumentBufferUniformsBindPoint];
    [enc setObjectBytes:&drawParams              length:sizeof(IRRuntimeDrawParams)       atIndex:kIRArgumentBufferDrawArgumentsBindPoint];
    [enc setMeshBytes:&drawParams                length:sizeof(IRRuntimeDrawParams)       atIndex:kIRArgumentBufferDrawArgumentsBindPoint];

    [enc setObjectThreadgroupMemoryLength:threadgroupMem atIndex:0];
    
    [enc drawMeshThreadgroups:objectThreadgroupCount
  threadsPerObjectThreadgroup:MTLSizeMake(objectThreadgroupSize, 1, 1)
    threadsPerMeshThreadgroup:MTLSizeMake(meshThreadgroupSize, 1, 1)];
#endif // IR_RUNTIME_METALCPP
}

IR_INLINE
bool IRRuntimeValidateTessellationPipeline(IRRuntimeTessellatorOutputPrimitive hsTessellatorOutputPrimitive,
                                           IRRuntimePrimitiveType gsInputPrimitive,
                                           uint32_t hsOutputControlPointSize,
                                           uint32_t dsInputControlPointSize,
                                           uint32_t hsPatchConstantsSize,
                                           uint32_t dsPatchConstantsSize,
                                           uint32_t hsOutputControlPointCount,
                                           uint32_t dsInputControlPointCount)
{
    bool result = true;
    
    bool primitivesMatch =
    ((gsInputPrimitive == IRRuntimePrimitiveTypeTriangle) && (hsTessellatorOutputPrimitive == IRRuntimeTessellatorOutputTriangleCW || hsTessellatorOutputPrimitive == IRRuntimeTessellatorOutputTriangleCCW))
    ||  (gsInputPrimitive == IRRuntimePrimitiveTypeLine   && hsTessellatorOutputPrimitive == IRRuntimeTessellatorOutputLine)
    ||  (gsInputPrimitive == IRRuntimePrimitiveTypePoint  && hsTessellatorOutputPrimitive == IRRuntimeTessellatorOutputPoint);
    
    result &= primitivesMatch;
    
    result &= ( hsOutputControlPointSize  == dsInputControlPointSize );
    result &= ( hsPatchConstantsSize      == dsPatchConstantsSize );
    result &= ( hsOutputControlPointCount == dsInputControlPointCount);
    
    
    return result;
}

IR_INLINE
renderpipelinestate_t IRRuntimeNewGeometryEmulationPipeline(device_t device, const IRGeometryEmulationPipelineDescriptor* descriptor, nserror_t* error) API_AVAILABLE(macosx(14), ios(17))
{
#ifdef IR_RUNTIME_METALCPP
    MTL::RenderPipelineState* pRenderPipelineState = nullptr;
    MTL::FunctionConstantValues* pFunctionConstants = MTL::FunctionConstantValues::alloc()->init();
    MTL::Function* pStageInFn  = nullptr;
    MTL::Function* pVertexFn   = nullptr;
    MTL::Function* pGeometryFn = nullptr;
    MTL::Function* pFragmentFn = nullptr;
    
    // Stage-in function:
    MTL::Library* pStageInLib = descriptor->stageInLibrary;
    pStageInFn = pStageInLib->newFunction( (NS::String *)pStageInLib->functionNames()->object(0) );
    
    // Vertex function:
    
    {
        bool bEnableTessellation = false;
        pFunctionConstants->setConstantValue(&bEnableTessellation, MTL::DataTypeBool, MTLSTR("tessellationEnabled"));
        
        std::stringstream ss;
        ss << descriptor->vertexFunctionName << ".dxil_irconverter_object_shader";
        pVertexFn = descriptor->vertexLibrary->newFunction(NS::String::string(ss.str().c_str(), NS::UTF8StringEncoding),
                                                           pFunctionConstants,
                                                           error);
        if (!pVertexFn)
        {
            goto exit_vertex_function_error;
        }
    }
    
    // Geometry function:
    
    {
        pFunctionConstants->setConstantValue(&(descriptor->pipelineConfig.gsVertexSizeInBytes),
                                             MTL::DataTypeInt,
                                             MTLSTR("vertex_shader_output_size_fc"));
        
        pGeometryFn = descriptor->geometryLibrary->newFunction(NS::String::string(descriptor->geometryFunctionName, NS::UTF8StringEncoding),
                                                               pFunctionConstants,
                                                               error);
        if (!pGeometryFn)
        {
            goto exit_geometry_function_error;
        }
    }
    
    // Fragment function:
    
    pFragmentFn = descriptor->fragmentLibrary->newFunction( NS::String::string(descriptor->fragmentFunctionName, NS::UTF8StringEncoding) );
    if (!pFragmentFn)
    {
        goto exit_fragment_function_error;
    }
    
    // Assemble the mesh render pipeline descriptor and build the pipeline.
    
    {
        MTL::MeshRenderPipelineDescriptor* pDesc = descriptor->basePipelineDescriptor;
        pDesc->setObjectFunction( pVertexFn );
        pDesc->setMeshFunction( pGeometryFn );
        pDesc->setFragmentFunction( pFragmentFn );
        
        // Link stage-in function:
        NS::Array* pFunctions = (NS::Array *)CFArrayCreate( nullptr, (const void **)&pStageInFn, 1, &kCFTypeArrayCallBacks );
        MTL::LinkedFunctions* pStageInFunctions = MTL::LinkedFunctions::alloc()->init();
        
        pStageInFunctions->setFunctions( pFunctions );
        pDesc->setObjectLinkedFunctions( pStageInFunctions );
        
        pStageInFunctions->release();
        pFunctions->release();
        
        pRenderPipelineState = device->newRenderPipelineState( pDesc, MTL::PipelineOptionNone, nullptr, error );
    }
    
    pFragmentFn->release();
    
exit_fragment_function_error:
    pGeometryFn->release();
    
exit_geometry_function_error:
    pVertexFn->release();
    
exit_vertex_function_error:
    pStageInFn->release();
    pFunctionConstants->release();
    
    return pRenderPipelineState;
    
#else
    
    id<MTLRenderPipelineState> pRenderPipelineState = nil;
    MTLFunctionConstantValues* pFunctionConstants = [[MTLFunctionConstantValues alloc] init];
    id<MTLFunction> pStageInFn  = nil;
    id<MTLFunction> pVertexFn   = nil;
    id<MTLFunction> pGeometryFn = nil;
    id<MTLFunction> pFragmentFn = nil;
    
    // Stage-in function:
    id<MTLLibrary> pStageInLib = descriptor->stageInLibrary;
    pStageInFn = [pStageInLib newFunctionWithName:pStageInLib.functionNames.firstObject];
    
    // Vertex function:
    
    {
        BOOL bEnableTessellation = NO;
        
        [pFunctionConstants setConstantValue:&bEnableTessellation
                                        type:MTLDataTypeBool
                                    withName:@"tessellationEnabled"];
        
        NSString* functionName = [NSString stringWithFormat:@"%s.dxil_irconverter_object_shader", descriptor->vertexFunctionName];
        pVertexFn = [descriptor->vertexLibrary newFunctionWithName:functionName
                                                    constantValues:pFunctionConstants
                                                             error:error];
        if (!pVertexFn)
        {
            return nil;
        }
    }
    
    // Geometry function:
    
    {
        [pFunctionConstants setConstantValue:&(descriptor->pipelineConfig.gsVertexSizeInBytes)
                                        type:MTLDataTypeInt
                                    withName:@"vertex_shader_output_size_fc"];
        
        pGeometryFn = [descriptor->geometryLibrary newFunctionWithName:[NSString stringWithUTF8String:descriptor->geometryFunctionName]
                                                        constantValues:pFunctionConstants
                                                                 error:error];
        
        if (!pGeometryFn)
        {
            return nil;
        }
    }
    
    // Fragment function:
    
    pFragmentFn = [descriptor->fragmentLibrary newFunctionWithName:[NSString stringWithUTF8String:descriptor->fragmentFunctionName]];
    if (!pFragmentFn)
    {
        return nil;
    }
    
    // Assemble the mesh render pipeline descriptor and build the pipeline.
    
    {
        MTLMeshRenderPipelineDescriptor* pDesc = descriptor->basePipelineDescriptor;
        [pDesc setObjectFunction:pVertexFn];
        [pDesc setMeshFunction:pGeometryFn];
        [pDesc setFragmentFunction:pFragmentFn];
        
        // Link stage-in function:
        MTLLinkedFunctions* pStageInFunctions = [[MTLLinkedFunctions alloc] init];
        
        [pStageInFunctions setFunctions:@[pStageInFn]];
        [pDesc setObjectLinkedFunctions:pStageInFunctions];
        
        pRenderPipelineState = [device newRenderPipelineStateWithMeshDescriptor:pDesc
                                                                        options:MTLPipelineOptionNone
                                                                     reflection:nil
                                                                          error:error];
    }
    
    return pRenderPipelineState;
    
#endif // IR_RUNTIME_METALCPP
}

IR_INLINE
void IRRuntimeSetFunctionConstantValue(functionconstantvalues_t values, uint16_t index, IRRuntimeFunctionConstantValue *value)
{
#ifdef IR_RUNTIME_METALCPP
    values->setConstantValue(value, MTL::DataTypeUInt4, index);
#else
    [values setConstantValue:value type:MTLDataTypeUInt4 atIndex:index];
#endif
}

IR_INLINE
renderpipelinestate_t IRRuntimeNewGeometryTessellationEmulationPipeline(device_t device, const IRGeometryTessellationEmulationPipelineDescriptor* descriptor, nserror_t* error) API_AVAILABLE(macosx(14), ios(17))
{
#ifdef IR_RUNTIME_METALCPP
    MTL::RenderPipelineState* pRenderPipelineState   = nullptr;
    MTL::MeshRenderPipelineDescriptor* pPipelineDesc = nullptr;
    
    MTL::Function* pStageInFn  = nullptr;
    MTL::Function* pFragmentFn = nullptr;
    MTL::Function* pDomainFn   = nullptr;
    MTL::Function* pVertexFn   = nullptr;
    MTL::Function* pHullFn     = nullptr;
    MTL::Function* pTessFn     = nullptr;
    MTL::Function* pGeometryFn = nullptr;
    
    // Stage-in function:
    
    pStageInFn = descriptor->stageInLibrary->newFunction(MTLSTR("irconverter_stage_in_shader"));
    if (!pStageInFn)
    {
        goto exit_stagein_function_error;
    }
    
    pFragmentFn = descriptor->fragmentLibrary->newFunction(NS::String::string(descriptor->fragmentFunctionName, NS::UTF8StringEncoding));
    if (!pFragmentFn)
    {
        goto exit_fragment_function_error;
    }
    
    pDomainFn = descriptor->domainLibrary->newFunction(MTLSTR("irconverter_dxil_domain_shader"));
    if (!pDomainFn)
    {
        goto exit_domain_function_error;
    }
    
    // Vertex function:
    
    {
        // Specify that the function needs to support tessellation, because the pipeline has a tessellation stage.
        
        bool enableTessellationSupport = true;
        MTL::FunctionConstantValues* pFunctionConstants = MTL::FunctionConstantValues::alloc()->init();
        pFunctionConstants->setConstantValue(&enableTessellationSupport, MTL::DataTypeBool, MTLSTR("tessellationEnabled"));
        
        MTL::FunctionDescriptor* pFunctionDesc = MTL::FunctionDescriptor::alloc()->init();
        pFunctionDesc->setConstantValues(pFunctionConstants);
        
        std::stringstream ss;
        ss << descriptor->vertexFunctionName << ".dxil_irconverter_object_shader";
        pFunctionDesc->setName(NS::String::string(ss.str().c_str(), NS::UTF8StringEncoding));
        
        pVertexFn = descriptor->vertexLibrary->newFunction(pFunctionDesc, error);
        
        pFunctionDesc->release();
        pFunctionConstants->release();
        
        if (!pVertexFn)
        {
            goto exit_vertex_function_error;
        }
        
    }
    
    // IRConverter compiles the hull stage into two functions:
    // * A hull function containing both the control point phase and the patch constant phase
    // * A tessellator function
    {
        // Obtain reflection information for the vertex and hull stages to configure
        // the Metal function.
        
        MTL::FunctionConstantValues* pFunctionConstants = MTL::FunctionConstantValues::alloc()->init();
        
        pFunctionConstants->setConstantValue(&(descriptor->pipelineConfig.vsOutputSizeInBytes),
                                             MTL::DataTypeInt, MTLSTR("vertex_shader_output_size_fc"));
        
        pFunctionConstants->setConstantValue(&(descriptor->pipelineConfig.hsMaxTessellationFactor),
                                             MTL::DataTypeFloat, MTLSTR("max_tessellation_factor_fc"));
        
        // Retrieve both functions.
        
        MTL::FunctionDescriptor* pFunctionDesc = MTL::FunctionDescriptor::alloc()->init();
        pFunctionDesc->setConstantValues(pFunctionConstants);
        
        pFunctionDesc->setName(MTLSTR("irconverter_hull_shader"));
        pHullFn = descriptor->hullLibrary->newFunction(pFunctionDesc, error);
        if (!pHullFn)
        {
            pFunctionConstants->release();
            pFunctionDesc->release();
            goto exit_hull_function_error;
        }
        
        pFunctionDesc->setName(MTLSTR("irconverter_tessellator"));
        pTessFn = descriptor->hullLibrary->newFunction(pFunctionDesc, error);
        if (!pTessFn)
        {
            pFunctionConstants->release();
            pFunctionDesc->release();
            goto exit_tess_function_error;
        }
        
        pFunctionDesc->release();
        pFunctionConstants->release();
    }
    
    // Geometry function:
    
    {
        // Configure function:
        if (descriptor->geometryLibrary != nullptr)
        {
            bool enableTessellationEmulation = true;
            bool enableStreamOut = false;
            
            MTL::FunctionConstantValues* pFunctionConstants = MTL::FunctionConstantValues::alloc()->init();
            
            pFunctionConstants->setConstantValue(&enableTessellationEmulation,
                                                 MTL::DataTypeBool, MTLSTR("tessellationEnabled"));
            
            pFunctionConstants->setConstantValue(&enableStreamOut,
                                                 MTL::DataTypeBool, MTLSTR("streamOutEnabled"));
            
            pFunctionConstants->setConstantValue(&(descriptor->pipelineConfig.vsOutputSizeInBytes),
                                                 MTL::DataTypeInt, MTLSTR("vertex_shader_output_size_fc"));
            
            MTL::FunctionDescriptor* pFunctionDesc = MTL::FunctionDescriptor::alloc()->init();
            pFunctionDesc->setConstantValues(pFunctionConstants);
            pFunctionDesc->setName( NS::String::string(descriptor->geometryFunctionName, NS::UTF8StringEncoding) );
            
            pGeometryFn = descriptor->geometryLibrary->newFunction(pFunctionDesc, error);
            
            pFunctionDesc->release();
            pFunctionConstants->release();
        }
        else
        {
            NS::String* passthroughName = NS::String::string(descriptor->geometryFunctionName, NS::UTF8StringEncoding);
            pGeometryFn = descriptor->domainLibrary->newFunction(passthroughName);
        }
        if (!pGeometryFn)
        {
            goto exit_geometry_function_error;
        }
    }
    
    // Configure and build the mesh render pipeline.
    {
        pPipelineDesc = descriptor->basePipelineDescriptor;
        
        // Emulate the vertex stage in the object function.
        pPipelineDesc->setObjectFunction(pVertexFn);
        
        // Emulate the geometry stage in the mesh function.
        pPipelineDesc->setMeshFunction(pGeometryFn);
        
        // The fragment stage remains as the fragment function.
        pPipelineDesc->setFragmentFunction(pFragmentFn);
        
        // Link the stage-in function and hull stage as functions visible to the object stage.
        MTL::Function* objectLinkedStages[] = { pStageInFn, pHullFn };
        NS::Array* pObjectLinkedStagesArray = (NS::Array*)CFArrayCreate(CFAllocatorGetDefault(),
                                                                        (const void **)objectLinkedStages,
                                                                        2, &kCFTypeArrayCallBacks);
        
        MTL::LinkedFunctions* pObjectLinkedFunctions = MTL::LinkedFunctions::alloc()->init();
        pObjectLinkedFunctions->setFunctions(pObjectLinkedStagesArray);
        
        
        // Requires macOS 14:
        pPipelineDesc->setObjectLinkedFunctions(pObjectLinkedFunctions);
        
        
        // Link the tessellator function and the domain stage as functions visible to the mesh stage.
        MTL::Function* meshLinkedStages[] = { pTessFn, pDomainFn };
        NS::Array* pMeshLinkedStagesArray = (NS::Array*)CFArrayCreate(CFAllocatorGetDefault(),
                                                                      (const void **)meshLinkedStages,
                                                                      2, &kCFTypeArrayCallBacks);
        
        MTL::LinkedFunctions* pMeshLinkedFunctions = MTL::LinkedFunctions::alloc()->init();
        pMeshLinkedFunctions->setFunctions(pMeshLinkedStagesArray);
        
        // Requires macOS 14:
        pPipelineDesc->setMeshLinkedFunctions(pMeshLinkedFunctions);
        
        
        // Build the pipeline.
        pRenderPipelineState = device->newRenderPipelineState(pPipelineDesc, MTL::PipelineOptionNone, nullptr, error);
        
        pObjectLinkedFunctions->release();
        pObjectLinkedStagesArray->release();
        pMeshLinkedFunctions->release();
        pMeshLinkedStagesArray->release();
        
        if (!pRenderPipelineState)
        {
            goto exit_pipeline_state_error;
        }
    }
    
    // Release resources:
exit_pipeline_state_error:
    pGeometryFn->release();

exit_geometry_function_error:
    pTessFn->release();
    
exit_tess_function_error:
    pHullFn->release();
    
exit_hull_function_error:
    pVertexFn->release();
    
exit_vertex_function_error:
    pDomainFn->release();
    
exit_domain_function_error:
    pFragmentFn->release();
    
exit_fragment_function_error:
    pStageInFn->release();
    
exit_stagein_function_error:
    return pRenderPipelineState;
    
#else
    
    id<MTLRenderPipelineState> pRenderPipelineState   = nil;
    MTLMeshRenderPipelineDescriptor* pPipelineDesc    = nil;
    
    id<MTLFunction> pStageInFn  = nil;
    id<MTLFunction> pFragmentFn = nil;
    id<MTLFunction> pDomainFn   = nil;
    id<MTLFunction> pVertexFn   = nil;
    id<MTLFunction> pHullFn     = nil;
    id<MTLFunction> pTessFn     = nil;
    id<MTLFunction> pGeometryFn = nil;
    
    // Stage-in function:
    
    pStageInFn = [descriptor->stageInLibrary newFunctionWithName:@"irconverter_stage_in_shader"];
    if (!pStageInFn)
    {
        return nil;
    }
    
    pFragmentFn = [descriptor->fragmentLibrary newFunctionWithName:[NSString stringWithUTF8String:descriptor->fragmentFunctionName]];
    if (!pFragmentFn)
    {
        return nil;
    }
    
    pDomainFn = [descriptor->domainLibrary newFunctionWithName:@"irconverter_dxil_domain_shader"];
    if (!pDomainFn)
    {
        return nil;
    }
    
    // Vertex function:
    
    {
        // Specify that the function needs to support tessellation, because the pipeline has a tessellation stage.
        
        bool enableTessellationSupport = true;
        MTLFunctionConstantValues* pFunctionConstants = [[MTLFunctionConstantValues alloc] init];
        [pFunctionConstants setConstantValue:&enableTessellationSupport
                                        type:MTLDataTypeBool
                                    withName:@"tessellationEnabled"];
        
        MTLFunctionDescriptor* pFunctionDesc = [[MTLFunctionDescriptor alloc] init];
        [pFunctionDesc setConstantValues:pFunctionConstants];
        pFunctionDesc.name = [NSString stringWithFormat:@"%s.dxil_irconverter_object_shader", descriptor->vertexFunctionName];
        
        pVertexFn = [descriptor->vertexLibrary newFunctionWithDescriptor:pFunctionDesc error:error];
        
        if (!pVertexFn)
        {
            return nil;
        }
        
    }
    
    // IRConverter compiles the hull stage into two functions:
    // * A hull function containing both the control point phase and the"patch constant phase
    // * A tessellator function
    {
        // Obtain reflection information for the vertex and hull stages to configure
        // the Metal function.
        
        MTLFunctionConstantValues* pFunctionConstants = [[MTLFunctionConstantValues alloc] init];
        
        [pFunctionConstants setConstantValue:&(descriptor->pipelineConfig.vsOutputSizeInBytes)
                                        type:MTLDataTypeInt
                                    withName:@"vertex_shader_output_size_fc"];
        
        [pFunctionConstants setConstantValue:&(descriptor->pipelineConfig.hsMaxTessellationFactor)
                                        type:MTLDataTypeFloat
                                    withName:@"max_tessellation_factor_fc"];
        
        // Retrieve both functions.
        
        MTLFunctionDescriptor* pFunctionDesc = [[MTLFunctionDescriptor alloc] init];
        [pFunctionDesc setConstantValues:pFunctionConstants];
        
        [pFunctionDesc setName:@"irconverter_hull_shader"];
        pHullFn = [descriptor->hullLibrary newFunctionWithDescriptor:pFunctionDesc error:error];
        if (!pHullFn)
        {
            return nil;
        }
        
        [pFunctionDesc setName:@"irconverter_tessellator"];
        pTessFn = [descriptor->hullLibrary newFunctionWithDescriptor:pFunctionDesc error:error];
        if (!pTessFn)
        {
            return nil;
        }
    }
    
    // Geometry function:
    
    {
        if (descriptor->geometryLibrary != nil)
        {
            // Configure function:
            bool enableTessellationEmulation = true;
            bool enableStreamOut = false;
            
            MTLFunctionConstantValues* pFunctionConstants = [[MTLFunctionConstantValues alloc] init];
            
            [pFunctionConstants setConstantValue:&enableTessellationEmulation
                                            type:MTLDataTypeBool
                                        withName:@"tessellationEnabled"];
            
            [pFunctionConstants setConstantValue:&enableStreamOut
                                            type:MTLDataTypeBool
                                        withName:@"streamOutEnabled"];
            
            [pFunctionConstants setConstantValue:&(descriptor->pipelineConfig.vsOutputSizeInBytes)
                                            type:MTLDataTypeInt
                                        withName:@"vertex_shader_output_size_fc"];
            
            MTLFunctionDescriptor* pFunctionDesc = [[MTLFunctionDescriptor alloc] init];
            [pFunctionDesc setConstantValues:pFunctionConstants];
            [pFunctionDesc setName:[NSString stringWithUTF8String:descriptor->geometryFunctionName]];
            
            pGeometryFn = [descriptor->geometryLibrary newFunctionWithDescriptor:pFunctionDesc error:error];
        }
        else
        {
            NSString* passthroughName = [NSString stringWithUTF8String:descriptor->geometryFunctionName];
            pGeometryFn = [descriptor->domainLibrary newFunctionWithName:passthroughName];
        }
        if (!pGeometryFn)
        {
            return nil;
        }
    }
    
    // Configure and build the mesh render pipeline.
    {
        pPipelineDesc = descriptor->basePipelineDescriptor;
        
        // Emulate the vertex stage in the object function.
        [pPipelineDesc setObjectFunction:pVertexFn];
        
        // Emulate the geometry stage in the mesh function.
        [pPipelineDesc setMeshFunction:pGeometryFn];
        
        // The fragment stage remains as the fragment function.
        [pPipelineDesc setFragmentFunction:pFragmentFn];
        
        // Link the stage-in function and hull stage as functions visible to the object stage.
        MTLLinkedFunctions* pObjectLinkedFunctions = [[MTLLinkedFunctions alloc] init];
        [pObjectLinkedFunctions setFunctions:@[pStageInFn, pHullFn]];
        
        // Requires macOS 14:
        [pPipelineDesc setObjectLinkedFunctions:pObjectLinkedFunctions];
        
        
        // Link the tessellator function and the domain stage as functions visible to the mesh stage.
        MTLLinkedFunctions* pMeshLinkedFunctions = [[MTLLinkedFunctions alloc] init];
        [pMeshLinkedFunctions setFunctions:@[pTessFn, pDomainFn]];
        
        // Requires macOS 14:
        [pPipelineDesc setMeshLinkedFunctions:pMeshLinkedFunctions];
        
        
        // Build the pipeline.
        pRenderPipelineState = [device newRenderPipelineStateWithMeshDescriptor:pPipelineDesc
                                                                        options:MTLPipelineOptionNone
                                                                     reflection:nil
                                                                          error:error];
    }
    
    return pRenderPipelineState;
    
#endif // IR_RUNTIME_METALCPP
}


#endif // IR_PRIVATE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#undef IR_MIN
#undef IR_INLINE
#undef IR_OVERLOADABLE
