cmake_minimum_required(VERSION 2.8)
project(sbrl)

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
set(CMAKE_C_FLAGS -Wall)
set(CMAKE_C_STANDARD 99)

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
        train.c
        utils.c
        save_load.c
        bit_vector.c
        test.c
        )

find_package(GSL REQUIRED)    # See below (2)
target_link_libraries(sbrl -lm GSL::gsl GSL::gslcblas)