cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake CACHE PATH "Project specific path. Set manually if it was not found.")
set(RESOURCES_PATH ${CMAKE_SOURCE_DIR}/../resources CACHE PATH "Project specific path. Set manually if it was not found.")
set(EXECUTABLES_PATH ${CMAKE_SOURCE_DIR}/executables CACHE PATH "Project specific path. Set manually if it was not found.")
set(LIBRARIES_PATH ${CMAKE_SOURCE_DIR}/libraries CACHE PATH "Project specific path. Set manually if it was not found.")
set(SHADERS_PATH ${CMAKE_SOURCE_DIR}/shaders CACHE PATH "Project specific path. Set manually if it was not found.")
set(MDTRAJ_PATH   CACHE PATH "/home/nlichtenberg/miniconda3/lib/python3.4/site-packages")
set(PYTHON_INCLUDE_DIRS   CACHE PATH "/usr/include/python3.4")
set(PYTHON_LIBRARIES   CACHE PATH "/usr/lib/python3.4/config-3.4m-x86_64-linux-gnu/libpython3.4.so")

include(${CMAKE_MODULE_PATH}/DefaultProject.cmake)
