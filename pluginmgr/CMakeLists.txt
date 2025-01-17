cmake_minimum_required(VERSION 3.10.1)

if (UNIX)
  find_package(CURL CONFIG REQUIRED)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
qt_standard_project_setup()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

configure_file(
  src/config.h.in config.h
)

qt_wrap_ui(wrap_files
  widgets/tabnetproxy.ui
  widgets/tabhotkey.ui
) # 根据UI文件生成头文件

file(GLOB source_files include/*.hpp src/*.cpp widgets/*.cpp widgets/*.hpp)
file(GLOB _resource_files fonts/*.ttf  icons/*.png icons/*.ico  jsons/*.json scripts/* styles/*.qss)

set(system_shared_files Shell32)
set(qt_shared_files Qt6::Core Qt6::Widgets Qt6::Gui)
set(pliugin_libfiles yjson)

if(WIN32)
  list(APPEND system_shared_files Iphlpapi Winhttp)
else()
  list(APPEND system_shared_files CURL::libcurl)
endif()

set(resource_files "")
foreach(full_filepath ${_resource_files})
  file(RELATIVE_PATH file_path ${CMAKE_CURRENT_SOURCE_DIR} ${full_filepath})
  message(STATUS "res='${file_path}'")
  list(APPEND resource_files ${file_path})
endforeach()

add_library(pluginmgr SHARED
  ${source_files}
  ${wrap_files}
)

target_link_libraries(pluginmgr PUBLIC
  ${pliugin_libfiles}
  ${qt_shared_files}
  ${system_shared_files}
)

qt_add_resources(pluginmgr "neobox"
  PREFIX "/"
  FILES ${resource_files}
)

install(TARGETS pluginmgr
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION include
)
