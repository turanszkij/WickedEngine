# install all libraries to path defined by DESTINATION
# symlinks are resolved and both the symlink and the target are installed
#
# example: install imported TARGETS
# find_package(Qt5 COMPONENTS Core REQUIRED)
# install_shared_libs(TARGETS Qt5::Core DESTINATION lib REQUIRED)
#
# example: define the libraries (or symlinks to libraries) to install
# install_shared_libs(FILES "/usr/lib/x86_64-linux-gnu/libQt5Core.so" DESTINATION bin REQUIRED)
#
# example: pass the linker flags and let cmake find the libraries to install
# install_shared_libs("-lQt5Core" DESTINATION lib REQUIRED)
#
# options:
# - NO_INSTALL_SYMLINKS: don't install symlinks, just the real shared libraries
# - REQUIRED|OPTIONAL: default is OPTIONAL, if REQUIRED is specified an error
#                      is thrown if the library is not found
# - QUIET: don't print status messages

function(install_shared_libs)
  set(function_name "install_shared_libs")
  set(function_synopsis "${function_name}([TARGETS [target]+] [FILES [file]+] DESTINATION destination [target_lib_or_file] [NO_INSTALL_SYMLINKS] [REQUIRED|OPTIONAL] [QUIET]")

  # parse arguments
  set(option_args REQUIRED OPTIONAL NO_INSTALL_SYMLINKS QUIET)
  set(one_value_args DESTINATION)
  set(multi_value_args TARGETS FILES CONFIGURATIONS)
  cmake_parse_arguments(x "${option_args}" "${one_value_args}" "${multi_value_args}" ${ARGV})

  # check options (flags)
  set(x_options)
  if(x_REQUIRED AND x_OPTIONAL)
    message(FATAL_ERROR
      "'${function_name}' incorrect usage,"
      " both REQUIRED and OPTIONAL are specified."
      " Synopsis: ${function_synopsis}"
      )
  endif()
  # append for recursive calls to insall_shared_libs
  if(x_NO_INSTALL_SYMLINKS)
    list(APPEND x_options "NO_INSTALL_SYMLINKS")
  endif()
  if(x_REQUIRED)
    list(APPEND x_options "REQUIRED")
  else()
    list(APPEND x_options "OPTIONAL")
  endif()
  if(x_QUIET)
    list(APPEND x_options "QUIET")
  endif()

  # option DESTINATION is mandatory
  string(COMPARE EQUAL "${x_DESTINATION}" "" is_empty)
  if(is_empty)
    message(FATAL_ERROR
      "'${function_name}' incorrect usage,"
      " option 'DESTINATION' with one argument is mandatory."
      " Synopsis: ${function_synopsis}"
      )
  endif()

  # when TARGETS is defined no free libs are allowed
  list(LENGTH x_UNPARSED_ARGUMENTS x_len)
  if(x_len GREATER 0)
    list(LENGTH x_TARGETS x_len_TARGETS)
    list(LENGTH x_FILES x_len_FILES)
    if(x_len_TARGETS GREATER 0)
      message(FATAL_ERROR
        "'${function_name}' incorrect usage,"
        " if TARGETS defined no free arguments are allowed '${x_UNPARSED_ARGUMENTS}'."
        " Synopsis: ${function_synopsis}"
        )
    endif()
    if(x_len_FILES GREATER 0)
      message(FATAL_ERROR
        "'${function_name}' incorrect usage,"
        " if FILES defined no free arguments are allowed '${x_FILES}' '${x_UNPARSED_ARGUMENTS}'."
        " Synopsis: ${function_synopsis}"
        )
    endif()
  endif()

  list(LENGTH x_CONFIGURATIONS x_len_CONFIGURATIONS)
  if(x_len_CONFIGURATIONS GREATER 0)
    set(x_conf_options CONFIGURATIONS ${x_CONFIGURATIONS})
    set(x_conf_info "CONFIGURATIONS '${x_CONFIGURATIONS}'")
  else()
    set(x_conf_options)
    set(x_conf_info "CONFIGURATIONS 'all'")
  endif()

  # local variable to use
  set(shared_libs_destination "${x_DESTINATION}")

  foreach(target_lib ${x_TARGETS})
    if(TARGET ${target_lib})
      unset(lib_location)
      unset(lib_location_rel) # Release
      unset(lib_location_dbg) # Debug
      unset(lib_location_noc) # NoConfig
      set(found_location NO)
      # special handling for cmake targets
      get_target_property(lib_location ${target_lib} IMPORTED_LOCATION)
      get_target_property(lib_location_rel ${target_lib} IMPORTED_LOCATION_RELEASE)
      get_target_property(lib_location_dbg ${target_lib} IMPORTED_LOCATION_DEBUG)
      get_target_property(lib_location_noc ${target_lib} IMPORTED_LOCATION_NOCONFIG)
      if(lib_location)
        install_shared_libs(FILES ${lib_location}
          DESTINATION ${shared_libs_destination}
          ${x_options})
        set(found_location YES)
      endif()
      if(lib_location_rel)
        install_shared_libs(FILES ${lib_location_rel}
          DESTINATION ${shared_libs_destination}
          CONFIGURATIONS Release RelWithDebInfo
          ${x_options})
        set(found_location YES)
      endif()
      if(lib_location_dbg)
        install_shared_libs(FILES ${lib_location_dbg}
          DESTINATION ${shared_libs_destination}
          CONFIGURATIONS Debug
          ${x_options})
        set(found_location YES)
      endif()
      if(lib_location_noc)
        install_shared_libs(FILES ${lib_location_noc}
          DESTINATION ${shared_libs_destination}
          ${x_options})
        set(found_location YES)
      endif()
      if(NOT found_location)
        if(x_REQUIRED)
          message(FATAL_ERROR
            "'${function_name}' can't get imported location from required target '${target_lib}'"
            )
        elseif(NOT x_QUIET)
          message(STATUS
            "'${function_name}' can't get imported location from optional target '${target_lib}'"
            )
        endif()
      endif()
    else() # TARGET
      if(x_REQUIRED)
        message(FATAL_ERROR
          "'${function_name}' required target can't be found '${target_lib}'"
          )
      elseif(NOT x_QUIET)
        message(STATUS
          "'${function_name}' optional target can't be found '${target_lib}'"
          )
      endif()
    endif()
  endforeach() # TARGETS

  # no-targets are defined, free arguments are parsed as libraries
  foreach(lib ${x_UNPARSED_ARGUMENTS})
    # check if it is a linker flag
    # for example  -L/usr/x86_64-w64-mingw32/lib  -lmingw32 -lSDL2main -lSDL2 -mwindows
    string(REGEX MATCH "-L[^ \t]+" linker_search_dir ${lib})
    string(REGEX MATCHALL "[ \t;]-l[^ \t]+" linker_libs " ${lib}")
    string(REGEX REPLACE "^-L" "" linker_search_dir "${linker_search_dir}")
    if(linker_libs)
      foreach(linker_lib ${linker_libs})
        string(REGEX REPLACE "^[ \t;]-l" "" linker_lib ${linker_lib})
        find_library(lib_path ${linker_lib}
          HINTS "${linker_search_dir}")
        if(lib_path)
          install_shared_libs(
            FILES ${lib_path}
            DESTINATION ${shared_libs_destination}
            ${x_options})
          unset(lib_path CACHE)
          continue()
        else() # no library path found
          if(x_REQUIRED)
            message(FATAL_ERROR
              "'${function_name}' no library found for required library '${linker_lib}' with linker search directory '${linker_search_dir}'"
              )
          elseif(NOT x_QUIET)
            message(STATUS
              "'${function_name}' no library found for optional library '${linker_lib}' with linker search directory '${linker_search_dir}'"
              )
          endif() # required
        endif() # lib_path
      endforeach() # linker_lib in linker_libs
      continue()
    endif() #linker_libs
  endforeach() # unparsed library

  # install all passed filepaths with some magic to get the dynamic libraries
  foreach(lib ${x_FILES})
    # got an absolute path for a library
    string(REGEX MATCH "\\.(dll\\.a$|lib$)" is_static "${lib}")
    if(is_static)
      # lib is a dynamic wrapper ending in .dll.a, search for the linked
      # dll in the ../bin and ../lib directories
      get_filename_component(dynlib_name ${lib} NAME_WE)
      string(REGEX REPLACE "^lib" "" dynlib_simple "${dynlib_name}")
      get_filename_component(linker_search_dir ${lib} DIRECTORY)
      get_filename_component(linker_search_dir ${linker_search_dir} DIRECTORY)
      file(GLOB dyn_lib
        "${linker_search_dir}/bin/${dynlib_simple}*.dll"
        "${linker_search_dir}/lib/${dynlib_simple}*.dll"
        LIST_DIRECTORIES false)
      if(dyn_lib)
        # found the dynamic library
        set(lib "${dyn_lib}")
      endif()
    endif()
    string(REGEX MATCH "\\.(so|dll$|dylib$)" is_shared_lib "${lib}")
    if(NOT is_shared_lib)
      # don't install static libraries
      if(x_REQUIRED)
        message(FATAL_ERROR
          "'${function_name}' ${x_conf_info}: required library is not a shared library '${lib}'"
          )
      elseif(NOT x_QUIET)
        message(STATUS
          "'${function_name}' ${x_conf_info}: optional library is not a shared library '${lib}'"
          )
        continue()
      endif()
    endif()
    # resolve possible symlink and install the real library
    get_filename_component(lib_REAL ${lib} REALPATH)
    if(NOT x_QUIET)
      message(STATUS
        "'${function_name}' ${x_conf_info}: install shared lib: ${lib_REAL}")
    endif()
    install(FILES ${lib_REAL} ${x_conf_options} DESTINATION ${shared_libs_destination})
    # check if given library is no symlink to prevent double installation
    STRING(COMPARE NOTEQUAL "${lib}" "${lib_REAL}" real_lib_different)
    if(real_lib_different AND NOT x_NO_INSTALL_SYMLINKS)
      # find all the symlinks linking to lib_REAL
      string(REGEX MATCH "\\.so" is_shared_lib_linux "${lib}")
      string(REGEX MATCH "\\.dylib$" is_shared_lib_darwin "${lib}")
      if(is_shared_lib_linux OR is_shared_lib_darwin)
        if(is_shared_lib_linux)
          string(REGEX REPLACE "\\.so.*" ".so" lib_base "${lib}")
            file(GLOB lib_symlinks "${lib_base}*")
        else()
          string(REGEX REPLACE "\\.dylib$" "" lib_base "${lib}")
            file(GLOB lib_symlinks "${lib_base}*.dylib")
        endif()
        foreach(lib_sym ${lib_symlinks})
          STRING(COMPARE NOTEQUAL "${lib_sym}" "${lib_REAL}" real_lib_different)
          if(NOT real_lib_different)
            continue() # we found the real lib, not a symlink, skip it
          endif()
          if(NOT x_QUIET)
            message(STATUS
              "'${function_name}' ${x_conf_info}: install symlink to lib: ${lib_sym}")
          endif()
          # actually install the found symlink
          install(FILES ${lib_sym}  ${x_conf_options} DESTINATION ${shared_libs_destination})
        endforeach()
      else() # neither linux nor darwin
        if(NOT x_QUIET)
          message(STATUS
            "'${function_name}' ${x_conf_info}: install symlink to lib: ${lib}")
        endif()
        install(FILES ${lib}  ${x_conf_options}     DESTINATION ${shared_libs_destination})
      endif()
    endif() # x_NO_INSTALL_SYMLINKS
  endforeach()
endfunction()
