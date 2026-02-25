cmake_minimum_required(VERSION 3.10)
project(BigIntegerLIB VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Опции сборки
option(BUILD_TESTS "Собирать тесты" ON)
option(BUILD_CLI "Собирать C++/CLI прослойку" OFF)

# Пути к заголовочным файлам
include_directories(include)

# Основная библиотека
add_library(big_integer 
    src/D512.cpp
    # src/D256.cpp
)

# Оптимизации в зависимости от компилятора
if(MSVC)
    target_compile_options(big_integer PRIVATE /O2 /arch:AVX2)
    target_compile_definitions(big_integer PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(big_integer PRIVATE -O3 -march=native -Wall -Wextra)
endif()

# Тесты
if(BUILD_TESTS)
    add_executable(d512_test main.cpp)
    target_link_libraries(d512_test big_integer)
endif()

# C++/CLI прослойка
if(BUILD_CLI AND MSVC)
    add_library(big_integer_cli SHARED cli/BigDigitCLI.cpp)
    target_link_libraries(big_integer_cli big_integer)
    set_target_properties(big_integer_cli PROPERTIES
        COMPILE_FLAGS "/clr"
        LINK_FLAGS "/clr"
    )
endif()