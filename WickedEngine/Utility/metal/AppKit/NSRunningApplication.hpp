/*
 *
 * Copyright 2020-2021 Apple Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// AppKit/NSRunningApplication.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

#include <Foundation/NSObject.hpp>
#include <Foundation/NSPrivate.hpp>
#include "AppKitPrivate.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace NS
{
	class RunningApplication : NS::Referencing< RunningApplication >
	{
		public:
			static RunningApplication*	currentApplication();
			String*						localizedName() const;
	};
}


_NS_INLINE NS::RunningApplication* NS::RunningApplication::currentApplication()
{
	return Object::sendMessage< NS::RunningApplication* >( _APPKIT_PRIVATE_CLS( NSRunningApplication ), _APPKIT_PRIVATE_SEL( currentApplication ) );
}

_NS_INLINE NS::String* NS::RunningApplication::localizedName() const
{
	return Object::sendMessage< NS::String* >( this, _APPKIT_PRIVATE_SEL( localizedName ) );
}
