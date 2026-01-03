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
// MetalKit/MetalKitPrivate.hpp
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <objc/runtime.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#define _MTK_PRIVATE_CLS( symbol )				   ( Private::Class::s_k ## symbol )
#define _MTK_PRIVATE_SEL( accessor )				 ( Private::Selector::s_k ## accessor )

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if defined( MTK_PRIVATE_IMPLEMENTATION )

#define _MTK_PRIVATE_VISIBILITY						__attribute__( ( visibility( "default" ) ) )
#define _MTK_PRIVATE_IMPORT						  __attribute__( ( weak_import ) )

#if __OBJC__
#define  _MTK_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define  _MTK_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   objc_lookUpClass( # symbol ) 
#endif // __OBJC__

#define _MTK_PRIVATE_DEF_CLS( symbol )			   void*				   s_k ## symbol	   _MTK_PRIVATE_VISIBILITY = _MTK_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _MTK_PRIVATE_DEF_SEL( accessor, symbol )	 SEL					 s_k ## accessor	 _MTK_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _MTK_PRIVATE_DEF_CONST( type, symbol )	   _NS_EXTERN type const   MTK ## symbo		_MTK_PRIVATE_IMPORT; \
													 type const			  MTK::symbol	 = ( nullptr != &MTK ## symbol ) ? MTK ## symbol : nullptr;


#else

#define _MTK_PRIVATE_DEF_CLS( symbol )				extern void*			s_k ## symbol;
#define _MTK_PRIVATE_DEF_SEL( accessor, symbol )	 extern SEL			  s_k ## accessor;
#define _MTK_PRIVATE_DEF_CONST( type, symbol )


#endif // MTK_PRIVATE_IMPLEMENTATION

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MTK::Private::Class {

_MTK_PRIVATE_DEF_CLS( MTKView );

} // Class

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace MTK::Private::Selector
{

_MTK_PRIVATE_DEF_SEL( autoresizeDrawable,
					 "autoresizeDrawable" );

_MTK_PRIVATE_DEF_SEL( clearColor,
					 "clearColor" );

_MTK_PRIVATE_DEF_SEL( clearDepth,
					 "clearDepth" );

_MTK_PRIVATE_DEF_SEL( clearStencil,
					 "clearStencil" );

_MTK_PRIVATE_DEF_SEL( colorPixelFormat,
					 "colorPixelFormat" );

_MTK_PRIVATE_DEF_SEL( colorspace,
					 "colorspace" );

_MTK_PRIVATE_DEF_SEL( currentDrawable,
					 "currentDrawable" );

_MTK_PRIVATE_DEF_SEL( currentRenderPassDescriptor,
					 "currentRenderPassDescriptor" );

_MTK_PRIVATE_DEF_SEL( device,
					 "device" );

_MTK_PRIVATE_DEF_SEL( delegate,
					 "delegate" );

_MTK_PRIVATE_DEF_SEL( depthStencilAttachmentTextureUsage,
					 "depthStencilAttachmentTextureUsage" );

_MTK_PRIVATE_DEF_SEL( depthStencilPixelFormat,
					 "depthStencilPixelFormat" );

_MTK_PRIVATE_DEF_SEL( depthStencilTexture,
					 "depthStencilTexture" );

_MTK_PRIVATE_DEF_SEL( draw,
					 "draw" );

_MTK_PRIVATE_DEF_SEL( drawableSize,
					 "drawableSize" );

_MTK_PRIVATE_DEF_SEL( drawInMTKView_,
					 "drawInMTKView:" );

_MTK_PRIVATE_DEF_SEL( enableSetNeedsDisplay,
					 "enableSetNeedsDisplay" );

_MTK_PRIVATE_DEF_SEL( framebufferOnly,
					 "framebufferOnly" );

_MTK_PRIVATE_DEF_SEL( initWithCoder_,
					 "initWithCoder:" );

_MTK_PRIVATE_DEF_SEL( initWithFrame_device_,
					 "initWithFrame:device:" );

_MTK_PRIVATE_DEF_SEL( multisampleColorAttachmentTextureUsage,
					 "multisampleColorAttachmentTextureUsage" );

_MTK_PRIVATE_DEF_SEL( multisampleColorTexture,
					 "multisampleColorTexture" );

_MTK_PRIVATE_DEF_SEL( isPaused,
					 "isPaused" );

_MTK_PRIVATE_DEF_SEL( preferredFramesPerSecond,
					 "preferredFramesPerSecond" );

_MTK_PRIVATE_DEF_SEL( preferredDevice,
					 "preferredDevice" );

_MTK_PRIVATE_DEF_SEL( preferredDrawableSize,
					 "preferredDrawableSize" );

_MTK_PRIVATE_DEF_SEL( presentsWithTransaction,
					 "presentsWithTransaction" );

_MTK_PRIVATE_DEF_SEL( sampleCount,
					 "sampleCount" );

_MTK_PRIVATE_DEF_SEL( setAutoresizeDrawable_,
					 "setAutoresizeDrawable:" );

_MTK_PRIVATE_DEF_SEL( setClearColor_,
					 "setClearColor:" );

_MTK_PRIVATE_DEF_SEL( setClearDepth_,
					 "setClearDepth:" );

_MTK_PRIVATE_DEF_SEL( setClearStencil_,
					 "setClearStencil:" );

_MTK_PRIVATE_DEF_SEL( setColorPixelFormat_,
					 "setColorPixelFormat:" );

_MTK_PRIVATE_DEF_SEL( setColorspace_,
					 "setColorspace:" );

_MTK_PRIVATE_DEF_SEL( setDelegate_,
					 "setDelegate:" );

_MTK_PRIVATE_DEF_SEL( setDepthStencilAttachmentTextureUsage_,
					 "setDepthStencilAttachmentTextureUsage:" );

_MTK_PRIVATE_DEF_SEL( setDepthStencilPixelFormat_,
					 "setDepthStencilPixelFormat:" );

_MTK_PRIVATE_DEF_SEL( setDevice_,
					 "setDevice:" );

_MTK_PRIVATE_DEF_SEL( setDrawableSize_,
					 "setDrawableSize:" );

_MTK_PRIVATE_DEF_SEL( setEnableSetNeedsDisplay_,
					 "setEnableSetNeedsDisplay:" );

_MTK_PRIVATE_DEF_SEL( setFramebufferOnly_,
					 "setFramebufferOnly:" );

_MTK_PRIVATE_DEF_SEL( setMultisampleColorAttachmentTextureUsage_,
					 "setMultisampleColorAttachmentTextureUsage:" )

_MTK_PRIVATE_DEF_SEL( setPaused_,
					 "setPaused:" );

_MTK_PRIVATE_DEF_SEL( setPreferredFramesPerSecond_,
					 "setPreferredFramesPerSecond:" );

_MTK_PRIVATE_DEF_SEL( setPresentsWithTransaction_,
					 "setPresentsWithTransaction:" );

_MTK_PRIVATE_DEF_SEL( setSampleCount_,
					 "setSampleCount:" );

_MTK_PRIVATE_DEF_SEL( releaseDrawables,
					 "releaseDrawables" );

}

//---------

