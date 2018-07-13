cmake_minimum_required(VERSION 3.6)
project(cpp)

set(CMAKE_CXX_FLAGS -Wall)
set(CMAKE_CXX_STANDARD 11)

add_executable(cpp
        main.cpp
        bit_vector.h
        bit_vector.cpp
        pysbrl.h
        pysbrl.cpp
        rule.h
        rulelib.cpp
        save_load.cpp
        train.cpp
        predict.cpp
        utils.h
        utils.cpp
        )

find_package(GSL REQUIRED)    # See below (2)
target_link_libraries(cpp GSL::gsl GSL::gslcblas)