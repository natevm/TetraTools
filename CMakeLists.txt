# ┌──────────────────────────────────────────────────────────────────┐
# │  Projects Settings                                               │
# └──────────────────────────────────────────────────────────────────┘
cmake_minimum_required (VERSION 3.13)
project(Pluto)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH 1)

# Use c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(MSVC)

# Set the install prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/" CACHE PATH "..." FORCE)
endif()

# Default to release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the build configuration" FORCE)
endif()

# Use .so files on Apple
if(APPLE)
set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
endif(APPLE)

# Global defines
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# RPATH on UNIX stuff
set(RPATHS "${CMAKE_INSTALL_PREFIX};")
set(CMAKE_INSTALL_RPATH ${INSTALL_RPATH})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ┌──────────────────────────────────────────────────────────────────┐
# │  External Dependencies                                           │
# └──────────────────────────────────────────────────────────────────┘

# swig
find_package(SWIG 3.0.8 REQUIRED)
include(${SWIG_USE_FILE})
cmake_policy(SET CMP0078 NEW)

# python
find_package(Python3 3.6 COMPONENTS Interpreter Development REQUIRED)
include_directories(SYSTEM ${Python3_INCLUDE_DIRS})

# add libraries to a list for linking
set (
    LIBRARIES
    ${Python3_LIBRARIES}
)

# ┌──────────────────────────────────────────────────────────────────┐
# │  TetraTools Module                                               │
# └──────────────────────────────────────────────────────────────────┘

# Build SWIG module 
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/TetraTools.i PROPERTY CPLUSPLUS ON)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/TetraTools.i PROPERTY USE_TARGET_INCLUDE_DIRECTORIES TRUE)
swig_add_library(TetraTools TYPE SHARED LANGUAGE python OUTFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR} SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/TetraTools.i)
set_target_properties(TetraTools PROPERTIES INSTALL_RPATH "${RPATHS}")
set_target_properties(TetraTools PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
if(APPLE)
set_target_properties(TetraTools PROPERTIES MACOSX_RPATH TRUE)
endif(APPLE)
target_link_libraries(TetraTools PUBLIC ${LIBRARIES})

# Install
install(FILES ${CMAKE_BINARY_DIR}/TetraTools.py DESTINATION ${CMAKE_INSTALL_PREFIX})

if (WIN32) 
install(FILES ${CMAKE_BINARY_DIR}/_TetraTools.pyd DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_BINARY_DIR}/_TetraTools.pdb DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)
elseif(APPLE)
install(FILES ${CMAKE_BINARY_DIR}/_TetraTools.so DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
install(FILES ${CMAKE_BINARY_DIR}/_TetraTools.so DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()
