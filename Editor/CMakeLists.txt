find_package(Threads REQUIRED)

add_executable(WickedEngineEditor 
    main_${PLATFORM}.cpp
    $<$<STREQUAL:${PLATFORM},Windows>:App_${PLATFORM}.cpp>
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

target_link_libraries(WickedEngineEditor PUBLIC
	WickedEngine 
	Threads::Threads
)
