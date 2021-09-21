if (PLATFORM MATCHES "SDL2")
	add_subdirectory(FAudio)
endif()

set (SOURCE_FILES
	utility_common.cpp
	spirv_reflect.c
	stb_vorbis.c
	samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		D3D12MemAlloc.cpp
	)
endif ()

add_library(Utility STATIC ${SOURCE_FILES})

set_property(TARGET "Utility" PROPERTY FOLDER "ThirdParty")
