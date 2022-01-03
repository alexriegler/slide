cmake_minimum_required (VERSION 3.8)

project ("slide")

add_library (slide INTERFACE)
target_include_directories (slide INTERFACE slide)

find_package (Catch2 REQUIRED)

add_executable (slide_test tests/test.cpp)

if (MSVC)
    target_compile_options (slide_test PRIVATE /std:c++latest)
else ()
    target_compile_options (slide_test PRIVATE -std=c++2b)
endif ()

target_link_libraries (slide_test PRIVATE slide Catch2::Catch2)