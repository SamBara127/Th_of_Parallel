cmake_minimum_required(VERSION 3.0)
project(Task_1)

option(USE_FLOAT "Use float for array elements" OFF)
option(USE_DOUBLE "Use double for array elements" OFF)

if(USE_FLOAT)
    add_definitions(-DFLOAT)
elseif(USE_DOUBLE)
    add_definitions(-DDOUBLE)
endif()

add_executable(app.out Source.c)

# m - это библиотека математических функций, 
# требуется при использовании функций из заголовочного файла math.h.
target_link_libraries(app.out m)