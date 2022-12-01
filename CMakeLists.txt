cmake_minimum_required(VERSION 3.21)
project(Exercises_DB C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(Exercises_DB
        examples/bf_main.c
        examples/hp_main.c
        examples/ht_main.c
        include/bf.h
        include/hp_file.h
        include/ht_table.h
        include/record.h
        src/hp_file.c
        src/ht_table.c
        src/record.c)
