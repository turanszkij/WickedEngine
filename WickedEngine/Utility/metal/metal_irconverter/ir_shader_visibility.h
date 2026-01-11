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

#ifndef IRSHADERVISIBILITY_H
#define IRSHADERVISIBILITY_H

typedef enum IRShaderVisibility
{
    IRShaderVisibilityAll           = 0,
    IRShaderVisibilityVertex        = 1,
    IRShaderVisibilityHull          = 2,
    IRShaderVisibilityDomain        = 3,
    IRShaderVisibilityGeometry      = 4,
    IRShaderVisibilityPixel         = 5,
    IRShaderVisibilityAmplification = 6,
    IRShaderVisibilityMesh          = 7,
} IRShaderVisibility;

#endif // IRSHADERVISIBILITY_H
