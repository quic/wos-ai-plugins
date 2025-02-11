cmake_minimum_required(VERSION 3.28)

project(qcom-code-gen-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

link_directories( "${CMAKE_CURRENT_LIST_DIR}/lib" )
include_directories("${CMAKE_CURRENT_LIST_DIR}/include")

add_executable(qcom-code-gen-server src/qcom-code-gen-server.cpp)
target_link_libraries(qcom-code-gen-server "${CMAKE_CURRENT_LIST_DIR}/lib/Genie.lib")

