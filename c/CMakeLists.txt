cmake_minimum_required(VERSION 3.10)
project(sbrl C)

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_C_FLAGS -Wall)
set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(sbrl
        rule.h
        mytime.h
        predict.c
        pysbrl.c
        pysbrl.h
        rulelib.c
        sbrlmod.c
        train.c
        utils.h
        utils.c
        save_load.c
        bit_vector.h
        bit_vector.c)

find_package(GSL REQUIRED)    # See below (2)
target_link_libraries(sbrl GSL::gsl GSL::gslcblas)