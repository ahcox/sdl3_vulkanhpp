# A bunch of C++ Code
# Let's try to modularise and reuse it without giving it some overarching name.
# Let's try to keep it all building over time.
# Every library and app added outside the core libs and their tests should be
# excluded from the build by default to make it easier to pick up and add
# something new over the years without immediately being hit by build errors
# in the old code due to changes in tools/platforms.
# Somewhere (in free online CI?) a full build with all modules enabled should
# flag the rot when it happens.

cmake_minimum_required(VERSION 3.5)
project(monorepo)

# Create an option to switch between a system sdl library and a local SDL library
option(MONOREPO_LOCAL_SDL3 "Use SDL stored under thirdparty as a submodule" ON)
# Create an option to switch between platform vulkan.hpp headers and a local submodule
# I don't need to go in and build the header generators -
option(MONOREPO_LOCAL_VULKAN_HPP "Use vulkan.hpp stored under thirdparty as a submodule" ON)

# List of all the libraries and apps in the monorepo with an option
# to enable/disable them.
option(MONOREPO_SDL3_CALLBACKS "Simple app responding to callbacks rather than having a main function." OFF)
option(MONOREPO_SDL3_WINDOW "Simple app opening an SDL3 window and responding to callbacks." OFF)
option(MONOREPO_SDL3_VULKAN_HPP_CLEAR "Simple app opening an SDL3 window, binding Vulkan to it, and clearing the surface with Vulkan." OFF)

if(MONOREPO_LOCAL_SDL3)
    add_subdirectory(thirdparty/sdl3)
else()
    # 1. Look for a SDL3 package,
    # 2. look for the SDL3-shared component, and
    # 3. fail if the shared component cannot be found.
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif()

# I don't need to go in and build the header generators, I just use the generated headers already in the repo.
if(MONOREPO_LOCAL_VULKAN_HPP)
    add_subdirectory(thirdparty/vulkan-hpp)
else()
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS vulkan-hpp)
endif()


if(MONOREPO_SDL3_CALLBACKS)
    add_subdirectory(playground/sdl3/sdl3_main_callbacks)
endif()

if(MONOREPO_SDL3_WINDOW)
    add_subdirectory(playground/sdl3/sdl3_window)
endif()

if(MONOREPO_SDL3_VULKAN_HPP_CLEAR)
    add_subdirectory(playground/sdl3/sdl3_vulkan_hpp_clear)
endif()
