# No idea what here happens
string(REPLACE "/" ";" p2list "${CMAKE_SOURCE_DIR}")
string(REPLACE "\\" ";" p2list "${p2list}")
list(REVERSE p2list)
list(GET p2list 0 first)
list(GET p2list 1 ProjectId)
string(REPLACE " " "_" ProjectId ${ProjectId})
project(${ProjectId})

# Include cmake macros
include(${CMAKE_MODULE_PATH}/doxygen.cmake)
include(${CMAKE_MODULE_PATH}/macros.cmake)

# CMake flags
set(CMAKE_CONFIGURATION_TYPES Debug;Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Link dependencies using CMake modules
link_dependency(OpenGL3)
link_dependency(GLEW)
link_dependency(GLFW3)
link_dependency(GLM)
link_dependency(DevIL)
link_dependency(ASSIMP)
# link_dependency(ZLIB) # Seems to be not necessary

# Include externals folder
include_directories(${EXTERNALS_PATH})

# Python from miniconda TODO: be more version independend
set(PYTHON_INCLUDE_DIRS "${MINICONDA3_PATH}/include/python3.5m")
set(PYTHON_LIBRARIES "${MINICONDA3_PATH}/lib/libpython3.5m.so")
set(PYTHON_PACKAGES_PATH "${MINICONDA3_PATH}/lib/python3.5/site-packages")

# Include directories
include_directories(
    ${PYTHON_INCLUDE_DIRS}
)

# Link against python
link_libraries(
     ${PYTHON_LIBRARIES}
)

# Link against XOrg libraries on Linux
if("${CMAKE_SYSTEM}" MATCHES "Linux")
    find_package(X11)
    set(ALL_LIBRARIES ${ALL_LIBRARIES} ${X11_LIBRARIES} Xcursor Xinerama Xrandr Xxf86vm Xi pthread -ldl -llzma)
endif()

# Tell application about some paths
add_definitions(-DSHADERS_PATH="${SHADERS_PATH}")
add_definitions(-DRESOURCES_PATH="${RESOURCES_PATH}")
add_definitions(-DPYTHON_PROGRAM_NAME="${MINICONDA3_PATH}/bin/python")

# Compiler settings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_definitions(-Wall -Wextra)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  add_definitions(/W2)
endif()

# Set output paths for libraries
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
GENERATE_SUBDIRS(ALL_LIBRARIES ${LIBRARIES_PATH} ${PROJECT_BINARY_DIR}/libraries)

# Set output paths for exetuables
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
GENERATE_SUBDIRS(ALL_EXECUTABLES ${EXECUTABLES_PATH} ${PROJECT_BINARY_DIR}/executables)

# Add shader path as subdirectory to have it available in project tree
if(EXISTS ${SHADERS_PATH})
        add_subdirectory(${SHADERS_PATH})
endif()

# Printing of Python values
file (COPY "${CMAKE_MODULE_PATH}/gdb_prettyprinter.py" DESTINATION ${PROJECT_BINARY_DIR})
