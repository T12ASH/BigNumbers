cmake_minimum_required(VERSION 3.10)
project(BigIntegerLIB)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

if(MSVC)
    add_compile_options(/W3 /wd4244 /wd4146)
endif()

add_executable(d512_test 
    main.cpp
    src/F256.cpp
    src/D256.cpp
    src/F512.cpp
    src/D512.cpp
)

target_include_directories(d512_test PRIVATE include)