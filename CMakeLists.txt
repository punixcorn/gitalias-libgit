cmake_minimum_required (VERSION 3.28)
 
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG main 
)

FetchContent_MakeAvailable(ftxui)
 
project(gitalias-libgit LANGUAGES CXX VERSION 1.0.0)
add_executable(${PROJECT_NAME} main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON 
)

# libgit2
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

# libfmt
find_library(boost_system boost_execption boost_program_options REQUIRED)
find_package(fmt)
find_library(LIBGIT2_LIBRARY git2 REQUIRED)
find_path(LIBGIT2_INCLUDE_DIR git2.h REQUIRED)

if(LIBGIT2_LIBRARY AND LIBGIT2_INCLUDE_DIR)
  message(STATUS "Found libgit2 at ${LIBGIT2_LIBRARY}")
  message(STATUS "Include directory: ${LIBGIT2_INCLUDE_DIR}")
else()
  message(FATAL_ERROR "libgit2 not found. Install it via your package manager or source.")
endif()


target_link_libraries(${PROJECT_NAME}
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component  
  PRIVATE ${LIBGIT2_LIBRARY}  
  PRIVATE boost_program_options boost_system boost_exception fmt::fmt-header-only 
)
# Include directories for libgit2
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBGIT2_INCLUDE_DIR})

# Add any additional options or flags for libgit2
target_compile_options(${PROJECT_NAME} PRIVATE ${LIBGIT2_CFLAGS_OTHER})
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
