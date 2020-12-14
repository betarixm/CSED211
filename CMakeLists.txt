cmake_minimum_required(VERSION 3.16)
project(CSED211 C)

set(CMAKE_C_STANDARD 99)

FILE(GLOB ASSN6_HEADERS ASSN/ASSN6/*.h)
FILE(GLOB ASSN6_SOURCES ASSN/ASSN6/*.c)

add_executable(TEST Lab/test.c)

add_executable(ASSN4-CSIM ASSN/ASSN4/csim.c ASSN/ASSN4/cachelab.c ASSN/ASSN4/cachelab.h)
add_executable(ASSN4-TRANS ASSN/ASSN4/test-trans.c ASSN/ASSN4/trans.c ASSN/ASSN4/cachelab.c ASSN/ASSN4/cachelab.h)

add_executable(ASSN5 ASSN/ASSN5/myint.c ASSN/ASSN5/myspin.c ASSN/ASSN5/mysplit.c ASSN/ASSN5/mystop.c ASSN/ASSN5/tsh.c)

add_executable(ASSN6 ${ASSN6_HEADERS} ${ASSN6_SOURCES})