cmake_minimum_required (VERSION 3.28)
 
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main 
)

FetchContent_MakeAvailable(ftxui)
 
project(ftxui-starter LANGUAGES CXX VERSION 1.0.0)
add_executable(ftxui-starter main.cpp)
set_target_properties(ftxui-starter PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON 
)

# libgit2
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

# libfmt
find_library(boost_system boost_execption boost_program_options REQUIRED)
find_package(fmt)

if(LIBGIT2_FOUND)
  message(STATUS "Found libgit2: ${LIBGIT2_VERSION}")
else()
  message(FATAL_ERROR "libgit2 not found. Install it via your package manager or source.")
endif()

target_link_libraries(ftxui-starter
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component 
  PRIVATE ${LIBGIT2_LIBRARIES}    # Link libgit2
  PRIVATE boost_program_options boost_system boost_exception fmt::fmt-header-only
)
# Include directories for libgit2
target_include_directories(ftxui-starter PRIVATE ${LIBGIT2_INCLUDE_DIRS})

# Add any additional options or flags for libgit2
target_compile_options(ftxui-starter PRIVATE ${LIBGIT2_CFLAGS_OTHER})
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
