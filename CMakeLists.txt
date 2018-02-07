cmake_minimum_required(VERSION 2.6)
project(n_gram_index)

SET(CMAKE_CXX_FLAGS "-std=c++0x -g")

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})


SET(PYTHON_EXECUTABLE /usr/bin/python3.5m)
SET(PYTHON_INCLUDE_PATH /usr/include/python3.5m)
SET(PYTHON_LIBRARIES /usr/lib/x86_64-linux-gnu/libpython3.5m.so)

INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/src")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/src/Python")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(CMAKE_SWIG_FLAGS "-Wall")

SET_SOURCE_FILES_PROPERTIES(src/Python/n_gram.i PROPERTIES CPLUSPLUS ON)
SET_SOURCE_FILES_PROPERTIES(src/Python/n_gram.i PROPERTIES SWIG_FLAGS "-builtin")

SWIG_ADD_MODULE(n_gram_index python src/NGram_p.h src/Python/n_gram.i src/Python/n_gram.cpp)
SWIG_LINK_LIBRARIES(n_gram_index ${PYTHON_LIBRARIES})


MESSAGE(STATUS "UnitTest: " ${CMAKE_CURRENT_BINARY_DIR}/tests/python_extension_test.py)
enable_testing()
add_test(NAME python_extension_test COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/python_extension_test.py  ${CMAKE_CURRENT_BINARY_DIR})
