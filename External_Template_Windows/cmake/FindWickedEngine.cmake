# - Find WickedEngine
# Find the native WickedEngine includes and library
#
#   WICKEDENGINE_FOUND       - True if WickedEngine is found.
#   WICKEDENGINE_INCLUDE_DIR - where to find WickedEngine.h, etc.
#   WICKEDENGINE_LIBRARIES   - List of libraries when using WickedEngine.
#

IF( WICKEDENGINE_INCLUDE_DIR )
    # Already in cache, be silent
    SET( WickedEngine_FIND_QUIETLY TRUE )
ENDIF( WICKEDENGINE_INCLUDE_DIR )

FIND_PATH( WICKEDENGINE_INCLUDE_DIR "WickedEngine.h"
           PATH_SUFFIXES "WickedEngine/" )

IF(${CMAKE_BUILD_TYPE} MATCHES Debug)
    FIND_LIBRARY( WICKEDENGINE_LIBRARIES
        NAMES "WickedEngine_Windows"
        PATH_SUFFIXES "out/build/x64-Debug/WickedEngine" )
ELSE()
    FIND_LIBRARY( WICKEDENGINE_LIBRARIES
        NAMES "WickedEngine_Windows"
        PATH_SUFFIXES "out/build/x64-Release/WickedEngine" )
ENDIF()


# handle the QUIETLY and REQUIRED arguments and set WICKEDENGINE_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE( "FindPackageHandleStandardArgs" )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( "WickedEngine" DEFAULT_MSG WICKEDENGINE_INCLUDE_DIR WICKEDENGINE_LIBRARIES )

MARK_AS_ADVANCED( WICKEDENGINE_INCLUDE_DIR WICKEDENGINE_LIBRARIES )
