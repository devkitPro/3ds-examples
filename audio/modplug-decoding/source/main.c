#include <3ds.h>
#include <stdio.h>

#include <sys/stat.h>
#include <malloc.h>

#include <string.h>

#include <libmodplug/modplug.h>

typedef struct
{
    ModPlugFile *plug;
    ModPlug_Settings settings;
}  ModplugDecoder;

typedef struct
{
    void* data;
    size_t size;
} StaticDataBuffer;

void staticDataBufferInit(StaticDataBuffer* buffer, void* data, size_t size)
{
    memset(buffer, 0, sizeof(StaticDataBuffer));

    buffer->data = (int16_t*)linearAlloc(size);
    buffer->size = size;

    memcpy(buffer->data, data, size);
}

void staticDataBufferDestroy(StaticDataBuffer* buffer)
{
    if (buffer)
        linearFree(buffer->data);
}

int main(int argc, char **argv)
{
    gfxInitDefault();

    consoleInit(GFX_TOP, NULL);

    ndspInit();
    romfsInit();

    ModplugDecoder decoder;

    /* set up ModPlug settings */
    decoder.settings.mFlags = MODPLUG_ENABLE_OVERSAMPLING | MODPLUG_ENABLE_NOISE_REDUCTION;
    decoder.settings.mChannels = 2;
    decoder.settings.mBits = 16;
    decoder.settings.mFrequency = 44100;
    decoder.settings.mResamplingMode = MODPLUG_RESAMPLE_LINEAR;

    /* Fill with modplug defaults */
    decoder.settings.mStereoSeparation = 128;
    decoder.settings.mMaxMixChannels = 32;
    decoder.settings.mReverbDepth = 0;
    decoder.settings.mReverbDelay = 0;
    decoder.settings.mBassAmount = 0;
    decoder.settings.mBassRange = 0;
    decoder.settings.mSurroundDepth = 0;
    decoder.settings.mSurroundDelay = 0;
    decoder.settings.mLoopCount = -1;

    ModPlug_SetSettings(&decoder.settings);

    /* Get the file size via stat */

    struct stat fileStat;
    stat("romfs:/space_debris.mod", &fileStat);
    size_t bufferSize = fileStat.st_size;

    /* Open the file for reading */

    FILE* file = fopen("romfs:/space_debris.mod", "rb");

    /* Read the file into a buffer */

    void* buffer = (void*)malloc(bufferSize);
    fread(buffer, bufferSize, 1, file);

    /* Load the ModPlug file from the buffer */

    decoder.plug = ModPlug_Load(buffer, bufferSize);

    /* Free the useless buffer now and close the file handle */

    free(buffer);
    fclose(file);

	if (decoder.plug == 0)
        printf("Well shit, could not load file!\n");
    else
        ModPlug_SetMasterVolume(decoder.plug, 128);

    /* Create a new ndspWaveBuf */

    ndspWaveBuf waveBuf;

    /*
    ** Create static buffer data and use
    ** a redundant size for the test.
    **
    ** == WARNING ==
    ** Do not actually do this.
    **
    ** The decoder size should be looping
    ** through calls to ModPlug_Read until nothing
    ** can be read anymore.
    **
    ** Memory *may* get exhausted if too much is allocated.
    ** Implement a way to check that, realloc, etc until a limit
    ** of streaming data is hit.
    */
    size_t decoderSize = 524288 * 50;
    void* decodedBuffer = (void*)malloc(decoderSize);

    size_t decoded = ModPlug_Read(decoder.plug, decodedBuffer, decoderSize);

    StaticDataBuffer staticDataBuffer;
    staticDataBufferInit(&staticDataBuffer, decodedBuffer, decoded);

    printf("Decoded Size: %zu\n", decoded);

    waveBuf.nsamples = ((decoded / 2) / 16 / 8);
    waveBuf.looping = false;

    waveBuf.data_pcm16 = (int16_t*)staticDataBuffer.data;
    DSP_FlushDataCache(waveBuf.data_pcm16, decoded);

    ndspChnReset(0);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(0, 44100);
    ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);

    printf("Playing modplug file!\n");
    ndspChnWaveBufAdd(0, &waveBuf);

    // Main loop
    while (aptMainLoop())
    {
        hidScanInput();

        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break; // break in order to return to hbmenu

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();

        //Wait for VBlank
        gspWaitForVBlank();
    }

    if (decoder.plug != 0)
        ModPlug_Unload(decoder.plug);

    staticDataBufferDestroy(&staticDataBuffer);

    romfsExit();

    ndspExit();

    gfxExit();

    return 0;
}
