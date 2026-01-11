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

#ifndef IRFILTER_H
#define IRFILTER_H

typedef enum IRFilter
{
    IRFilterMinMagMipPoint                       = 0,
    IRFilterMinMagPointMipLinear                 = 0x1,
    IRFilterMinPointMagLinearMipPoint            = 0x4,
    IRFilterMinPointMagMipLinear                 = 0x5,
    IRFilterMinLinearMagMipPoint                 = 0x10,
    IRFilterMinLinearMagPointMipLinear           = 0x11,
    IRFilterMinMagLinearMipPoint                 = 0x14,
    IRFilterMinMagMipLinear                      = 0x15,
    IRFilterAnisotropic                          = 0x55,
    IRFilterComparisonMinMagMipPoint             = 0x80,
    IRFilterComparisonMinMagPointMipLinear       = 0x81,
    IRFilterComparisonMinPointMagLinearMipPoint  = 0x84,
    IRFilterComparisonMinPointMagMipLinear       = 0x85,
    IRFilterComparisonMinLinearMagMipPoint       = 0x90,
    IRFilterComparisonMinLinearMagPointMipLinear = 0x91,
    IRFilterComparisonMinMagLinearMipPoint       = 0x94,
    IRFilterComparisonMinMagMipLinear            = 0x95,
    IRFilterComparisonAnisotropic                = 0xd5,
    IRFilterMinimumMinMagMipPoint                = 0x100,
    IRFilterMinimumMinMagPointMipLinear          = 0x101,
    IRFilterMinimumMinPointMagLinearMipPoint     = 0x104,
    IRFilterMinimumMinPointMagMipLinear          = 0x105,
    IRFilterMinimumMinLinearMagMipPoint          = 0x110,
    IRFilterMinimumMinLinearMagPointMipLinear    = 0x111,
    IRFilterMinimumMinMagLinearMipPoint          = 0x114,
    IRFilterMinimumMinMagMipLinear               = 0x115,
    IRFilterMinimumAnisotropic                   = 0x155,
    IRFilterMaximumMinMagMipPoint                = 0x180,
    IRFilterMaximumMinMagPointMipLinear          = 0x181,
    IRFilterMaximumMinPointMagLinearMipPoint     = 0x184,
    IRFilterMaximumMinPointMagMipLinear          = 0x185,
    IRFilterMaximumMinLinearMagMipPoint          = 0x190,
    IRFilterMaximumMinLinearMagPointMipLinear    = 0x191,
    IRFilterMaximumMinMagLinearMipPoint          = 0x194,
    IRFilterMaximumMinMagMipLinear               = 0x195,
    IRFilterMaximumAnisotropic                   = 0x1d5,
} IRFilter;

#endif // IRFILTER_H
