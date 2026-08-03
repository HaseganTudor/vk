cmake_minimum_required(VERSION 3.20)

project(VulkanWin32)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

add_executable(VulkanWin32
    src/main.cpp
    src/renderer.cpp
)

target_link_libraries(VulkanWin32 PRIVATE
    Vulkan::Vulkan
)
