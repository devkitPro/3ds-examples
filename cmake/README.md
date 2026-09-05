# CMake template

This is a copy of the [textured_cube](/graphics/gpu/textured_cube/) example that shows how to take advantage of devkitPro's built-in CMake support.

Start with the following commands:

```sh
# Configure project
$DEVKITPRO/portlibs/3ds/bin/arm-none-eabi-cmake -B build -DCMAKE_BUILD_TYPE=Release .

# Build project
$DEVKITPRO/portlibs/3ds/bin/arm-none-eabi-cmake --build build --config Release

# Install built binary under "{current directory}/bin"
$DEVKITPRO/portlibs/3ds/bin/arm-none-eabi-cmake --install build --prefix $(pwd)
```

Refer to [CMakeLists.txt](CMakeLists.txt) for more informations on how to use CMake for your project.