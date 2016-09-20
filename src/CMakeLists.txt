cmake_minimum_required(VERSION 2.8)

# Set pathes
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake CACHE PATH "Project specific path. Set manually if it was not found.")
set(EXTERNALS_PATH ${CMAKE_SOURCE_DIR}/../externals CACHE PATH "Project specific path. Set manually if it was not found.")
set(RESOURCES_PATH ${CMAKE_SOURCE_DIR}/../resources CACHE PATH "Project specific path. Set manually if it was not found.")
set(EXECUTABLES_PATH ${CMAKE_SOURCE_DIR}/executables CACHE PATH "Project specific path. Set manually if it was not found.")
set(LIBRARIES_PATH ${CMAKE_SOURCE_DIR}/libraries CACHE PATH "Project specific path. Set manually if it was not found.")
set(SHADERS_PATH ${CMAKE_SOURCE_DIR}/shaders CACHE PATH "Project specific path. Set manually if it was not found.")
set(MINICONDA3_PATH $ENV{HOME}/miniconda3 CACHE PATH "Path to miniconda3 installation.")

# Find site packages folder of miniconda
file(GLOB SIDE_PACKAGES_PATH $ENV{HOME}/miniconda3/lib/python3.*/site-packages)
message("Path to site packages of minicoda3: ${SIDE_PACKAGES_PATH}")

# Include default project because reasons (Gerrit's design :D )
include(${CMAKE_MODULE_PATH}/DefaultProject.cmake)
