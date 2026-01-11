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

#ifndef IRVERSION_H
#define IRVERSION_H

#define IR_VERSION_MAJOR 3
#define IR_VERSION_MINOR 0
#define IR_VERSION_PATCH 6

#define IR_SUPPORTS_VERSION(major, minor, patch) \
    ((major < IR_VERSION_MAJOR) || \
    (major == IR_VERSION_MAJOR && minor < IR_VERSION_MINOR) || \
    (major == IR_VERSION_MAJOR && minor == IR_VERSION_MINOR && patch <= IR_VERSION_PATCH))

#endif // IRVERSION_H
