cmake_minimum_required(VERSION 4.1)
project(DnDai)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
        main.cpp

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
FetchContent_GetProperties(llama.cpp)

set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(llama.cpp)

add_executable(DnDai ${SOURCES})

target_link_libraries(DnDai PRIVATE llama ggml ggml-base ggml-cpu)

target_link_libraries(DnDai PRIVATE
        ftxui::screen
        ftxui::dom
        ftxui::component
)

target_link_libraries(DnDai PRIVATE nlohmann_json::nlohmann_json)
