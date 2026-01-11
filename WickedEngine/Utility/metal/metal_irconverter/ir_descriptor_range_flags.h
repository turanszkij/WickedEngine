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

#ifndef IRDESCRIPTORRANGEFLAGS_H
#define IRDESCRIPTORRANGEFLAGS_H

typedef enum IRDescriptorRangeFlags
{
    IRDescriptorRangeFlagNone                                       = 0,
    IRDescriptorRangeFlagDescriptorsVolatile                        = 0x1,
    IRDescriptorRangeFlagDataVolatile                               = 0x2,
    IRDescriptorRangeFlagDataStaticWhileSetAtExecute                = 0x4,
    IRDescriptorRangeFlagDataStatic                                 = 0x8,
    IRDescriptorRangeFlagDescriptorsStaticKeepingBufferBoundsChecks = 0x10000,
} IRDescriptorRangeFlags;

#endif // IRDESCRIPTORRANGEFLAGS_H
