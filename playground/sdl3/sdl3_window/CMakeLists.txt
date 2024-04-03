# CMakeLists.txt

add_executable(sdl3_window sdl3_window.cpp)
target_link_libraries(sdl3_window PRIVATE SDL3::SDL3)
