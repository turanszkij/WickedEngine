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
// AppKit/NSMenu.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Foundation/NSPrivate.hpp>
#include "AppKitPrivate.hpp"

namespace NS
{
	class MenuItem;

	class Menu : public Referencing< Menu >
	{
		public:
			static Menu*	alloc();
			Menu*			init();
			Menu*			init( const String* pTitle );

			MenuItem*		addItem( const String* pTitle, SEL pSelector, const String* pKeyEquivalent );
			void			addItem( const MenuItem* pItem );
	};
}


_NS_INLINE NS::Menu* NS::Menu::alloc()
{
	return Object::alloc< Menu >( _NS_PRIVATE_CLS( NSMenu ) );
}

_NS_INLINE NS::Menu* NS::Menu::init()
{
	return Object::sendMessage< Menu* >( this, _NS_PRIVATE_SEL( init ) );
}

_NS_INLINE NS::Menu* NS::Menu::init( const String* pTitle )
{
	return Object::sendMessage< Menu* >( this, _NS_PRIVATE_SEL( initWithTitle_ ), pTitle );
}

_NS_INLINE NS::MenuItem* NS::Menu::addItem( const String* pTitle, SEL pSelector, const String* pKeyEquivalent )
{
	return Object::sendMessage< MenuItem* >( this, _NS_PRIVATE_SEL( addItemWithTitle_action_keyEquivalent_ ), pTitle, pSelector, pKeyEquivalent );
}

_NS_INLINE void NS::Menu::addItem( const MenuItem* pItem )
{
	Object::sendMessage< void >( this, _NS_PRIVATE_SEL( addItem_ ), pItem );
}
