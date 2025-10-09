cmake_minimum_required(VERSION 3.28)
project(vktst)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME} Main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glfw)