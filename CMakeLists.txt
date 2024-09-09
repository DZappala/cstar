cmake_minimum_required(VERSION 3.5)
project(cstar)

# set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAC -std=c++11")
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
