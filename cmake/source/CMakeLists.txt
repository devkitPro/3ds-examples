# Depending on the structure of your project, a CMakeLists.txt might handle multiple binaries, or just one.
# In our case, we only need to compile one example, so this is irrelevant.

# Let's specify the GPU shader sources, to be compiled into a single shader binary.
ctr_add_shader_library(vshader # This is the name used for the #include directive.
    vshader.v.pica
    # May add more, a geometry shader for example.
)

# Next, we have to tell CMake that we want to embed the shaders in the binary, so we can use them in our application.
# This command is what enables us to simply #include the shaders in C code.
# It's also possible to skip this step, then we would have to load the shader binary externally (eg. through a file).
dkp_add_embedded_binary_library(textured_cube_shaders vshader)

# At this point, we have to setup assets. We have to specify each asset separately.
ctr_add_graphics_target(kitten IMAGE # Use "CUBEMAP" for cubemaps.
    INPUTS ../gfx/kitten.png         # Relative path(s) to texture(s).
    OPTIONS -f auto-etc1 -z auto     # These are the options you would normally write to the .t3s file.
)

# Once again, we have to tell CMake that the assets have to be embedded in the binary.
dkp_add_embedded_binary_library(textured_cube_gfx kitten)

# Now we can configure the actual application. This step is standard CMake.
add_executable(textured_cube
    main.c
    # May add more source files.
)

# When embedding files, we have to link to them.
# To use portlibs, install them through dkp-pacman and add them here. CMake knows where to find them.
target_link_libraries(textured_cube PRIVATE
    textured_cube_shaders
    textured_cube_gfx
    citro3d
)

# Although not necessary, one might want to setup an smdh file.
ctr_generate_smdh(textured_cube_smdh
    NAME "textured_cube"
    DESCRIPTION "Textured cube PICA200 GPU example"
    AUTHOR "devkitPro"
    # This is optional.
    # ICON path/to/icon.png
)

# Finally, we can turn our application into 3DSX format.
ctr_create_3dsx(textured_cube
    SMDH textured_cube_smdh
    # If you are using romfs, specify a folder with its content.
    # ROMFS path/to/romfs
)

# One might also want to specify an install command for the 3DSX file.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/textured_cube.3dsx TYPE BIN)