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

#ifndef IRTESSELLATORPARTITIONING_H
#define IRTESSELLATORPARTITIONING_H

typedef enum IRTessellatorPartitioning
{
    IRTessellatorPartitioningUndefined      = 0,
    IRTessellatorPartitioningInteger        = 1,
    IRTessellatorPartitioningPow2           = 2,
    IRTessellatorPartitioningFractionalOdd  = 3,
    IRTessellatorPartitioningFractionalEven = 4,
} IRTessellatorPartitioning;

#endif // IRTESSELLATORPARTITIONING_H
