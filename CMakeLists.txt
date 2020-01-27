cmake_minimum_required(VERSION 3.16)
project(mdmaker LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}
    markdownmaker.h
	main.cpp
    markdownmaker.cpp
	)