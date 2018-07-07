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
        predict.cpp
        pysbrl.cpp
        rulelib.cpp
        sbrlmod.cpp
        train.cpp
        utils.cpp
        save_load.cpp
        bit_vector.cpp)

find_package(GSL REQUIRED)    # See below (2)
target_link_libraries(sbrl GSL::gsl GSL::gslcblas)