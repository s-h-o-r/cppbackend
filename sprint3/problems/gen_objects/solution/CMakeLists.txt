cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/sdk.h
    src/model.h
    src/model.cpp
    src/tagged.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    src/logger.cpp
    src/logger.h
    src/player.h
    src/player.cpp
    src/app.h
    src/app.cpp
    src/cl_parser.h
    src/cl_parser.cpp
    src/ticker.h
    src/ticker.cpp
    src/loot_generator.h
    src/loot_generator.cpp
)

add_executable(game_server_tests
    tests/loot_generator_tests.cpp
    src/tagged.h
    src/model.h
    src/model.cpp
    src/loot_generator.h
    src/loot_generator.cpp
)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE CONAN_PKG::boost PRIVATE Threads::Threads)

target_include_directories(game_server_tests PRIVATE  CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads)


