cmake_minimum_required(VERSION 3.10)
project(sbrl)

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS -Wall)
set(CMAKE_CXX_STANDARD 98)

include_directories(.)

add_executable(sbrl
        rule.h
        mytime.h
        bit_vector.h
        pysbrl.h
        utils.h
        predict.c
        pysbrl.c
        rulelib.c
        sbrlmod.c
        train.c
        utils.c
        save_load.c
        bit_vector.c)

find_package(GSL REQUIRED)    # See below (2)
target_link_libraries(sbrl GSL::gsl GSL::gslcblas)