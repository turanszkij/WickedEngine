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
// AppKit/NSWindow.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "AppKitPrivate.hpp"
#include "NSView.hpp"
#include <Foundation/NSObject.hpp>

#include <CoreGraphics/CGGeometry.h>


namespace NS
{
	class Window : public Referencing< Window >
	{
		public:
			static Window*		alloc();
			Window*				init( CGRect contentRect, WindowStyleMask styleMask, BackingStoreType backing, bool defer );

			void				setContentView( const View* pContentView );
			void				makeKeyAndOrderFront( const Object* pSender );
			void				setTitle( const String* pTitle );

			void				close();
	};

}


_NS_INLINE NS::Window* NS::Window::alloc()
{
	return Object::sendMessage< Window* >( _APPKIT_PRIVATE_CLS( NSWindow ), _NS_PRIVATE_SEL( alloc ) );
}

_NS_INLINE NS::Window* NS::Window::init( CGRect contentRect, WindowStyleMask styleMask, BackingStoreType backing, bool defer )
{
	return Object::sendMessage< Window* >( this, _APPKIT_PRIVATE_SEL( initWithContentRect_styleMask_backing_defer_ ), contentRect, styleMask, backing, defer );
}

_NS_INLINE void NS::Window::setContentView( const NS::View* pContentView )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setContentView_ ), pContentView );
}

_NS_INLINE void NS::Window::makeKeyAndOrderFront( const Object* pSender )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( makeKeyAndOrderFront_ ), pSender );
}

_NS_INLINE void NS::Window::setTitle( const String* pTitle )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setTitle_), pTitle );
}

_NS_INLINE void NS::Window::close()
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( close ) );
}
