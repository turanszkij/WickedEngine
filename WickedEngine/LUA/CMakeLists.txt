set(LUA_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/lapi.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lcode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lctype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ldebug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ldo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lfunc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lgc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/llex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/llimits.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lmem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lobject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lparser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lprefix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lstate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lstring.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ltable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ltm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lua.h
    ${CMAKE_CURRENT_SOURCE_DIR}/luaconf.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lualib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lundump.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lvm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lzio.h
)

add_library(LUA STATIC
    lapi.c
    lauxlib.c
    lbaselib.c
    lbitlib.c
    lcode.c
    lcorolib.c
    lctype.c
    ldblib.c
    ldebug.c
    ldo.c
    ldump.c
    lfunc.c
    lgc.c
    linit.c
    liolib.c
    llex.c
    lmathlib.c
    lmem.c
    loadlib.c
    lobject.c
    lopcodes.c
    loslib.c
    lparser.c
    lstate.c
    lstring.c
    lstrlib.c
    ltable.c
    ltablib.c
    ltm.c
    lua.c
    lundump.c
    lutf8lib.c
    lvm.c
    lzio.c
    ${LUA_HEADERS}
)

if (UNIX)
    # for Linux, BSD, Solaris, Minix, Apple
    target_compile_definitions(LUA PUBLIC LUA_USE_POSIX=1)
endif()

set_target_properties(LUA PROPERTIES
	FOLDER "ThirdParty"
	POSITION_INDEPENDENT_CODE ${WICKED_PIC_CODE}
)

install(FILES ${LUA_HEADERS}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/WickedEngine/LUA/")
