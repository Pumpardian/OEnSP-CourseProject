cmake_minimum_required(VERSION 3.21)
project(LockFree_Queue)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SOURCES
    main.cpp
    queue/node.h
    queue/hazardPointerManager.h
    queue/queue.h
    )

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})