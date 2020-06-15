#include <3ds.h>
#include <stdio.h>
#include <string.h>

static bool allowed = false;

// If you define this function, you can monitor/debug APT events
void _aptDebug(int a, int b)
{
	if (allowed)
		printf("_aptDebug(%d,%x)\n", a, b);
}

int main()
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	allowed = true;

	u32 aptbuf[0x400/4];

	printf("Press A to launch extrapad\n");
	printf("Press B to launch error\n");
	printf("Press X to launch appleted (Mii Editor)\n");
	printf("Press Y to launch memolib\n");
	printf("Press L to launch Photo Selector\n");
	printf("Press R to launch Sound Selector\n");
	printf("Press ^ to launch mint (eShop)\n");
	printf("Press < to launch software keyboard\n");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		gfxFlushBuffers();

		if (kDown)
		{
			NS_APPID appId;

			if (kDown & KEY_A) appId = APPID_EXTRAPAD;
			else if (kDown & KEY_B) appId = APPID_ERROR;
			else if (kDown & KEY_X) appId = APPID_APPLETED;
			else if (kDown & KEY_Y) appId = APPID_MEMOLIB;
			else if (kDown & KEY_L) appId = APPID_PNOTE_AP;
			else if (kDown & KEY_R) appId = APPID_SNOTE_AP;
			else if (kDown & KEY_UP) appId = APPID_MINT;
			else if (kDown & KEY_LEFT) appId = APPID_SOFTWARE_KEYBOARD;
			else continue;

			memset(aptbuf, 0, sizeof(aptbuf));
			aptLaunchLibraryApplet(appId, aptbuf, sizeof(aptbuf), 0);
			printf("Library applet exited\n");
		}
	}

	// Exit services
	allowed = false;
	gfxExit();
	return 0;
}
