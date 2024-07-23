cmake_minimum_required(VERSION 3.12)

if(CMAKE_VERSION VERSION_LESS "3.26.0")
  set(COPY_DIR_CMD copy_directory)
else()
  set(COPY_DIR_CMD copy_directory_if_different)
endif()

add_custom_target(Content
    COMMAND ${CMAKE_COMMAND} -E ${COPY_DIR_CMD}
        ${WICKED_ROOT_DIR}/Content
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copying Content"
    VERBATIM
)
