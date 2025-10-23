# Macro to setup a WickedEngine application
# Usage: setup_wicked_app(MyProjectName)
macro(setup_wicked_app PROJECT_NAME)
    # THIS IS A HACK BUT WORKS FOR NOW: THIS IS ALREADY SET IN WICKEDENGINE's CMakeLists.txt BUT ITS NOT VISIBLE HERE
    if(WIN32)
        set(LIBDXCOMPILER "dxcompiler.dll")
        set(COPY_OR_SYMLINK_DIR_CMD "copy_directory")
    else()
        set(LIBDXCOMPILER "libdxcompiler.so")
        set(COPY_OR_SYMLINK_DIR_CMD "symlink_directory")
    endif()
    # Link against WickedEngine
    target_link_libraries(${PROJECT_NAME} WickedEngine)
    
    # Copy dxcompiler DLL next to the executable
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${WICKED_ROOT_DIR}/WickedEngine/${LIBDXCOMPILER} 
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying ${LIBDXCOMPILER} to output directory for ${PROJECT_NAME}"
    )
    
    # Optionally copy shaders as well
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} 
            ${WICKED_ROOT_DIR}/WickedEngine/shaders 
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders to output directory for ${PROJECT_NAME}"
    )

    if(WIN32 AND MSVC)
        add_compile_options(/MP)
    endif()
endmacro()