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
// AppKit/NSApplication.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Foundation/Foundation.hpp>
#include "AppKitPrivate.hpp"

namespace NS
{
	_NS_OPTIONS( NS::UInteger, WindowStyleMask )
	{
		WindowStyleMaskBorderless	   = 0,
		WindowStyleMaskTitled		   = ( 1 << 0 ),
		WindowStyleMaskClosable		 = ( 1 << 1 ),
		WindowStyleMaskMiniaturizable   = ( 1 << 2 ),
		WindowStyleMaskResizable		= ( 1 << 3 ),
		WindowStyleMaskTexturedBackground = ( 1 << 8 ),
		WindowStyleMaskUnifiedTitleAndToolbar = ( 1 << 12 ),
		WindowStyleMaskFullScreen	   = ( 1 << 14 ),
		WindowStyleMaskFullSizeContentView = ( 1 << 15 ),
		WindowStyleMaskUtilityWindow	= ( 1 << 4 ),
		WindowStyleMaskDocModalWindow   = ( 1 << 6 ),
		WindowStyleMaskNonactivatingPanel   = ( 1 << 7 ),
		WindowStyleMaskHUDWindow		= ( 1 << 13 )
	};

	_NS_ENUM( NS::UInteger, BackingStoreType )
	{
		BackingStoreRetained = 0,
		BackingStoreNonretained = 1,
		BackingStoreBuffered = 2
	};

	_NS_ENUM( NS::UInteger, ActivationPolicy )
	{
		ActivationPolicyRegular,
		ActivationPolicyAccessory,
		ActivationPolicyProhibited
	};

	class ApplicationDelegate
	{
		public:
			virtual					~ApplicationDelegate() { }
			virtual void			applicationWillFinishLaunching( Notification* pNotification ) { }
			virtual void			applicationDidFinishLaunching( Notification* pNotification ) { }
			virtual bool			applicationShouldTerminateAfterLastWindowClosed( class Application* pSender ) { return false; }
	};

	class Application : public NS::Referencing< Application >
	{
		public:
			static Application*		sharedApplication();

			void 					setDelegate( const ApplicationDelegate* pDelegate );

			bool					setActivationPolicy( ActivationPolicy activationPolicy );

			void					activateIgnoringOtherApps( bool ignoreOtherApps );

			void					setMainMenu( const class Menu* pMenu );

			NS::Array*				windows() const;

			void					run();

			void					terminate( const Object* pSender );
	};

}

_NS_INLINE NS::Application* NS::Application::sharedApplication()
{
	return Object::sendMessage< Application* >( _APPKIT_PRIVATE_CLS( NSApplication ), _APPKIT_PRIVATE_SEL( sharedApplication ) );
}

_NS_INLINE void NS::Application::setDelegate( const ApplicationDelegate* pAppDelegate )
{
	// TODO: Use a more suitable Object instead of NS::Value?
	// NOTE: this pWrapper is only held with a weak reference
	NS::Value* pWrapper = NS::Value::value( pAppDelegate );

	typedef void (*DispatchFunction)( NS::Value*, SEL, void* );
	
	DispatchFunction willFinishLaunching = []( Value* pSelf, SEL, void* pNotification ){
		auto pDel = reinterpret_cast< NS::ApplicationDelegate* >( pSelf->pointerValue() );
		pDel->applicationWillFinishLaunching( (NS::Notification *)pNotification );
	};

	DispatchFunction didFinishLaunching = []( Value* pSelf, SEL, void* pNotification ){
		auto pDel = reinterpret_cast< NS::ApplicationDelegate* >( pSelf->pointerValue() );
		pDel->applicationDidFinishLaunching( (NS::Notification *)pNotification );
	};

	DispatchFunction shouldTerminateAfterLastWindowClosed = []( Value* pSelf, SEL, void* pApplication ){
		auto pDel = reinterpret_cast< NS::ApplicationDelegate* >( pSelf->pointerValue() );
		pDel->applicationShouldTerminateAfterLastWindowClosed( (NS::Application *)pApplication );
	};

	class_addMethod( (Class)_NS_PRIVATE_CLS( NSValue ), _APPKIT_PRIVATE_SEL( applicationWillFinishLaunching_ ), (IMP)willFinishLaunching, "v@:@" );
	class_addMethod( (Class)_NS_PRIVATE_CLS( NSValue ), _APPKIT_PRIVATE_SEL( applicationDidFinishLaunching_ ), (IMP)didFinishLaunching, "v@:@" );
	class_addMethod( (Class)_NS_PRIVATE_CLS( NSValue ), _APPKIT_PRIVATE_SEL( applicationShouldTerminateAfterLastWindowClosed_), (IMP)shouldTerminateAfterLastWindowClosed, "B@:@" );

	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setDelegate_ ), pWrapper );
}

_NS_INLINE bool NS::Application::setActivationPolicy( ActivationPolicy activationPolicy )
{
	return NS::Object::sendMessage< bool >( this, _APPKIT_PRIVATE_SEL( setActivationPolicy_ ), activationPolicy );
}

_NS_INLINE void NS::Application::activateIgnoringOtherApps( bool ignoreOtherApps )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( activateIgnoringOtherApps_ ), (ignoreOtherApps ? YES : NO) );
}

_NS_INLINE void NS::Application::setMainMenu( const class Menu* pMenu )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( setMainMenu_ ), pMenu );
}

_NS_INLINE NS::Array* NS::Application::windows() const
{
	return Object::sendMessage< NS::Array* >( this, _APPKIT_PRIVATE_SEL( windows ) );
}

_NS_INLINE void NS::Application::run()
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( run ) );
}

_NS_INLINE void NS::Application::terminate( const Object* pSender )
{
	Object::sendMessage< void >( this, _APPKIT_PRIVATE_SEL( terminate_ ), pSender );
}
