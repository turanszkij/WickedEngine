#pragma     once

#include    <initializer_list>

#if defined(__ARM_ARCH) || defined(_M_ARM) || defined(_M_ARM64)
#   define BUILD_ARCH_ARM 1
#elif defined(_MSC_VER)
#   if defined(_M_X86) || defined(_M_X64)
#       define BUILD_ARCH_X86 1
#   endif
#elif defined(__GNUC__) || defined(__clang__)
#   if defined(__i386__) || defined(__x86_64__)
#       define BUILD_ARCH_X86 1
#   endif
#endif

#if defined(_WIN32)
#   define BUILD_PLATFORM_WIN 1
#elif defined(__APPLE__)
#   include "TargetConditionals.h"
#   if defined(TARGET_OS_IPHONE)
#       define BUILD_PLATFORM_IOS 1
#   else
#       define BUILD_PLATFORM_OSX 1
#   endif
#elif defined(__linux__) || defined(__gnu_linux__) || defined(linux)
#   define BUILD_PLATFORM_LINUX 1
#elif defined(__unix__)
#   define BUILD_PLATFORM_UNIX 1
#elif defined(__ANDROID__)
#   define BUILD_PLATFORM_ANDROID 1
#endif

/*
 * BUILD_INTRINSICS_LEVEL 0..3. Try setting different levels and see what compiles/runs/doesn't crash.
 * 0 _XM_NO_INTRINSICS_
 * 1 Some
 * 2 More
 * 3 All
 */
#if defined(BUILD_PLATFORM_WIN)
#   if !defined(BUILD_INTRINSICS_LEVEL)
#       define BUILD_INTRINSICS_LEVEL 3
#   endif
#else
#   if !defined(BUILD_INTRINSICS_LEVEL)
#       define BUILD_INTRINSICS_LEVEL 1
#   endif
#endif

#if defined(BUILD_ARCH_ARM)
#   if defined(__ARM_NEON) && BUILD_INTRINSICS_LEVEL > 0
#       define _XM_ARM_NEON_INTRINSICS_
#   else
#       define _XM_NO_INTRINSICS_
#   endif
#else
#   if BUILD_INTRINSICS_LEVEL > 0
#       define _XM_SSE_INTRINSICS_
#   endif
#   if BUILD_INTRINSICS_LEVEL > 1
#       define _XM_SSE3_INTRINSICS_
#       define _XM_SSE4_INTRINSICS_
#       define _XM_AVX_INTRINSICS_
#   endif
#   if BUILD_INTRINSICS_LEVEL > 2
#       define _XM_F16C_INTRINSICS_
#   endif
#endif
#if defined(__GNUC__) || defined(BUILD_PLATFORM_IOS)
#   define _XM_NO_CALL_CONVENTION_
#endif
#if defined(BUILD_PLATFORM_IOS) || defined(BUILD_PLATFORM_ANDROID)
#   define _XM_ARM_NEON_NO_ALIGN_
#endif
