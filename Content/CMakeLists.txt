cmake_minimum_required(VERSION 3.12)
add_custom_target(Content
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${WICKED_ROOT_DIR}/Content

        ${CMAKE_CURRENT_BINARY_DIR}

    COMMENT "Copying Content"
    VERBATIM
)
