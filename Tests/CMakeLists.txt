find_package(Threads REQUIRED)

add_executable(Tests
	main_SDL2.cpp
	stdafx.cpp
	Tests.cpp
)

target_link_libraries(Tests PUBLIC 
	WickedEngine
	Threads::Threads
)
