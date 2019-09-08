cmake_minimum_required(VERSION 2.8.12)
project(plugin-runner)

set(PLUGIN_RUNNER_VERSION 1.2)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(AMXConfig)

add_definitions(
  -DPAWN_CELL_SIZE=32
  -DAMXCONSOLE_NOIDLE
  -DAMX_ALTPRINT
  -DAMXTIME_NOIDLE
)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  add_definitions(-DLINUX)
  include_directories(src/amx)
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(AMX_SOURCES
  src/amx/amx.c
  src/amx/amx.h
  src/amx/amxaux.c
  src/amx/amxaux.h
  src/amx/amxcons.c
  src/amx/amxcore.c
  src/amx/amxfile.c
  src/amx/amxstring.c
  src/amx/amxtime.c
  src/amx/float.c
  src/amx/fpattern.h
  src/amx/getch.h
  src/amx/osdefs.h
  src/amx/sclinux.h
)

if(UNIX)
  list(APPEND AMX_SOURCES src/amx/getch.c)
endif()

add_library(amx STATIC ${AMX_SOURCES})

if(UNIX)
  set_property(TARGET amx APPEND_STRING PROPERTY
               COMPILE_FLAGS "-m32 -Wno-attributes")
  target_link_libraries(amx -m32)
endif()

add_executable(plugin-runner
  src/plugin-runner.cpp
  src/plugin.cpp
  src/plugin.h
  src/plugincommon.h
)

target_link_libraries(plugin-runner amx)

if(UNIX)
  set_property(TARGET plugin-runner APPEND_STRING PROPERTY
               COMPILE_FLAGS "-std=c++11 -m32 -Wno-attributes")
  target_link_libraries(plugin-runner -m32 dl)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(plugin-runner
      -fvisibility=hidden
      -Wl,--no-export-dynamic # Don't expose internal symbols to plugins
    )
  endif()
endif()

if(MSVC)
  # Incremental linking causes MSVC to genrate jmps in the AMX functions table,
  # and because of that jit/crashdetect think that amx_Exec() is hoooked by
  # something else.
  set_property(TARGET plugin-runner APPEND_STRING PROPERTY
               LINK_FLAGS "/INCREMENTAL:NO")
endif()

install(TARGETS plugin-runner RUNTIME DESTINATION .)
install(DIRECTORY include DESTINATION .)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${PLUGIN_RUNNER_VERSION}-win32)
  set(CPACK_GENERATOR ZIP)
elseif(UNIX)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${PLUGIN_RUNNER_VERSION}-linux)
  set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
