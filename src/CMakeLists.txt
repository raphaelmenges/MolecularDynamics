cmake_minimum_required(VERSION 2.8)

# Set paths
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "Path to custom CMake modules.")
set(EXTERNALS_PATH "${CMAKE_SOURCE_DIR}/../externals" CACHE PATH "Path to external code.")
set(RESOURCES_PATH "${CMAKE_SOURCE_DIR}/../resources" CACHE PATH "Path to resources.")
set(EXECUTABLES_PATH "${CMAKE_SOURCE_DIR}/executables" CACHE PATH "Path to code of executables.")
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/libraries" CACHE PATH "Path to code of libraries.")
set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/shaders" CACHE PATH "Path to code of shaders.")
set(MINICONDA3_PATH "$ENV{HOME}/miniconda3" CACHE PATH "Path to miniconda3 installation.")

# Find site packages folder of miniconda
set(SITE_PACKAGES_PATH "${MINICONDA3_PATH}/lib/python3.5/site-packages")
set(PYTHON_INCLUDE_DIRS "${MINICONDA3_PATH}/include/python3.5m")
set(PYTHON_LIBRARIES "${MINICONDA3_PATH}/lib/libpython3.5m.so")
set(PYTHON_PACKAGES_PATH "${SITE_PACKAGES_PATH}")
message("Path to site packages of minicoda3: ${SITE_PACKAGES_PATH}")

# Include default project
include(${CMAKE_MODULE_PATH}/DefaultProject.cmake)
