cmake_minimum_required(VERSION 3.12)

include(GNUInstallDirs)
if (PLATFORM MATCHES "SDL2")
	add_subdirectory(FAudio)
endif()

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
	*.h
	*.hpp
	*.inl
)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
	*.c
	*.cpp
)

# FAudio has their own CMakeLists
list(FILTER HEADER_FILES
	EXCLUDE REGEX ${CMAKE_CURRENT_SOURCE_DIR}/FAudio/.*
)
list(FILTER SOURCE_FILES
	EXCLUDE REGEX ${CMAKE_CURRENT_SOURCE_DIR}/FAudio/.*
)

install(FILES ${HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/Utility/")

# some *.cpp and *.c files are included by other files,
# so they are not source files that should be
# compiled on their own; hence we mark them as header files
file(GLOB_RECURSE DIRECTLY_INCLUDED_FILES CONFIGURE_DEPENDS
	# included by utility_common.cpp
	basis_universal/*.cpp
	basis_universal/*.c
	mikktspace.c

	# included by volk.h
	volk.c
)
set_property(
	SOURCE ${DIRECTLY_INCLUDED_FILES}
	PROPERTY HEADER_FILE_ONLY ON
)

if (NOT WIN32)
	list(REMOVE_ITEM SOURCE_FILES
		${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemAlloc.cpp
	)
endif()
add_library(Utility STATIC
	${SOURCE_FILES}
	${HEADER_FILES}
)

set_target_properties("Utility" PROPERTIES
	FOLDER "ThirdParty"
	POSITION_INDEPENDENT_CODE ${WICKED_DYNAMIC_LIBRARY}
)
