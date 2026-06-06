cmake_minimum_required(VERSION 3.24)
project(DnDai)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SOURCES
        classes/ChatTab.cpp
        classes/ChatTab.hpp
        classes/LLamaInterface.cpp
        classes/LLamaInterface.hpp
        classes/ChatController.cpp
        classes/ChatController.hpp
        classes/CharList.cpp
        classes/CharList.hpp
        classes/DiceRoll.cpp
        classes/DiceRoll.hpp
)

include(FetchContent)

FetchContent_Declare(
        ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
        GIT_TAG        v6.1.9
)
FetchContent_MakeAvailable(ftxui)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

set(GGML_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE INTERNAL "" FORCE)
set(GGML_SHARED OFF CACHE INTERNAL "" FORCE)
set(LLAMA_SHARED OFF CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "" FORCE)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:char8_t- /Zc:__cplusplus")
endif()

FetchContent_Declare(
        llama.cpp
        GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
        GIT_TAG        b9460
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(llama.cpp)

add_library(DnDai_lib ${LIB_SOURCES})
target_link_libraries(DnDai_lib PRIVATE llama ggml ggml-base ggml-cpu)
target_link_libraries(DnDai_lib PUBLIC ftxui::screen ftxui::dom ftxui::component nlohmann_json::nlohmann_json)

add_executable(DnDai main.cpp)
target_link_libraries(DnDai PRIVATE DnDai_lib)

enable_testing()

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(run_tests tests/test_main.cpp)
target_link_libraries(run_tests PRIVATE DnDai_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)