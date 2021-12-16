/*
BSD LICENSE

Copyright (c) 2019-2020 Superluminal. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include "PerformanceAPI_capi.h"

// When PERFORMANCEAPI_ENABLED is defined to 0, all calls to the PerformanceAPI (either through macro or direct function calls) will be compiled out.
#ifndef PERFORMANCEAPI_ENABLED
	#ifdef _WIN32
		#define	PERFORMANCEAPI_ENABLED 1
	#else
		#define	PERFORMANCEAPI_ENABLED 0
	#endif
#endif

/* ------------------------------------------------------------------------
*  Documentation
*  ------------------------------------------------------------------------
*
* NOTE: The C++ free functions in this header are deprecated. They remain here only for backwards compatibility. The C functions, prefixed with PerformanceAPI_ and found in PerformanceAPI_capi.h,
*       should be used instead. The C++ functions are no longer maintained and new features will be added to the C interface only.
*		The InstrumentationScope helper is *not* deprecated.
*
* The Performance API can be used to augment the sampling data that is naturally collected by Superluminal Performance
* with instrumentation data. Instrumentation data is seamlessly blended in all views in the UI.
*  
* To send instrumentation data to Superluminal, two mechanisms are provided:
* - The InstrumentationScope class. This class will signal the start of a scope in its constructor and signal the end of the scope in its destructor. 
*									InstrumentationScopes can be freely nested.
*
* - The PerformanceAPI_BeginScope/PerformanceAPI_EndScope free functions. These functions are designed for integration with existing profiling systems that, for example, 
*											already define their own scope-based profiling classes.
*
*	Note: calls to the Begin/EndScope functions must be within the same function. For example, it is not allowed to call BeginScope in function Foo 
*		  and EndScope in function Bar.
*  
*  When sending an instrumentation event through either of these mechanisms, two pieces of data can be provided:
*  - The event ID [required]   :	This must be a static string (e.g. a regular C string literal). It is used to distinguish events
* 									in the UI and is displayed in all views (Instrumentation Chart, Timeline, CallGraph).
* 									It is important that the ID of a particular scope remains the same over the lifetime of the program:
* 									it's not allowed to use a string that changes for every invocation of the function/scope.
* 									Some examples of IDs: the name of a function ("Game::Update"), the operation being performed ("ReadFile"), etc
*  
*  - The event Data [optional] :	This must be a string that is either dynamically allocated or a regular string literal. You are free to put
*									whatever data you want in the string; there are no restrictions. The data is also free to change over the lifetime of the program.
*									The intent of the data string is to include data in the event that can differ per instance.
*									This data is displayed in the Instrumentation Chart and Timeline.
*									Some examples of Data strings: the current frame number (for "Game::Update"), the path of the file being read (for "ReadFile"), etc
*									This parameter is optional; use nullptr as argument if you don't have any contextual data.
*  
*  - The event Color [optional] :	This is a color that will be used to display the event in the timeline. The color for a specific scope is coupled to the ID and must 
*									be the same over the lifetime of the program. It's an RGB value encoded as an uint32_t: RRGGBB00.
*									You can use PERFORMANCEAPI_MAKE_COLOR to create the uint32_t from 3 RGB values in the range of [0, 255].
*									This parameter is optional; use PERFORMANCEAPI_DEFAULT_COLOR as argument to use the default coloring.
*
*  All const char* arguments in the API are assumed to be UTF8 encoded strings (i.e. non-ASCII chars are fully supported).
*/
namespace PerformanceAPI
{
#if PERFORMANCEAPI_ENABLED
	// An InstrumentationScope measures the time of the scope it is contained in; time starts when the constructor is called and ends when the 
	// destructor is called. 
	// An ID for the scope must be provided, with optional data and an optional color (see documentation at the top of this file for more info)
	// While you can manually use this, it's usually more convenient to use the PERFORMANCEAPI_* macros
	struct InstrumentationScope final
	{
		/**
		 * @param inID	 The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 */
		InstrumentationScope(const char* inID);
		
		/**
		 * @param inID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 * @param inData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
		 */
		InstrumentationScope(const char* inID, const char* inData);

		/**
		 * @param inID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 * @param inData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
		 * @param inColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
		 */
		InstrumentationScope(const char* inID, const char* inData, uint32_t inColor);

