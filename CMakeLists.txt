cmake_minimum_required(VERSION 3.14)
project(yetanotherlogger VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if yetanotherlogger is the top-level project
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(YAL_MASTER_PROJECT ON)
else()
    set(YAL_MASTER_PROJECT OFF)
endif()

# Options
option(YAL_BUILD_EXAMPLES "Build examples" ${YAL_MASTER_PROJECT})
option(YAL_BUILD_TESTS "Build tests" ${YAL_MASTER_PROJECT})
option(YAL_USE_ASAN "Enable AddressSanitizer (Debug only)" ${YAL_MASTER_PROJECT})

# Define the interface library
add_library(yetanotherlogger INTERFACE)
add_library(yetanotherlogger::logger ALIAS yetanotherlogger)

# Set the include directories
target_include_directories(yetanotherlogger INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Sanitizers configuration
if(YAL_USE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer for yetanotherlogger")
    if(MSVC)
        target_compile_options(yetanotherlogger INTERFACE /fsanitize=address)
    else()
        target_compile_options(yetanotherlogger INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(yetanotherlogger INTERFACE -fsanitize=address)
    endif()
endif()

# Example usage
if(YAL_BUILD_EXAMPLES)
    add_executable(example_usage examples/basic_usage.cpp)
    target_link_libraries(example_usage PRIVATE yetanotherlogger)
endif()

# Testing infrastructure
if(YAL_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(unit_tests tests/test_logger.cpp)
    target_link_libraries(unit_tests PRIVATE yetanotherlogger Catch2::Catch2WithMain)
    
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
install(TARGETS yetanotherlogger EXPORT yetanotherlogger-targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT yetanotherlogger-targets
    NAMESPACE yetanotherlogger::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/yetanotherlogger
)
