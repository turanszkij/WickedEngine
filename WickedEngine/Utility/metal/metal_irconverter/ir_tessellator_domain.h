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

#ifndef IRTESSELLATORDOMAIN_H
#define IRTESSELLATORDOMAIN_H

typedef enum IRTessellatorDomain
{
    IRTessellatorDomainUndefined = 0,
    IRTessellatorDomainIsoline   = 1,
    IRTessellatorDomainTri       = 2,
    IRTessellatorDomainQuad      = 3,
} IRTessellatorDomain;

#endif // IRTESSELLATORDOMAIN_H