		/**
		 * @param inID	 The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 */
		InstrumentationScope(const wchar_t* inID);
		
		/**
		 * @param inID   The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 * @param inData The data for this scope as an UTF16 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
		 */
		InstrumentationScope(const wchar_t* inID, const wchar_t* inData);

		/**
		 * @param inID   The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
		 * @param inData The data for this scope as an UTF16 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
		 * @param inColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
		 */
		InstrumentationScope(const wchar_t* inID, const wchar_t* inData, uint32_t inColor);

		~InstrumentationScope();
	};

	// Private helper macros
	#define PERFORMANCEAPI_CAT_IMPL(a, b) a##b	
	#define PERFORMANCEAPI_CAT(a, b) PERFORMANCEAPI_CAT_IMPL(a, b)	
	#define PERFORMANCEAPI_UNIQUE_IDENTIFIER(a) PERFORMANCEAPI_CAT(a, __LINE__)

	/**
	 * Creates an InstrumentationScope with the specified ID.
	 *
	 * @param InstrumentationID The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 */
	#define PERFORMANCEAPI_INSTRUMENT(InstrumentationID)							PerformanceAPI::InstrumentationScope PERFORMANCEAPI_UNIQUE_IDENTIFIER(__instrumentation_scope__)((InstrumentationID));

	/**
	 * Creates an InstrumentationScope with the specified ID and runtime data
	 *
	 * @param InstrumentationID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program. See docs at the top of this file.
	 * @param InstrumentationData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 */
	#define PERFORMANCEAPI_INSTRUMENT_DATA(InstrumentationID, InstrumentationData)	PerformanceAPI::InstrumentationScope PERFORMANCEAPI_UNIQUE_IDENTIFIER(__instrumentation_scope__)((InstrumentationID), (InstrumentationData));

	/**
	 * Creates an InstrumentationScope with the specified ID and color
	 *
	 * @param InstrumentationID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program. See docs at the top of this file.
	 * @param InstrumentationColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	#define PERFORMANCEAPI_INSTRUMENT_COLOR(InstrumentationID, InstrumentationColor)	PerformanceAPI::InstrumentationScope PERFORMANCEAPI_UNIQUE_IDENTIFIER(__instrumentation_scope__)((InstrumentationID), "", (InstrumentationColor));

	/**
	 * Creates an InstrumentationScope with the specified ID, runtime data and color
	 *
	 * @param InstrumentationID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program. See docs at the top of this file.
	 * @param InstrumentationData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 * @param InstrumentationColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	#define PERFORMANCEAPI_INSTRUMENT_DATA_COLOR(InstrumentationID, InstrumentationData, InstrumentationColor)	PerformanceAPI::InstrumentationScope PERFORMANCEAPI_UNIQUE_IDENTIFIER(__instrumentation_scope__)((InstrumentationID), (InstrumentationData), (InstrumentationColor));

	/**
	 * Convenience wrapper around PERFORMANCEAPI_INSTRUMENT to create an InstrumentationScope with the name of the function as ID
	 */
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION()									PERFORMANCEAPI_INSTRUMENT(__FUNCTION__)

	/**
	 * Convenience wrapper around PERFORMANCEAPI_INSTRUMENT_DATA to create an InstrumentationScope with the name of the function as ID
	 *
	 * @param InstrumentationData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 */
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA(InstrumentationData)			PERFORMANCEAPI_INSTRUMENT_DATA(__FUNCTION__, (InstrumentationData))

	/**
	 * Convenience wrapper around PERFORMANCEAPI_INSTRUMENT_COLOR to create an InstrumentationScope with the name of the function as ID
	 *
	 * @param InstrumentationColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_COLOR(InstrumentationColor)			PERFORMANCEAPI_INSTRUMENT_COLOR(__FUNCTION__, (InstrumentationColor))

	/**
	 * Convenience wrapper around PERFORMANCEAPI_INSTRUMENT_DATA_COLOR to create an InstrumentationScope with the name of the function as ID
	 *
	 * @param InstrumentationData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 * @param InstrumentationColor The color for this scope. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA_COLOR(InstrumentationData, InstrumentationColor) PERFORMANCEAPI_INSTRUMENT_DATA_COLOR(__FUNCTION__, (InstrumentationData), (InstrumentationColor))


	 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Deprecated functions. 
	//
	// These functions remain here only for backwards compatibility. They're no longer maintained and new features will be added to the C interface only.
	// The C functions, prefixed with PerformanceAPI_ and found in PerformanceAPI_capi.h, should be used instead.
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * Set the name of the current thread to the specified thread name. 
	 *
	 * @param inThreadName The thread name as an UTF8 encoded string.
	 */
	inline void SetCurrentThreadName(const char* inThreadName) { PerformanceAPI_SetCurrentThreadName(inThreadName); }

