# CMakeLists.txt

add_executable(sdl3_vulkan_hpp_clear sdl3_vulkan_hpp_clear.cpp)
target_link_libraries(sdl3_vulkan_hpp_clear PRIVATE SDL3::SDL3)
