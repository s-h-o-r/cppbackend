cmake_minimum_required(VERSION 3.11)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(BookManagerLib STATIC
    src/book_manager.h
    src/book_manager.cpp
)

target_include_directories(BookManagerLib PUBLIC CONAN_PKG::boost)
target_link_libraries(BookManagerLib PUBLIC CONAN_PKG::libpqxx PRIVATE Threads::Threads)
target_link_libraries(BookManagerLib PUBLIC CONAN_PKG::boost PRIVATE Threads::Threads)

add_executable(book_manager
    src/main.cpp
)

add_executable(book_manager_tests
    tests/test.cpp
)

target_link_libraries(book_manager PRIVATE BookManagerLib)
target_link_libraries(book_manager_tests CONAN_PKG::catch2 BookManagerLib)
