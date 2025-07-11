cmake_minimum_required(VERSION 3.19)

add_custom_target(Content
    COMMAND
	${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR} &&
	${CMAKE_COMMAND} -E create_symlink ${WICKED_ROOT_DIR}/Content ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Symlinking Content directory"
    VERBATIM
)
