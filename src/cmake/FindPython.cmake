#
# Try to find Python library and include path.
# as well as PythonInterpreter
# Once done this will define
#
# PYTHON_FOUND
# PYTHON_LIBRARY
# PYTHON_LIBRARIES
# PYTHON_INCLUDE_PATH
# PYTHON_EXECUTABLE
#

##############################################################################
#          Find the right directory of python within miniconda               #
##############################################################################

SET(MINICONDA_SEARCH_PATHS $ENV{HOME}/miniconda3)
MESSAGE(WARNING "Miniconda search path: " ${MINICONDA_SEARCH_PATHS})

SET(PYTHON_SEARCH_PATH ${MINICONDA_SEARCH_PATHS}/pkgs/python-3.5.1-0)
MESSAGE(WARNING "Python search path: " ${PYTHON_SEARCH_PATH})


SET( PYTHON_INCLUDE_DIRS ${PYTHON_SEARCH_PATH}/include/python3.5m/)
FIND_LIBRARY( PYTHON_LIBRARY
        NAMES
            libpython3.5m.so
        PATHS
            ${PYTHON_SEARCH_PATH}/lib/
        DOC
            "The python library")
SET(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
SET(PYTHON_EXECUTABLE ${PYTHON_SEARCH_PATH}/bin/python3.5m)
MESSAGE(WARNING "Python include: " ${PYTHON_INCLUDE_DIRS})
MESSAGE(WARNING "Python library: " ${PYTHON_LIBRARY})
MESSAGE(WARNING "Python interpreter: " ${PYTHON_EXECUTABLE})

IF (PYTHON_INCLUDE_DIRS)
    SET( PYTHON_FOUND 1 CACHE STRING "Set to 1 if PYTHON is found, 0 otherwise")
ELSE (PYTHON_INCLUDE_PATH)
    SET( PYTHON_FOUND 0 CACHE STRING "Set to 1 if PYTHON is found, 0 otherwise")
ENDIF (PYTHON_INCLUDE_DIRS)

MARK_AS_ADVANCED( PYTHON_FOUND )