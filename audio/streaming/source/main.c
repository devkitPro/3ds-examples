#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <3ds.h>

#define SAMPLERATE 22050
#define SAMPLESPERBUF (SAMPLERATE / 30)
#define BYTESPERSAMPLE 4

//----------------------------------------------------------------------------
void fill_buffer(void *audioBuffer,size_t offset, size_t size, int frequency ) {
//----------------------------------------------------------------------------

	u32 *dest = (u32*)audioBuffer;

	for (int i=0; i<size; i++) {

		s16 sample = INT16_MAX * sin(frequency*(2*M_PI)*(offset+i)/SAMPLERATE);

		dest[i] = (sample<<16) | (sample & 0xffff);
	}

	DSP_FlushDataCache(audioBuffer,size);

}

//----------------------------------------------------------------------------
int main(int argc, char **argv) {
//----------------------------------------------------------------------------

	PrintConsole topScreen;
	ndspWaveBuf waveBuf[2];

	gfxInitDefault();

	consoleInit(GFX_TOP, &topScreen);

	consoleSelect(&topScreen);

	printf("libctru streaming audio\n");

	u32 *audioBuffer = (u32*)linearAlloc(SAMPLESPERBUF*BYTESPERSAMPLE*2);

	bool fillBlock = false;

	ndspInit();

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);

	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, SAMPLERATE);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	int notefreq[] = {
		262, 294, 339, 349, 392, 440,
		494, 440, 392, 349, 339, 294
	};

	int note = 4;

	memset(waveBuf,0,sizeof(waveBuf));
	waveBuf[0].data_vaddr = &audioBuffer[0];
	waveBuf[0].nsamples = SAMPLESPERBUF;
	waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
	waveBuf[1].nsamples = SAMPLESPERBUF;

	size_t stream_offset = 0;

	fill_buffer(audioBuffer,stream_offset, SAMPLESPERBUF * 2, notefreq[note]);

	stream_offset += SAMPLESPERBUF;

	ndspChnWaveBufAdd(0, &waveBuf[0]);
	ndspChnWaveBufAdd(0, &waveBuf[1]);

	printf("Press up/down to change tone\n");

	while(aptMainLoop()) {

		gfxSwapBuffers();
		gfxFlushBuffers();
		gspWaitForVBlank();

		hidScanInput();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kDown & KEY_DOWN) {
			note -= 1;

			if (note<0) note = sizeof(notefreq)/sizeof(notefreq[0])-1;
		}

		if (kDown & KEY_UP) {
			note += 1;

			if (note > (sizeof(notefreq)/sizeof(notefreq[0])-1)) note = 0;
		}

		if (waveBuf[fillBlock].status == NDSP_WBUF_DONE) {

			fill_buffer(waveBuf[fillBlock].data_pcm16, stream_offset, waveBuf[fillBlock].nsamples,notefreq[note]);

			ndspChnWaveBufAdd(0, &waveBuf[fillBlock]);
			stream_offset += waveBuf[fillBlock].nsamples;

			fillBlock = !fillBlock;
		}
	}

	ndspExit();

	linearFree(audioBuffer);

	gfxExit();
	return 0;
}
