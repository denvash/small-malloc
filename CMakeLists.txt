cmake_minimum_required(VERSION 3.15)
project(OS_HW2_WET)

set(CMAKE_CXX_STANDARD 11)
set(GCC "-std=c++11 -Wall")

add_definitions(${GCC})

#add_executable(malloc1
#        malloc_1.cpp
#        )
#
#add_executable(malloc2
#        malloc_2.cpp
#        malloc_2.h
#        )
#
#add_executable(malloc3
#        malloc_3.cpp
#        )
#
#add_executable(testMain2
#        tests/mainTest2.cpp
#        malloc_2.h
#        malloc_2.cpp
#        )

add_executable(testMain3
        tests/mainTest3.cpp
        malloc_3.h
        malloc_3.cpp
        )