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

#if defined(MTL_FRAMEBUFFER_FETCH_SPACE) || defined(MTL_FUNCTION_CONSTANT_SPACE)
#define __MTL_CONCAT2(a,b) a##b
#define __MTL_CONCAT3(a,b,c) a##b##c
#define __MTL_DECLARE_SPACE(value) __MTL_CONCAT2(space,value)
#endif

#if defined(MTL_FRAMEBUFFER_FETCH_SPACE)
#define MTL_DECLARE_FRAMEBUFFER(reg, type, spaceN) \
    Texture2D<type> __MTL_CONCAT3(mtl_framebuffer_,reg,type) : register(__MTL_CONCAT2(t,reg), __MTL_DECLARE_SPACE(spaceN));
    
#define MTL_DECLARE_FRAMEBUFFERS(type, space) \
    MTL_DECLARE_FRAMEBUFFER(0, type, space) \
    MTL_DECLARE_FRAMEBUFFER(1, type, space) \
    MTL_DECLARE_FRAMEBUFFER(2, type, space) \
    MTL_DECLARE_FRAMEBUFFER(3, type, space) \
    MTL_DECLARE_FRAMEBUFFER(4, type, space) \
    MTL_DECLARE_FRAMEBUFFER(5, type, space) \
    MTL_DECLARE_FRAMEBUFFER(6, type, space) \
    MTL_DECLARE_FRAMEBUFFER(7, type, space)
    
MTL_DECLARE_FRAMEBUFFERS(int4, MTL_FRAMEBUFFER_FETCH_SPACE)
MTL_DECLARE_FRAMEBUFFERS(float4, MTL_FRAMEBUFFER_FETCH_SPACE)
MTL_DECLARE_FRAMEBUFFERS(half4, MTL_FRAMEBUFFER_FETCH_SPACE)
MTL_DECLARE_FRAMEBUFFERS(uint4, MTL_FRAMEBUFFER_FETCH_SPACE)

#define MTL_LOAD_FRAMEBUFFER(reg, type) \
    __MTL_CONCAT3(mtl_framebuffer_,reg,type).Load(int3(0,0,0))
#endif
    
#if defined(MTL_FUNCTION_CONSTANT_SPACE)
#define MTL_FUNCTION_CONSTANT(type, name, reg) \
    cbuffer name : register(__MTL_CONCAT2(b,reg), __MTL_DECLARE_SPACE(MTL_FUNCTION_CONSTANT_SPACE)) { type name; }
#endif

