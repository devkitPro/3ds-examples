#include <3ds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	gfxInitDefault();
	gfxSetWide(true); // Enable wide mode
	consoleInit(GFX_TOP, NULL);

	printf("\x1b[2;38HHello wide screen world!");
	printf("\x1b[4;1H");

	printf("800px wide mode can be used with the libctru console. All you need to do is call gfxSetWide(true);\n");
	printf("before initializing the console, and there you go! A nice 100x30 character console ready for use :)\n\n");

	printf("Don't forget: wide mode is not compatible with Old 2DS consoles (however bizarrely enough, it *is*\n");
	printf("compatible with New 2DS XL consoles. All other 3DS models seem to be compatible as well.\n");

	printf("\x1b[28;40HPress Start to exit.\n");

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
