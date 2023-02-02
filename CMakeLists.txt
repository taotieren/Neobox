cmake_minimum_required(VERSION 3.21.2)

project(Neobox VERSION 2.3.5 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX "install")

# https://zhuanlan.zhihu.com/p/429849408
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

if (NOT CMAKE_BUILD_TYPE)
    message(WARNING "No CMAKE_BUILD_TYPE value specified, defaulting to RelWithDebInfo.")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
else ()
    message(STATUS "The CMake build type is: ${CMAKE_BUILD_TYPE}")
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if( NOT DEFAULT_INSTALL_PATH )
    set( DEFAULT_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}"
         CACHE
         PATH
         "Location of Neobox data files." )
endif()

# -----------------------------------------------------------------------------
# Determine which port will be built
# -----------------------------------------------------------------------------
set(ALL_PORTS
    Linux
    Mac
    Windows
)
set(PORT "NOPORT" CACHE STRING "choose which Neobox port to build (one of ${ALL_PORTS})")

list(FIND ALL_PORTS ${PORT} RET)
if (${RET} EQUAL -1)
    message(FATAL_ERROR "Please choose which Neobox port to build (one of ${ALL_PORTS})")
endif ()

string(TOLOWER ${PORT} NEOBOX_PORT_DIR)

string(TIMESTAMP COMPILE_TIME %Y-%m-%d)
set(NEOBOX_BUILD_TIME ${COMPILE_TIME})

# list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.4.2/msvc2019_64")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH})
message(STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")

add_definitions(-DUNICODE -D_UNICODE)
add_compile_options("/utf-8")

link_directories(build/pluginmgr)
file(GLOB project_includes */include */*/include)
file(GLOB thrd_includes 3rdlib/*/include)
file(GLOB project_subdirs */CMakeLists.txt)
foreach(item ${project_includes})
  message(STATUS "include dir is ${item}")
endforeach()
include_directories(${project_includes})
include_directories(../YJson/include)
add_subdirectory(../YJson yjson)

foreach(item ${project_subdirs})
  string(REGEX REPLACE "/CMakeLists.txt" "" subdir ${item})
  get_filename_component(subdir ${subdir} NAME)
  add_subdirectory(${subdir})
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_executable(neobox WIN32 main.cpp logo.rc)

target_link_libraries(neobox PUBLIC
  pluginmgr
  yjson
  Qt6::Widgets
)

install(TARGETS neobox
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include
)
