# This is the root CMakeLists.txt file. In this file we will configure the project as a whole, so we can
# deal with binary-specific configuration in other CMakeLists.txt files.
# This helps keeping the project well organized, but feel free to do your own structuring if you want.

# First, we need to specify the CMake version supported. "3.13" is the lowest supported by the toolchain.
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# Next, we would like to specify the version of C to be used. As of writing this devkitARM supports C23,
# so let's use that. For C++ you might want to tweak the "CXX" version of the variable.
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

# We can now setup our project. A single project may produce multiple different binaries. While not strictly
# necessary, it's recommended to specify which languages our project uses. In our case, it's C and ASM, as
# we need GPU shaders, and those count as assembly sources. 
project(textured_cube C ASM)

# And that's it for this file. Another thing one might want to do is setup external, non-portlibs dependencies
# that are global to the whole project, but that is out of the scope for this example.
# For now, let's move to the other CMakeLists.txt file under source/, so that we can setup our homebrew.
add_subdirectory(source)