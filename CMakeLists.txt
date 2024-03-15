cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    glpaper
    VERSION 0.1.0
    DESCRIPTION "OpenGL Wallpaper for Window Managers"
    HOMEPAGE_URL "https://github.com/matjam/glpaper"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----
find_package(fmt CONFIG REQUIRED)
find_package(X11 REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(toml11 CONFIG REQUIRED)

# ---- Declare executable ----

add_executable(glpaper_exe src/main.cpp src/getwindow.cpp src/config.cpp src/scaling.cpp)
add_executable(glpaper::exe ALIAS glpaper_exe)

set_property(TARGET glpaper_exe PROPERTY OUTPUT_NAME glpaper)

target_compile_features(glpaper_exe PRIVATE cxx_std_20)

target_link_libraries(glpaper_exe PRIVATE fmt::fmt X11::X11 sfml-system sfml-network sfml-graphics sfml-window spdlog::spdlog toml11::toml11)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT glpaper_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of glpaper"
  )
endif()

include(cmake/dev-mode.cmake)
