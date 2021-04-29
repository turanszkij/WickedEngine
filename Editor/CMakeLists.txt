if (NOT WIN32)
	find_package(Threads REQUIRED)
endif ()

set (SOURCE_FILES
	main_${PLATFORM}.cpp
	#$<$<STREQUAL:${PLATFORM},Windows>:App_${PLATFORM}.cpp>
	AnimationWindow.cpp
	CameraWindow.cpp
	DecalWindow.cpp
	Editor.cpp
	EmitterWindow.cpp
	EnvProbeWindow.cpp
	ForceFieldWindow.cpp
	HairParticleWindow.cpp
	IKWindow.cpp
	LayerWindow.cpp
	LightWindow.cpp
	MaterialWindow.cpp
	MeshWindow.cpp
	ModelImporter_GLTF.cpp
	ModelImporter_OBJ.cpp
	NameWindow.cpp
	ObjectWindow.cpp
	PaintToolWindow.cpp
	PostprocessWindow.cpp
	RendererWindow.cpp
	SoundWindow.cpp
	SpringWindow.cpp
	stdafx.cpp
	TransformWindow.cpp
	Translator.cpp
	WeatherWindow.cpp
	xatlas.cpp
)

if (WIN32)
	list (APPEND SOURCE_FILES
		Editor.rc
	)

	add_executable(WickedEngineEditor WIN32 ${SOURCE_FILES})

	target_link_libraries(WickedEngineEditor PUBLIC
		WickedEngine_Windows 
	)

	set_property(TARGET WickedEngineEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
else ()
	add_executable(WickedEngineEditor ${SOURCE_FILES})

	target_link_libraries(WickedEngineEditor PUBLIC
		WickedEngine 
		Threads::Threads
	)
	
	# Copy shaders to build and source folders just to be safe:
	add_custom_command(
		TARGET WickedEngineEditor POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/WickedEngine/shaders/spirv ${CMAKE_CURRENT_SOURCE_DIR}/shaders/spirv
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/WickedEngine/shaders/spirv ${CMAKE_CURRENT_BINARY_DIR}/shaders/spirv
	)

endif ()

# Copy content to build folder:
add_custom_command(
	TARGET WickedEngineEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_BINARY_DIR}/images
)

