cmake_minimum_required(VERSION 3.16)
project(mdmaker LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

if(WIN32 OR MSVC)
    add_compile_options(-DWINDOWS_OS)
endif()

add_executable(${PROJECT_NAME}
    markdownmaker.h
	main.cpp
    markdownmaker.cpp
	)
