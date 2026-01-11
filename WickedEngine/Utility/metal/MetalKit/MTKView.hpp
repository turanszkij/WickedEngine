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
// MetalKit/MTKView.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "MetalKitPrivate.hpp"

#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <CoreGraphics/CGColorSpace.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MTK
{
	class ViewDelegate
	{
		public:
			virtual						~ViewDelegate() { }
			virtual void				drawInMTKView( class View* pView ) { }
			virtual void				drawableSizeWillChange( class View* pView, CGSize size ) { }
	};

	class View : public NS::Referencing< MTK::View, NS::View >
	{
		public:
			static View*				alloc();
			View*						init( CGRect frame, const MTL::Device* pDevice );
			View*						init( NS::Coder* pCoder );

			void						setDevice( const MTL::Device* pDevice );
			MTL::Device*				device() const;

			void						setDelegate( const MTK::ViewDelegate* pDelegate );
			ViewDelegate*				delegate() const;

			CA::MetalDrawable*			currentDrawable() const;

			void						setFramebufferOnly( bool framebufferOnly );
			bool						framebufferOnly() const;

			void						setDepthStencilAttachmentTextureUsage( MTL::TextureUsage textureUsage );
			MTL::TextureUsage			depthStencilAttachmentTextureUsage() const;

			void						setMultisampleColorAttachmentTextureUsage( MTL::TextureUsage textureUsage );
			MTL::TextureUsage			multisampleColorAttachmentTextureUsage() const;

			void						setPresentsWithTransaction( bool presentsWithTransaction );
			bool						presentsWithTransaction() const;

			void						setColorPixelFormat( MTL::PixelFormat colorPixelFormat );
			MTL::PixelFormat			colorPixelFormat() const;

			void						setDepthStencilPixelFormat( MTL::PixelFormat colorPixelFormat );
			MTL::PixelFormat			depthStencilPixelFormat() const;

			void						setSampleCount( NS::UInteger sampleCount );
			NS::UInteger				sampleCount() const;

			void						setClearColor( MTL::ClearColor clearColor );
			MTL::ClearColor				clearColor() const;

			void						setClearDepth( double clearDepth );
			double						clearDepth() const;

			void						setClearStencil( uint32_t clearStencil );
			uint32_t					clearStencil() const;

			MTL::Texture*				depthStencilTexture() const;

			MTL::Texture*				multisampleColorTexture() const;

			void						releaseDrawables() const;

			MTL::RenderPassDescriptor*	currentRenderPassDescriptor() const;

			void						setPreferredFramesPerSecond( NS::Integer preferredFramesPerSecond );
			NS::Integer					preferredFramesPerSecond() const;

			void						setEnableSetNeedsDisplay( bool enableSetNeedsDisplay );
			bool						enableSetNeedsDisplay() const;

			void						setAutoresizeDrawable( bool autoresizeDrawable );
			bool						autoresizeDrawable();

			void						setDrawableSize( CGSize drawableSize );
			CGSize						drawableSize() const;

			CGSize						preferredDrawableSize() const;
			MTL::Device*				preferredDevice() const;

			void						setPaused( bool paused );
			bool						isPaused() const;

			void						setColorSpace( CGColorSpaceRef colorSpace );
			CGColorSpaceRef				colorSpace() const;

			void						draw();

	};
}

_NS_INLINE MTK::View* MTK::View::alloc()
{
	   return NS::Object::alloc< View >( _MTK_PRIVATE_CLS( MTKView ) );
}

_NS_INLINE MTK::View* MTK::View::init( CGRect frame, const MTL::Device* pDevice )
{
	return NS::Object::sendMessage< View* >( this, _MTK_PRIVATE_SEL( initWithFrame_device_ ), frame, pDevice );
}

_NS_INLINE MTK::View* MTK::View::init( NS::Coder* pCoder )
{
	return NS::Object::sendMessage< View* >( this, _MTK_PRIVATE_SEL( initWithCoder_ ), pCoder );
}

_NS_INLINE void MTK::View::setDevice( const MTL::Device* pDevice )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setDevice_ ), pDevice );
}

_NS_INLINE MTL::Device* MTK::View::device() const
{
	return NS::Object::sendMessage< MTL::Device* >( this, _MTK_PRIVATE_SEL( device ) );
}

