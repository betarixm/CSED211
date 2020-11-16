cmake_minimum_required(VERSION 3.16)
project(CSED211 C)

set(CMAKE_C_STANDARD 99)
add_executable(TEST Lab/test.c)

add_executable(ASSN4-CSIM ASSN/ASSN4/csim.c ASSN/ASSN4/cachelab.c ASSN/ASSN4/cachelab.h)
add_executable(ASSN4-TRANS ASSN/ASSN4/test-trans.c ASSN/ASSN4/trans.c ASSN/ASSN4/cachelab.c ASSN/ASSN4/cachelab.h)
