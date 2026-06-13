cmake_minimum_required(VERSION 3.28)
project(vulkanTest)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/libs)

add_executable(${PROJECT_NAME} 
				src/Main.cpp
				src/engine-utils.cpp
				src/compute.cpp
				src/engine.cpp
				src/window.cpp)


target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES} glfw)
