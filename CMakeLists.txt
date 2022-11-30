cmake_minimum_required(VERSION 3.6)

set (CMAKE_CXX_STANDARD 20)

project(pivideo VERSION 0.2.0)

#set(CMAKE_TOOLCHAIN_FILE arm-linux-gnueabihf.cmake)

#set(BOARD_IP pizero)
set(BOARD_IP raspberrypi)

add_executable(${PROJECT_NAME} 
        src/main.cpp
)

set(TARGET_BINARY ${PROJECT_NAME})

#Custom target for Only Building
#add_custom_target(build ALL)

#Custom target for build and debug with GDB
add_custom_target(build_and_deploy ALL DEPENDS ${TARGET_BINARY})

add_custom_command(TARGET build_and_deploy POST_BUILD
        COMMAND scp ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_BINARY} ${BOARD_IP}:
)