project(plugin-runner)
cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(AMXConfig)

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  add_definitions(-DLINUX)
  include_directories(src/amx)
endif()

add_definitions(
  -DPAWN_CELL_SIZE=32
  -DAMXCONSOLE_NOIDLE
  -DAMX_ALTPRINT
)

set(AMX_SOURCES
  src/amx/amx.c
  src/amx/amx.h
  src/amx/amxaux.c
  src/amx/amxaux.h
  src/amx/amxcons.c
  src/amx/amxcore.c
  src/amx/amxfile.c
  src/amx/amxstring.c
  src/amx/float.c
  src/amx/fpattern.h
  src/amx/getch.h
  src/amx/osdefs.h
  src/amx/sclinux.h
)

if(UNIX)
  list(APPEND AMX_SOURCES src/amx/getch.c)
endif()

add_executable(plugin-runner
  ${AMX_SOURCES}
  src/plugin-runner.cpp
  src/plugin.cpp
  src/plugin.h
  src/plugincommon.h
)

if(UNIX)
  target_compile_options(plugin-runner PRIVATE -m32 -Wno-attributes)
  target_link_libraries(plugin-runner -m32)
endif()

if(MSVC)
  # Incremental linking causes MSVC to genrate jmps in the AMX functions table,
  # and because of that jit/crashdetect think that amx_Exec() is hoooked by
  # something else.
  set_property(TARGET plugin-runner APPEND_STRING PROPERTY LINK_FLAGS "/INCREMENTAL:NO")
endif()

if(UNIX)
  target_link_libraries(plugin-runner dl)
endif()
