cmake_minimum_required(VERSION 3.28)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
        GIT_TAG cdf28903a7781f97ba94d30b79c3a4b0c97ccce7
)
FetchContent_MakeAvailable(ftxui)

project(cstar LANGUAGES CXX VERSION 3.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAC -std=c++23 -stdlib=libc++")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g -ggdb -ggnu-pubnames -std=c++11")

set(HEADERS cs_global.h cs_basic.h cs_block.h cs_compile.h cs_defines.h cs_errors.h
        cs_interpret.h cs_exec.h
        cs_PreBuffer.h)

set(SOURCE_FILES lexer.cpp
        errors.cpp
        interpret.cpp
        expression.cpp
        block.cpp
        statement.cpp
        program.cpp
        exec.cpp
        execlib.cpp
        displays.cpp)

add_executable(cstar ${SOURCE_FILES} ${HEADERS})

#add_executable(cstar main.cpp)
#        commdelay.cpp
#        interpret.cpp)

target_link_libraries(cstar
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
)
