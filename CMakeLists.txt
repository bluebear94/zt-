CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(sca_e_kozet)

SET(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/build)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

## ===============================================



## ===============================================

INCLUDE_DIRECTORIES(include/)

SET(SOURCES
  src/main.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/SCA.cpp
  src/errors.cpp
  src/matching.cpp
  src/Rule.cpp
)

ADD_EXECUTABLE(sca_e_kozet ${SOURCES})
SET(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} --std=c++17 -Wall -Werror -pedantic -fno-exceptions -fno-rtti")
#TARGET_LINK_LIBRARIES(sca_e_kozet
#  -lncursesw
#)
