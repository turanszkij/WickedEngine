find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

add_executable( Tests
        main_SDL2.cpp
        stdafx.cpp
        Tests.cpp
)

target_link_libraries(Tests PUBLIC 
	WickedEngine 
	${SDL2_LIBRARY} 
	Threads::Threads
)

target_include_directories(WickedEngine PUBLIC
	${SDL2_INCLUDE_DIR}
)
