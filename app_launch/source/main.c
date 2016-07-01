#include <stdio.h>
#include <string.h>
#include <3ds.h>

int main()
{
	gfxInitDefault(); // Init graphic stuff

	// We need these 2 buffers for APT_DoAppJump() later.
	u8 param[0x300];
	u8 hmac[0x20];

	// Loop as long as the status is not exit
	while(aptMainLoop())
	{
		// Scan hid shared memory for input events
		hidScanInput();

		if(hidKeysDown() & KEY_A) // If the A button got pressed, start the app launch
		{
			// Clear both buffers
			memset(param, 0, sizeof(param));
			memset(hmac, 0, sizeof(hmac));

			// Prepare for the app launch
			APT_PrepareToDoApplicationJump(0, 0x0004001000022400LL, 0); // *EUR* camera app title ID
			// Tell APT to trigger the app launch and set the status of this app to exit
			APT_DoApplicationJump(param, sizeof(param), hmac);
		}

		// Flush + swap framebuffers and wait for VBlank. Not really needed in this example
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();

	return 0;
}
