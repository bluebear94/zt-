CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(sca_e_kozet)

SET(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/build)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

## ===============================================

FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem system)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

FIND_PACKAGE(Lua REQUIRED)
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

## ===============================================

INCLUDE_DIRECTORIES(include/)

SET(SOURCES
  src/errors.cpp
  src/PHash.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/matching.cpp
  src/verify_rule.cpp
  src/Rule.cpp
  src/sca_lua.cpp
  src/SCA.cpp
  src/main.cpp
)

ADD_EXECUTABLE(sca_e_kozet ${SOURCES})
SET(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} --std=c++17 -Wall -Werror -pedantic -fno-exceptions -fno-rtti")
TARGET_LINK_LIBRARIES(sca_e_kozet
  ${Boost_LIBRARIES} ${LUA_LIBRARIES}
)

# This works only with in-source builds. Sorry.
SET(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
ADD_CUSTOM_TARGET(
  atest
  COMMAND python3 ${TEST_DIR}/auto/test.py ${CMAKE_BINARY_DIR}/sca_e_kozet ${TEST_DIR}
  SOURCES ${TEST_DIR}/auto/test.py
)
ADD_DEPENDENCIES(atest sca_e_kozet)
