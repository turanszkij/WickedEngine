set (SOURCE_FILES
	python_bindings.cpp
)

find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(pywickedengine MODULE ${SOURCE_FILES})
target_compile_definitions(pywickedengine PUBLIC PYTHON_BINDINGS)
target_link_libraries(pywickedengine PUBLIC WickedEngine)
target_compile_definitions(pywickedengine PUBLIC "WICKED_ROOT_DIR=\"${WICKED_ROOT_DIR}\"")
install(TARGETS pywickedengine DESTINATION "~/.config/blender/4.0/scripts/addons/pywickedengine/")
