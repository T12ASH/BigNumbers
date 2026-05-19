cmake_minimum_required(VERSION 3.15)
project(BigIntegerLIB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # важно для shared библиотек на Unix

# Опции сборки
option(BUILD_SHARED_LIBS "Build shared library (DLL/so)" ON)
option(BUILD_STATIC_LIBS "Build static library (LIB/a)" OFF)
option(INSTALL_DEV "Install development files" ON)

# Исходные файлы (автоматически собираем все .cpp из src)
file(GLOB_RECURSE LIB_SOURCES "src/*.cpp")
file(GLOB_RECURSE LIB_HEADERS "include/*.hpp")

# Создание основной библиотеки (тип определяется BUILD_SHARED_LIBS)
add_library(BigIntegerLIB ${LIB_SOURCES})
target_include_directories(BigIntegerLIB
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Для Windows: экспорт/импорт символов
if(WIN32)
    target_compile_definitions(BigIntegerLIB PRIVATE BUILDING_DLL)
    set_target_properties(BigIntegerLIB PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Отдельная статическая библиотека (если нужно)
if(BUILD_STATIC_LIBS)
    add_library(BigIntegerLIB_static STATIC ${LIB_SOURCES})
    target_include_directories(BigIntegerLIB_static
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    set_target_properties(BigIntegerLIB_static PROPERTIES OUTPUT_NAME BigIntegerLIB)
endif()

# Установка (для использования библиотеки из других проектов)
if(INSTALL_DEV)
    install(TARGETS BigIntegerLIB
        EXPORT BigIntegerLIBTargets
        RUNTIME DESTINATION bin      # DLL (Windows)
        LIBRARY DESTINATION lib      # .so (Linux/macOS)
        ARCHIVE DESTINATION lib      # .lib (Windows), .a (Unix)
    )
    if(BUILD_STATIC_LIBS)
        install(TARGETS BigIntegerLIB_static
            EXPORT BigIntegerLIBTargets
            ARCHIVE DESTINATION lib
        )
    endif()

    install(DIRECTORY include/ DESTINATION include)

    # Экспорт целей для find_package
    install(EXPORT BigIntegerLIBTargets
        FILE BigIntegerLIBTargets.cmake
        DESTINATION lib/cmake/BigIntegerLIB
    )

    # Создание конфигурационного файла
    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/BigIntegerLIBConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/BigIntegerLIBConfig.cmake
        INSTALL_DESTINATION lib/cmake/BigIntegerLIB
    )
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/BigIntegerLIBConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/BigIntegerLIBConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/BigIntegerLIBConfigVersion.cmake
        DESTINATION lib/cmake/BigIntegerLIB
    )
endif()