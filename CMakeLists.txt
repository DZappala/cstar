cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
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

project(cstar LANGUAGES CXX VERSION 1.0.0)
find_package(ftxui 5 REQUIRED)
add_executable(cstar ${SOURCE_FILES} ${HEADERS})

#add_executable(cstar main.cpp)
#        commdelay.cpp
#        interpret.cpp)

target_link_libraries(cstar
        PUBLIC ftxui::screen
        PUBLIC ftxui::dom
        PUBLIC ftxui::component
)
