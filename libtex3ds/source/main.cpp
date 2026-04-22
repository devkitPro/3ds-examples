#define STB_IMAGE_IMPLEMENTATION
#include "libtex3ds.h"
#include "stb_image.h"
#include <iostream>
#include <3ds.h>
#include <stdio.h>
#include <cstdint>

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    int w, h, channels;
    uint8_t* data = stbi_load("sdmc:/input.png", &w, &h, &channels, 4);
    if (!data) {
        printf("Erreur lors du chargement de l'image.\n");
        return 1;
    }
    printf("Image loaded: %d x %d, channels: %d\n", w, h, channels);

    Tex3DS::Image img(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const uint8_t* src = data + (y * w + x) * 4;
            Tex3DS::RGBA& px = img.pixels[y * img.stride + x];
            px.r = src[0]; px.g = src[1]; px.b = src[2]; px.a = src[3];
        }
    }
    stbi_image_free(data);
    printf("Image converted to RGBA format.\n");

    Tex3DS::Params params;
    params.input_img      = std::move(img);
    params.output_path    = "sdmc:/output.t3x";
    params.process_format = Tex3DS::RGBA4444;

    if (Tex3DS::Process(params)) {
        printf("Processing completed.\n");
    } else {
        printf("Error occurred during processing.\n");
    }

    printf("Press START to exit.\n");
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) break;
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