_NS_INLINE void MTK::View::setDelegate( const MTK::ViewDelegate* pDelegate )
{
	// TODO: Same problem as NS::Application::setDelegate.
	// Requires a similar soution
	NS::Value* pWrapper = NS::Value::value( pDelegate );

	// drawInMTKView:

	void (*drawDispatch)( NS::Value*, SEL, id ) = []( NS::Value* pSelf, SEL _cmd, id pMTKView ){
		auto pDel = reinterpret_cast< MTK::ViewDelegate* >( pSelf->pointerValue() );
		pDel->drawInMTKView( (MTK::View *)pMTKView );
	};

	class_addMethod( (Class)objc_lookUpClass( "NSValue" ), sel_registerName( "drawInMTKView:" ), (IMP)drawDispatch, "v@:@" );

	// mtkView:drawableSizeWillChange:

	void (*drawableSizeWillChange)( NS::Value*, SEL, View*, CGSize ) = []( NS::Value* pSelf, SEL, View* pMTKView, CGSize size){
		auto pDel = reinterpret_cast< MTK::ViewDelegate* >( pSelf->pointerValue() );
		pDel->drawableSizeWillChange( pMTKView, size );
	};

	#if CGFLOAT_IS_DOUBLE
		const char* cbparams = "v@:@{CGSize=dd}";
	#else
		const char* cbparams = "v@:@{CGSize=ff}";
	#endif // CGFLOAT_IS_DOUBLE

	class_addMethod( (Class)objc_lookUpClass( "NSValue" ), sel_registerName( "mtkView:drawableSizeWillChange:"), (IMP)drawableSizeWillChange, cbparams );

	// This circular reference leaks the wrapper object to keep it around for the dispatch to work.
	// It may be better to hoist it to the MTK::View as a member.
	objc_setAssociatedObject( (id)pWrapper, "mtkviewdelegate_cpp", (id)pWrapper, OBJC_ASSOCIATION_RETAIN_NONATOMIC );

	NS::Object::sendMessage< void >( this, sel_registerName( "setDelegate:" ), pWrapper );
}

_NS_INLINE MTK::ViewDelegate* MTK::View::delegate() const
{
	NS::Value* pWrapper = NS::Object::sendMessage< NS::Value* >( this, _MTK_PRIVATE_SEL( delegate ) );
	if ( pWrapper )
	{
		return reinterpret_cast< ViewDelegate* >( pWrapper->pointerValue() );
	}
	return nullptr;
}

_NS_INLINE CA::MetalDrawable* MTK::View::currentDrawable() const
{
	return NS::Object::sendMessage< CA::MetalDrawable* >( this, _MTK_PRIVATE_SEL( currentDrawable ) );
}

_NS_INLINE void MTK::View::setFramebufferOnly( bool framebufferOnly )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setFramebufferOnly_ ), framebufferOnly );
}

_NS_INLINE bool MTK::View::framebufferOnly() const
{
	return NS::Object::sendMessage< bool >( this, _MTK_PRIVATE_SEL( framebufferOnly ) );
}

_NS_INLINE void MTK::View::setDepthStencilAttachmentTextureUsage( MTL::TextureUsage textureUsage )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setDepthStencilAttachmentTextureUsage_ ), textureUsage );
}

_NS_INLINE MTL::TextureUsage MTK::View::depthStencilAttachmentTextureUsage() const
{
	return NS::Object::sendMessage< MTL::TextureUsage >( this, _MTK_PRIVATE_SEL( depthStencilAttachmentTextureUsage ) ); 
}

_NS_INLINE void MTK::View::setMultisampleColorAttachmentTextureUsage( MTL::TextureUsage textureUsage )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setMultisampleColorAttachmentTextureUsage_ ), textureUsage );
}

_NS_INLINE MTL::TextureUsage MTK::View::multisampleColorAttachmentTextureUsage() const
{
	return NS::Object::sendMessage< MTL::TextureUsage >( this, _MTK_PRIVATE_SEL( multisampleColorAttachmentTextureUsage ) ); 
}

_NS_INLINE void MTK::View::setPresentsWithTransaction( bool presentsWithTransaction )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setPresentsWithTransaction_ ), presentsWithTransaction );
}

_NS_INLINE bool MTK::View::presentsWithTransaction() const
{
	return NS::Object::sendMessage< bool >( this, _MTK_PRIVATE_SEL( presentsWithTransaction ) );
}

_NS_INLINE void MTK::View::setColorPixelFormat( MTL::PixelFormat colorPixelFormat )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setColorPixelFormat_ ), colorPixelFormat );
}

_NS_INLINE MTL::PixelFormat MTK::View::colorPixelFormat() const
{
	return NS::Object::sendMessage< MTL::PixelFormat >( this, _MTK_PRIVATE_SEL( colorPixelFormat ) );
}

_NS_INLINE void MTK::View::setDepthStencilPixelFormat( MTL::PixelFormat colorPixelFormat )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setDepthStencilPixelFormat_ ), colorPixelFormat );
}

_NS_INLINE MTL::PixelFormat MTK::View::depthStencilPixelFormat() const
{
	return NS::Object::sendMessage< MTL::PixelFormat >( this, _MTK_PRIVATE_SEL( depthStencilPixelFormat ) );
}

_NS_INLINE void MTK::View::setSampleCount( NS::UInteger sampleCount )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setSampleCount_), sampleCount );
}

