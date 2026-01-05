cmake_minimum_required(VERSION 3.14)
project(header_only_logger VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(USE_ASAN "Enable AddressSanitizer (Debug only)" ON)

# Define the interface library
add_library(header_only_logger INTERFACE)
add_library(header_only_logger::logger ALIAS header_only_logger)

# Set the include directories
target_include_directories(header_only_logger INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Sanitizers configuration
if(USE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer")
    if(MSVC)
        target_compile_options(header_only_logger INTERFACE /fsanitize=address)
    else()
        target_compile_options(header_only_logger INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(header_only_logger INTERFACE -fsanitize=address)
    endif()
endif()

# Example usage
if(BUILD_EXAMPLES)
    add_executable(example_usage examples/basic_usage.cpp)
    target_link_libraries(example_usage PRIVATE header_only_logger)
endif()

# Testing infrastructure
if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(unit_tests tests/test_logger.cpp)
    target_link_libraries(unit_tests PRIVATE header_only_logger Catch2::Catch2WithMain)
    
    include(Catch)
    catch_discover_tests(unit_tests)

    # Valgrind Check Target
    find_program(VALGRIND_PATH valgrind)
    if(VALGRIND_PATH)
        add_custom_target(valgrind-check
            COMMAND ${VALGRIND_PATH} --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./unit_tests
            DEPENDS unit_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running Valgrind on unit tests..."
        )
    endif()
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS header_only_logger EXPORT header_only_logger-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT header_only_logger-targets
    NAMESPACE header_only_logger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/header_only_logger
)