	/**
	 * Begin an instrumentation event with the specified ID
	 *
	 * @param inID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 */
	inline void BeginEvent(const char* inID) { PerformanceAPI_BeginEvent(inID, nullptr, PERFORMANCEAPI_DEFAULT_COLOR); }

	/**
	 * Begin an instrumentation event with the specified ID
	 *
	 * @param inID   The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 */
	inline void BeginEvent(const wchar_t* inID) { PerformanceAPI_BeginEvent_Wide(inID, nullptr, PERFORMANCEAPI_DEFAULT_COLOR); }

	/**
	 * Begin an instrumentation event with the specified ID and runtime data
	 *
	 * @param inID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 * @param inData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 */
	inline void BeginEvent(const char* inID, const char* inData) { PerformanceAPI_BeginEvent(inID, inData, PERFORMANCEAPI_DEFAULT_COLOR); }

	/**
	 * Begin an instrumentation event with the specified ID and runtime data
	 *
	 * @param inID   The ID of this scope as an UTF8 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 * @param inData The data for this scope as an UTF8 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 * @param inColor The color for this event. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	inline void BeginEvent(const char* inID, const char* inData, uint32_t inColor) { PerformanceAPI_BeginEvent(inID, inData, inColor); }

	/**
	 * Begin an instrumentation event with the specified ID and runtime data
	 *
	 * @param inID   The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 * @param inData The data for this scope as an UTF16 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 */
	inline void BeginEvent(const wchar_t* inID, const wchar_t* inData) { PerformanceAPI_BeginEvent_Wide(inID, inData, PERFORMANCEAPI_DEFAULT_COLOR); }

	/**
	 * Begin an instrumentation event with the specified ID and runtime data
	 *
	 * @param inID   The ID of this scope as an UTF16 encoded string. The ID for a specific scope must be the same over the lifetime of the program (see docs at the top of this file)
	 * @param inData The data for this scope as an UTF16 encoded string. The data can vary for each invocation of this scope and is intended to hold information that is only available at runtime. See docs at the top of this file.
	 * @param inColor The color for this event. The color for a specific scope is coupled to the ID and must be the same over the lifetime of the program
	 */
	inline void BeginEvent(const wchar_t* inID, const wchar_t* inData, uint32_t inColor) { PerformanceAPI_BeginEvent_Wide(inID, inData, inColor); }

	/**
	 * End an instrumentation event. Must be matched with a call to BeginEvent within the same function
	 * Note: the return value can be ignored. It is only there to prevent calls to the function from being optimized to jmp instructions as part of tail call optimization.
	 */
	inline PerformanceAPI_SuppressTailCallOptimization EndEvent() { return PerformanceAPI_EndEvent(); }

#else
	#define PERFORMANCEAPI_INSTRUMENT(InstrumentationID)
	#define PERFORMANCEAPI_INSTRUMENT_DATA(InstrumentationID, InstrumentationData)
	#define PERFORMANCEAPI_INSTRUMENT_COLOR(InstrumentationID, InstrumentationColor)
	#define PERFORMANCEAPI_INSTRUMENT_DATA_COLOR(InstrumentationID, InstrumentationData, InstrumentationColor)

	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION()
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA(InstrumentationData)
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_COLOR(InstrumentationColor)
	#define PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA_COLOR(InstrumentationData, InstrumentationColor)

	inline void SetCurrentThreadName(const char* inThreadName) {}
	inline void BeginEvent(const char* inID) {}
	inline void BeginEvent(const char* inID, const char* inData) {}
	inline void BeginEvent(const char* inID, const char* inData, uint32_t inColor) {}
	inline void BeginEvent(const wchar_t* inID) {}
	inline void BeginEvent(const wchar_t* inID, const wchar_t* inData) {}
	inline void BeginEvent(const wchar_t* inID, const wchar_t* inData, uint32_t inColor) {}
	inline void EndEvent() {}
#endif
}