_NS_INLINE NS::UInteger MTK::View::sampleCount() const
{
	return NS::Object::sendMessage< NS::UInteger >( this, _MTK_PRIVATE_SEL( sampleCount ) );
}

_NS_INLINE void MTK::View::setClearColor( MTL::ClearColor clearColor )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setClearColor_ ), clearColor );
}

_NS_INLINE MTL::ClearColor MTK::View::clearColor() const
{
	return NS::Object::sendMessage< MTL::ClearColor >( this, _MTK_PRIVATE_SEL( clearColor) );
}

_NS_INLINE void MTK::View::setClearDepth( double clearDepth )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setClearDepth_ ), clearDepth );
}

_NS_INLINE double MTK::View::clearDepth() const
{
	return NS::Object::sendMessage< double >( this, _MTK_PRIVATE_SEL( clearDepth ) );
}

_NS_INLINE void MTK::View::setClearStencil( uint32_t clearStencil )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setClearStencil_ ), clearStencil );
}

_NS_INLINE uint32_t MTK::View::clearStencil() const
{
	return NS::Object::sendMessage< uint32_t >( this, _MTK_PRIVATE_SEL( clearStencil ) );
}

_NS_INLINE MTL::Texture* MTK::View::depthStencilTexture() const
{
	return NS::Object::sendMessage< MTL::Texture* >( this, _MTK_PRIVATE_SEL( depthStencilTexture ) );
}

_NS_INLINE MTL::Texture* MTK::View::multisampleColorTexture() const
{
	return NS::Object::sendMessage< MTL::Texture* >( this, _MTK_PRIVATE_SEL( multisampleColorTexture ) );
}

_NS_INLINE void MTK::View::releaseDrawables() const
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( releaseDrawables ) );
}

_NS_INLINE MTL::RenderPassDescriptor* MTK::View::currentRenderPassDescriptor() const
{
	return NS::Object::sendMessage< MTL::RenderPassDescriptor* >( this, _MTK_PRIVATE_SEL( currentRenderPassDescriptor ) );
}

_NS_INLINE void MTK::View::setPreferredFramesPerSecond( NS::Integer preferredFramesPerSecond )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setPreferredFramesPerSecond_ ), preferredFramesPerSecond );
}

_NS_INLINE NS::Integer MTK::View::preferredFramesPerSecond() const
{
	return NS::Object::sendMessage< NS::Integer >( this, _MTK_PRIVATE_SEL( preferredFramesPerSecond ) );
}

_NS_INLINE void MTK::View::setEnableSetNeedsDisplay( bool enableSetNeedsDisplay )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setEnableSetNeedsDisplay_ ), enableSetNeedsDisplay );
}

_NS_INLINE bool MTK::View::enableSetNeedsDisplay() const
{
	return NS::Object::sendMessage< bool >( this, _MTK_PRIVATE_SEL( enableSetNeedsDisplay ) );
}

_NS_INLINE void MTK::View::setAutoresizeDrawable( bool autoresizeDrawable )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setAutoresizeDrawable_ ), autoresizeDrawable );
}

_NS_INLINE bool MTK::View::autoresizeDrawable()
{
	return NS::Object::sendMessage< bool >( this, _MTK_PRIVATE_SEL( autoresizeDrawable ) );
}

_NS_INLINE void MTK::View::setDrawableSize( CGSize drawableSize )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setDrawableSize_ ), drawableSize );
}

_NS_INLINE CGSize MTK::View::drawableSize() const
{
	return NS::Object::sendMessage< CGSize >( this, _MTK_PRIVATE_SEL( drawableSize ) );
}

_NS_INLINE CGSize MTK::View::preferredDrawableSize() const
{
	return NS::Object::sendMessage< CGSize >( this, _MTK_PRIVATE_SEL( preferredDrawableSize ) );
}

_NS_INLINE MTL::Device* MTK::View::preferredDevice() const
{
	return NS::Object::sendMessage< MTL::Device* >( this, _MTK_PRIVATE_SEL( preferredDevice ) );
}

_NS_INLINE void MTK::View::setPaused( bool paused )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setPaused_ ), paused );
}

_NS_INLINE bool MTK::View::isPaused() const
{
	return NS::Object::sendMessage< bool >( this, _MTK_PRIVATE_SEL( isPaused ) );
}

_NS_INLINE void MTK::View::setColorSpace( CGColorSpaceRef colorSpace )
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( setColorspace_), colorSpace );
}

_NS_INLINE CGColorSpaceRef MTK::View::colorSpace() const
{
	return NS::Object::sendMessage< CGColorSpaceRef >( this, _MTK_PRIVATE_SEL( colorspace ) );
}

_NS_INLINE void MTK::View::draw()
{
	NS::Object::sendMessage< void >( this, _MTK_PRIVATE_SEL( draw ) );
}
