cmake_minimum_required(VERSION 3.12)

set(MIN_OpenImageDenoise_VERSION "2.0")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(INSTALL_LOCAL_CONFIGDIR "${CMAKE_BINARY_DIR}/cmake")
set(INSTALL_CONFIGDIR "${CMAKE_INSTALL_LIBDIR}/cmake/WickedEngine")

if (WICKED_DYNAMIC_LIBRARY)
	set(WICKED_LIBRARY_TYPE SHARED)
	message(STATUS "Building WickedEngine as a shared library")
else()
	set(WICKED_LIBRARY_TYPE STATIC)
	message(STATUS "Building WickedEngine as a static library")
endif()

if (WIN32)
	# TODO: Choose whether to use SDL2 on windows as well
	set(TARGET_NAME WickedEngine_Windows)
else ()
	set(TARGET_NAME WickedEngine_Linux)
	find_package(SDL2 REQUIRED)
	find_package(OpenImageDenoise "${MIN_OpenImageDenoise_VERSION}" QUIET)
	find_package(Threads REQUIRED)
	if(NOT ${OpenImageDenoise_FOUND})
		message("OpenImageDenoise>=${MIN_OpenImageDenoise_VERSION} not found, it will be disabled.")
	else()
		message("OpenImageDenoise ${OpenImageDenoise_VERSION} Found.")
	endif()

	if(NOT TARGET SDL2::SDL2)
		# using old SDL2 cmake, lets create a SDL2 target ourselves
		find_library(SDL2_LIBRARY_FILE_LOCATION SDL2 REQUIRED)

		add_library(SDL2::SDL2 SHARED IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES ${SDL2_INCLUDE_DIRS}
			INTERFACE_LINK_LIBRARIES ${SDL2_LIBRARIES}
			IMPORTED_LOCATION ${SDL2_LIBRARY_FILE_LOCATION}
		)

		unset(SDL2_LIBRARY_FILE_LOCATION)
	endif()

	# minimum sdl version is 2.0.14 for controller LED support
	if((${SDL_VERSION_MAJOR} GREATER_EQUAL 2) AND (${SDL2_VERSION_MINOR} GREATER_EQUAL 0) AND (${SDL2_VERSION_PATCH} GREATER_EQUAL 14))
		add_compile_definitions(SDL2_FEATURE_CONTROLLER_LED=1)
	endif()

	if (WAYLAND_BACKEND)

		find_package(PkgConfig REQUIRED)
		pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
		pkg_check_modules(WAYLAND_SCANNER REQUIRED wayland-scanner)
		pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
		# Get variables
		pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)
		pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
		################################################################################################################
		# Wayland protocol generated code
		function(GENERATE_XDG_PROTOCOLS)
			# GENERATE XDG PROTOCOLS SOURCES
			set(XDG_PROTOCOLS_SOURCES)
			set(XDG_PROTOCOLS_HEADERS)
			foreach(XML ${ARGN})
				get_filename_component(XML_NAME "${XML}" NAME_WE)
				MESSAGE("Generating C source code for ${XML} -> ${XML_NAME}")
				add_custom_command(COMMAND ${WAYLAND_SCANNER_BIN}
					ARGS private-code "${XML}" "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.c"
					DEPENDS "${XML}"
					OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.c"
					COMMENT "Generating \"${XML_NAME}\" protocol private code."
					)
				list(APPEND XDG_PROTOCOLS_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.c")

				add_custom_command(COMMAND ${WAYLAND_SCANNER_BIN}
					ARGS client-header "${XML}" "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.h"
					DEPENDS "${XML}"
					OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.h"
					COMMENT "Generating \"${XML_NAME}\" protocol header."
					)
				list(APPEND XDG_PROTOCOLS_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/${XML_NAME}.h")
				set(XDG_PROTOCOLS_SOURCES ${XDG_PROTOCOLS_SOURCES} PARENT_SCOPE)
				set(XDG_PROTOCOLS_HEADERS ${XDG_PROTOCOLS_HEADERS} PARENT_SCOPE)
			endforeach()
		endfunction()

		GENERATE_XDG_PROTOCOLS(
			"${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml"
			"${WAYLAND_PROTOCOLS_DATADIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
		)
		file(GLOB WAYLAND_SOURCES CONFIGURE_DEPENDS Wayland/*.cpp)
		file(GLOB WAYLAND_HEADERS CONFIGURE_DEPENDS Wayland/*.h)
		set(WAYLAND_SOURCES ${WAYLAND_SOURCES} ${XDG_PROTOCOLS_SOURCES})
		set(WAYLAND_HEADERS ${WAYLAND_HEADERS} ${XDG_PROTOCOLS_HEADERS})
	endif()
endif()

add_subdirectory(LUA)
add_subdirectory(Utility)

add_compile_definitions(JPH_DEBUG_RENDERER=1)
set(PHYSICS_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
include(${PHYSICS_REPO_ROOT}/Jolt/Jolt.cmake)

file(GLOB HEADER_FILES CONFIGURE_DEPENDS *.h)

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
list(REMOVE_ITEM SOURCE_FILES offlineshadercompiler.cpp)

add_library(${TARGET_NAME} ${WICKED_LIBRARY_TYPE}
	${SOURCE_FILES}
	${HEADER_FILES}
	${WAYLAND_SOURCES}
	${WAYLAND_HEADERS}
)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

add_library(WickedEngine ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADER_FILES} ${WAYLAND_HEADERS}")

target_include_directories(${TARGET_NAME} SYSTEM PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine>
)

set(WICKEDENGINE_STATIC_LIBRARIES
	Jolt
	LUA
	Utility
)

if (WIN32)
	target_compile_definitions(${TARGET_NAME} PUBLIC
		UNICODE _UNICODE
	)

	target_compile_options(${TARGET_NAME} PRIVATE /W3)

	set(LIBDXCOMPILER "dxcompiler.dll")
else ()
	# `ska::flat_hash_map` has issues on linux because of the hash function being identity
	# in same cases. Use `robin_hood::unordered_flat_map` instead
	target_compile_definitions(${TARGET_NAME} PUBLIC WI_UNORDERED_MAP_TYPE=2)

	target_link_libraries(${TARGET_NAME} PUBLIC
		Threads::Threads
		SDL2::SDL2
		$<$<BOOL:${OpenImageDenoise_FOUND}>:OpenImageDenoise> # links OpenImageDenoise only if it's found
	)
	set(WICKEDENGINE_STATIC_LIBRARIES ${WICKEDENGINE_STATIC_LIBRARIES} FAudio)

	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${TARGET_NAME} PRIVATE
			# C and C++ warnings
			-Wuninitialized
			#-Wwrite-strings
			#-Winit-self
			#-Wreturn-type
			# C++ only warnings
			#$<$<COMPILE_LANGUAGE:CXX>:-Wreorder>
			#$<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
			# Treat warnings as errors
			#-Werror
			#uncomment this to stop the compilation at the first error
			# -Wfatal-errors
			)
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# add some warnings and set them as errors
	# read more details here: https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
	target_compile_options(${TARGET_NAME} PRIVATE
			# C and C++ warnings
			-Wuninitialized
			-Wmaybe-uninitialized
			-Wwrite-strings
			-Winit-self
			-Wreturn-type
			# C++ only warnings
			$<$<COMPILE_LANGUAGE:CXX>:-Wreorder>
			$<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
			# Treat warnings as errors
			-Werror
			#uncomment this to stop the compilation at the first error
			# -Wfatal-errors
			)
	endif()


	if (WAYLAND_BACKEND)
		target_include_directories(${TARGET_NAME}
			PUBLIC ${WAYLAND_CLIENT_INCLUDE_DIRS}
			# PRIVATE ${WAYLAND_CURSOR_INCLUDE_DIRS}
			PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
			# PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine>
			)

		target_link_libraries(${TARGET_NAME}
			PUBLIC ${WAYLAND_CLIENT_LIBRARIES}
			# PRIVATE ${WAYLAND_CURSOR_LIBRARIES}
			)
	endif()

	target_link_libraries(${TARGET_NAME} PRIVATE dl)

	set(LIBDXCOMPILER "libdxcompiler.so")
endif()

target_link_libraries(${TARGET_NAME} PUBLIC ${WICKEDENGINE_STATIC_LIBRARIES})

if (PLATFORM MATCHES "SDL2")
	target_compile_definitions(${TARGET_NAME} PUBLIC SDL2=1)
endif()

add_library(dxcompiler SHARED IMPORTED)
set_property(TARGET dxcompiler PROPERTY
		IMPORTED_LOCATION ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} )

# Copy the shader library next to the executable
add_custom_command(
		TARGET ${TARGET_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${WICKED_ROOT_DIR}/WickedEngine/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)


# OFFLINE SHADER COMPILER
add_executable(offlineshadercompiler
		offlineshadercompiler.cpp
)

target_link_libraries(offlineshadercompiler
		PUBLIC ${TARGET_NAME})

install(TARGETS offlineshadercompiler
		RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

install(DIRECTORY "${WICKED_ROOT_DIR}/Content"
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

#### LOCAL FOLDER INSTALLATION
# Create importable target here

#set_target_properties(${TARGET_NAME} PROPERTIES PUBLIC_HEADER "${HEADERS}")
set_target_properties("${TARGET_NAME}" PROPERTIES EXPORT_NAME "${TARGET_NAME}")


install(TARGETS "${TARGET_NAME}" ${WICKEDENGINE_STATIC_LIBRARIES}
		EXPORT Engine-Targets
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine")

install(FILES ${LIBDXCOMPILER}
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/WickedEngine")

export(EXPORT Engine-Targets
		FILE "${CMAKE_BINARY_DIR}/cmake/WickedEngineTargets.cmake"
		NAMESPACE WickedEngine::
		)

install(EXPORT Engine-Targets
		FILE WickedEngineTargets.cmake
		NAMESPACE WickedEngine::
		DESTINATION ${INSTALL_CONFIGDIR})

set(_CONFIG_INSTALL_DIR_INCLUDE "${WICKED_ROOT_DIR}")
set(_CONFIG_INSTALL_DIR_LIB "${WICKED_ROOT_DIR}")
configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
		${CMAKE_BINARY_DIR}/cmake/WickedEngineConfig.cmake
		INSTALL_DESTINATION ${INSTALL_LOCAL_CONFIGDIR}
)
set(_CONFIG_INSTALL_DIR_INCLUDE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(_CONFIG_INSTALL_DIR_LIB "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
configure_package_config_file(
		${CMAKE_CURRENT_SOURCE_DIR}/WickedEngineConfig.cmake.in
		${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
		INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

install(FILES ${CMAKE_BINARY_DIR}/cmake/install/WickedEngineConfig.cmake
		DESTINATION ${INSTALL_CONFIGDIR}
)

# Shaders
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine"
		PATTERN "spirv" EXCLUDE
		PATTERN "hlsl6" EXCLUDE
		PATTERN "*.vcxitems*" EXCLUDE
		)
