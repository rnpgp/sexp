# Copyright (c) 2022, [Ribose Inc](https://www.ribose.com).
# All rights reserved.
# This file is a part of RNP sexp library
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# 3.14 -- FetchContent
cmake_minimum_required(VERSION 3.14)

include(cmake/version.cmake)
determine_version("${CMAKE_CURRENT_SOURCE_DIR}" SEXP)

# project name, etc
project(sexp
  VERSION "${SEXP_VERSION}"
  LANGUAGES C CXX
  DESCRIPTION "S-expressions parser and generator C++ library, fully compliant to [https://people.csail.mit.edu/rivest/Sexp.txt]"
)

option(WITH_SEXP_TESTS "Build tests" ON)
option(WITH_SEXP_CLI "Build sexp console application" ON)
option(WITH_SANITIZERS "Enable ASAN and other sanitizers" OFF)
option(WITH_COVERAGE "Enable coverage report" OFF)
option(DOWNLOAD_GTEST "Download googletest" ON)

include(GNUInstallDirs)
include(CheckCXXSourceCompiles)


if (WITH_SANITIZERS)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Sanitizers work with clang compiler only.")
    endif()
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        message(STATUS "Forcing build type to Debug for sanitizers")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type. Forced to Debug" FORCE)
        set(WITH_TESTS ON CACHE STRING "BForced to ON" FORCE)
    endif()
endif()

if (WITH_COVERAGE)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(FATAL_ERROR "Coverage works with GNU compiler only.")
    endif()
    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        message(STATUS "Forcing build type to Debug for coverage")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type. Forced to Debug" FORCE)
        set(WITH_TESTS ON CACHE STRING "BForced to ON" FORCE)
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Defaulting build type to Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "Building ${CMAKE_BUILD_TYPE} configuration")

if (WITH_SANITIZERS)
  add_compile_options(-fsanitize=leak,address,undefined -fno-omit-frame-pointer -fno-common -O1)
  link_libraries(-fsanitize=leak,address,undefined)
endif(WITH_SANITIZERS)

if (WITH_COVERAGE)
    add_compile_options(--coverage -O0)
    link_libraries(--coverage)
endif(WITH_COVERAGE)

# set warning flags at the top level
if(NOT MSVC)
  add_compile_options(
    -Wall -Wextra
    -Wunreachable-code -Wpointer-arith
    -Wmissing-declarations
  )
# relax some warnings a bit
  add_compile_options(
    -Wno-pedantic
    -Wno-ignored-qualifiers
    -Wno-unused-parameter
    -Wno-missing-field-initializers
  )

endif(NOT MSVC)


CHECK_CXX_SOURCE_COMPILES(
    "#include <memory>
    int main(int argc, char**argv) {
        return 0;
    }
    std::unique_ptr<int> test(void) {
        std::unique_ptr<int> p(new int);
        return p;
    }"
    CAN_RETURN_UNIQUE_PTR
)

if(CAN_RETURN_UNIQUE_PTR)
    add_compile_definitions(RETURN_UNIQUE_PTR)
endif(CAN_RETURN_UNIQUE_PTR)

add_library(sexp STATIC
    "src/sexp-input.cpp"
    "src/sexp-output.cpp"
    "src/sexp-object.cpp"
    "src/sexp-simple-string.cpp"
    "src/sexp-char-defs.cpp"
    "src/sexp-error.cpp"
    "src/ext-key-format.cpp"
    "include/sexp/sexp.h"
    "include/sexp/sexp-error.h"
    "include/sexp/ext-key-format.h"
)

target_compile_features(sexp PUBLIC cxx_std_11)
target_include_directories (sexp PUBLIC include)
set_target_properties(sexp PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WITH_SEXP_CLI)
    add_executable (sexp-cli
                    src/sexp-main.cpp
                    include/sexp/sexp.h
                    include/sexp/sexp-error.h
    )
    target_include_directories (sexp-cli PUBLIC include)
    target_link_libraries(sexp-cli PRIVATE sexp)
    target_compile_features(sexp-cli PUBLIC cxx_std_11)
    set_target_properties(sexp-cli PROPERTIES RUNTIME_OUTPUT_NAME sexp)
endif(WITH_SEXP_CLI)


if(WITH_SEXP_TESTS)
    CHECK_CXX_SOURCE_COMPILES(
       "#include <iostream>
        #include <type_traits>
        struct test_struct {
            std::string str;
        };
        int main() {
            bool b = std::is_copy_constructible<test_struct>::value;
            return b ? 0 : -1 ;
        }"
        CAN_USE_NEW_GTEST
    )

    if(CAN_USE_NEW_GTEST)
        set(GTEST_TAG "release-1.12.1")
    else(CAN_USE_NEW_GTEST)
        if(WITH_COVERAGE)
            message(FATAL_ERROR "Coverage requires newer version of GTest that won't compile with your toolset")
        endif(WITH_COVERAGE)
        set(GTEST_TAG "release-1.8.1")
    endif(CAN_USE_NEW_GTEST)

    include(CTest)
    enable_testing()
    include(GoogleTest)

    set(SEXP_SAMPLES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/samples")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sexp-samples-folder.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/include/sexp-samples-folder.h"
        @ONLY
    )

    add_executable(sexp-tests
        "tests/src/baseline-tests.cpp"
        "tests/src/exception-tests.cpp"
        "tests/src/primitives-tests.cpp"
        "tests/src/g10-compat-tests.cpp"
        "tests/src/g23-compat-tests.cpp"
        "tests/src/g23-exception-tests.cpp"
        "tests/src/compare-files.cpp"
        "tests/include/sexp-tests.h"
    )

    if(DOWNLOAD_GTEST)
        message(STATUS "Fetching googletest")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        ${GTEST_TAG}
        )
        # maintain compiler/linker settings on Windows
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # do not install gtest
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        # explicitely disable unneeded gmock build
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(googletest)
        set(GTestMain gtest_main)
        set(GTest gtest)
    else(DOWNLOAD_GTEST)
        find_package(GTest REQUIRED)
        set(GTestMain GTest::Main)
        set(GTest GTest::GTest)

        if(NOT MSVC)
            target_link_libraries(sexp-tests PRIVATE pthread)
        endif(NOT MSVC)
    endif(DOWNLOAD_GTEST)

    target_link_libraries(sexp-tests PRIVATE
        sexp
         ${GTestMain}
         ${GTest}
    )

    target_include_directories(sexp-tests PRIVATE
        tests/include
        include
        "${GTEST_INCLUDE_DIRS}"
    )

    target_compile_features(sexp-tests PUBLIC cxx_std_11)

    gtest_discover_tests(sexp-tests)
endif(WITH_SEXP_TESTS)

set(CONFIGURED_PC "${CMAKE_CURRENT_BINARY_DIR}/sexp.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sexp.pc.in"
    "${CONFIGURED_PC}"
    @ONLY
)

install(TARGETS sexp DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES "${CONFIGURED_PC}" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
if(WITH_SEXP_CLI)
    install(TARGETS sexp-cli DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif(WITH_SEXP_CLI)
