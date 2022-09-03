cmake_minimum_required(VERSION 3.16)

project(ExistOS-Dump-Brige VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)

include_directories(./)

aux_source_directory(. EDB_SRCS)

add_executable(edb ${EDB_SRCS})