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

#ifndef IRCOMPARISONFUNCTION_H
#define IRCOMPARISONFUNCTION_H

typedef enum IRComparisonFunction
{
    IRComparisonFunctionNever        = 1,
    IRComparisonFunctionLess         = 2,
    IRComparisonFunctionEqual        = 3,
    IRComparisonFunctionLessEqual    = 4,
    IRComparisonFunctionGreater      = 5,
    IRComparisonFunctionNotEqual     = 6,
    IRComparisonFunctionGreaterEqual = 7,
    IRComparisonFunctionAlways       = 8,
} IRComparisonFunction;

#endif // IRCOMPARISONFUNCTION_H
