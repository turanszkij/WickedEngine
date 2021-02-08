set (SOURCE_FILES
	utility_common.cpp
	spirv_reflect.c
	stb_vorbis.c
)

if (WIN32)
	list (APPEND SOURCE_FILES
		D3D12MemAlloc.cpp
	)
endif ()

add_library(Utility STATIC ${SOURCE_FILES})

set_property(TARGET "Utility" PROPERTY FOLDER "ThirdParty")
