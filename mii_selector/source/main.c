#include <3ds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("Mii selector demo.\n");
	printf("Press A to bring up Mii selector with default settings.\n");
	printf("Press B to bring up Mii selector with custom settings.\n");
	printf("Press START to exit.\n");

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;

		bool didit = false;
		static MiiSelectorConf msConf;
		static MiiSelectorReturn msRet;
		static char miiname[36];
		static char miiauthor[30];

		if (kDown & KEY_A)
		{
			didit = true;
			// Ensure that the config is initalized and that all
			// defaults are set properly. (failing to do this to a new
			// MiiSelecorConf at least once before launching the miiSelector
			// can result in undefined behavior)
			miiSelectorInit(&msConf);
			miiSelectorLaunch(&msConf, &msRet);
		}

		if (kDown & KEY_B)
		{
			didit = true;
			miiSelectorInit(&msConf);
			// Sets title of Mii selector.
			miiSelectorSetTitle(&msConf, "This is a custom title!");
			// Choose and option to enable. Any options not listed will be disabled.
			// 0 can be used to select no option.
			miiSelectorSetOptions(&msConf, MIISELECTOR_CANCEL|MIISELECTOR_GUESTS|MIISELECTOR_TOP|MIISELECTOR_GUESTSTART);
			// Start on the Mii with the database index of 1.
			// This is a guest Mii since the option MIISELECTOR_GUESTSTART is enabled.
			// If MIISELECTOR_GUESTSTART was not enabled, this would be a user Mii.
			miiSelectorSetInitialIndex(&msConf, 1);
			// Blacklist main user Mii
			miiSelectorBlacklistUserMii(&msConf, 0);
			miiSelectorLaunch(&msConf, &msRet);
		}

		if (didit)
		{
			// Check that the data in the miiSelector return buffer
			// is correct.
			if (miiSelectorChecksumIsValid(&msRet))
			{
				if (!msRet.no_mii_selected)
				{
					printf("A Mii was selected.\n");
					miiSelectorReturnGetName(&msRet, miiname, sizeof(miiname));
					miiSelectorReturnGetAuthor(&msRet, miiauthor, sizeof(miiauthor));
					printf("Name: %s\n", miiname);
					printf("Author: %s\n", miiauthor);
					printf("Birthday: Month-%d/Day-%d\n", msRet.mii.mii_details.bday_month, msRet.mii.mii_details.bday_day);
					printf("Sex: %d\n", msRet.mii.mii_details.sex);
					printf("Color: %d\n", msRet.mii.mii_details.shirt_color);
					printf("Favorite: %d\n", msRet.mii.mii_details.favorite);
					// Keep in mind that not all things align with their order in Mii Maker.
					// https://www.3dbrew.org/wiki/Mii#Mii_values
					printf("Eyebrow: %d\n", msRet.mii.eyebrow_details.style);
					printf("Eyebrow color: %d\n", msRet.mii.eyebrow_details.color);
					printf("Nose: %d\n", msRet.mii.nose_details.style);
				} else
					printf("No Mii was selected.\n");
			} else
				printf("Return checksum invalid.\n");
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
