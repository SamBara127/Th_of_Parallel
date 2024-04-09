cmake_minimum_required(VERSION 3.1)

# Устанавливаем имя проекта
project(Task_2.1)

# Устанавливаем стандарт языка C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Включаем поддержку OpenMP
find_package(OpenMP REQUIRED)

# Добавляем флаги OpenMP для компилятора и линковщика
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# Добавляем исполняемый файл
add_executable(app.out Source.c)

# Линкуем библиотеки OpenMP с нашим исполняемым файлом
target_link_libraries(app.out PRIVATE OpenMP::OpenMP_CXX)

