cmake_minimum_required(VERSION 3.14)

project(replxx_example LANGUAGES CXX C DESCRIPTION "replxx CMake example with FetchContent" VERSION 1.0.0.0)

include(GNUInstallDirs)
include(FetchContent)

FetchContent_Declare(
  replxx
  GIT_REPOSITORY https://github.com/AmokHuginnsson/replxx
  GIT_TAG        master
)
FetchContent_MakeAvailable(replxx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
  cxx-api.cxx
  util.c util.h
  include/replxx.hxx
)

target_include_directories( ${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    replxx::replxx
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
