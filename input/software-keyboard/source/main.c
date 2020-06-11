#include <3ds.h>
#include <stdio.h>
#include <string.h>

static SwkbdCallbackResult MyCallback(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	if (strstr(text, "lenny"))
	{
		*ppMessage = "Nice try but I'm not letting you use that meme right now";
		return SWKBD_CALLBACK_CONTINUE;
	}

	if (strstr(text, "brick"))
	{
		*ppMessage = "~Time to visit Brick City~";
		return SWKBD_CALLBACK_CLOSE;
	}

	return SWKBD_CALLBACK_OK;
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("Software keyboard demo - by fincs\n");
	printf("A: text input\n");
	printf("B: numpad input\n");
	printf("X: western input\n");
	printf("Y: filtered input + HOME/POWER/reset handling\n");
	printf("START: exit\n");

	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;

		static SwkbdState swkbd;
		static char mybuf[60];
		static SwkbdStatusData swkbdStatus;
		static SwkbdLearningData swkbdLearning;
		SwkbdButton button = SWKBD_BUTTON_NONE;
		bool didit = false;

		if (kDown & KEY_A)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 3, -1);
			swkbdSetInitialText(&swkbd, mybuf);
			swkbdSetHintText(&swkbd, "Please enter dank memes");
			swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Maybe Not", false);
			swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, "~Middle~", true);
			swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Do It", true);
			swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
			SwkbdDictWord words[2];
			swkbdSetDictWord(&words[0], "lenny", "( ͡° ͜ʖ ͡°)");
			swkbdSetDictWord(&words[1], "shrug", "¯\\_(ツ)_/¯");
			swkbdSetDictionary(&swkbd, words, sizeof(words)/sizeof(SwkbdDictWord));
			static bool reload = false;
			swkbdSetStatusData(&swkbd, &swkbdStatus, reload, true);
			swkbdSetLearningData(&swkbd, &swkbdLearning, reload, true);
			reload = true;
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_B)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 8);
			swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
			swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);
			swkbdSetFeatures(&swkbd, SWKBD_FIXED_WIDTH);
			swkbdSetNumpadKeys(&swkbd, L'ツ', L'益');
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_X)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS | SWKBD_FILTER_AT | SWKBD_FILTER_PERCENT | SWKBD_FILTER_BACKSLASH | SWKBD_FILTER_PROFANITY, 2);
			swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
			swkbdSetHintText(&swkbd, "No Japanese text allowed ¯\\_(ツ)_/¯");
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		}

		if (kDown & KEY_Y)
		{
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
			swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_ALLOW_HOME | SWKBD_ALLOW_RESET | SWKBD_ALLOW_POWER);
			swkbdSetFilterCallback(&swkbd, MyCallback, NULL);

			bool shouldQuit = false;
			mybuf[0] = 0;
			do
			{
				swkbdSetInitialText(&swkbd, mybuf);
				button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
				if (button != SWKBD_BUTTON_NONE)
					break;

				SwkbdResult res = swkbdGetResult(&swkbd);
				if (res == SWKBD_RESETPRESSED)
				{
					shouldQuit = true;
					aptSetChainloaderToSelf();
					break;
				}
				else if (res != SWKBD_HOMEPRESSED && res != SWKBD_POWERPRESSED)
					break; // An actual error happened, display it

				shouldQuit = !aptMainLoop();
			} while (!shouldQuit);

			if (shouldQuit)
				break;
		}

		if (didit)
		{
			if (button != SWKBD_BUTTON_NONE)
			{
				printf("You pressed button %d\n", button);
				printf("Text: %s\n", mybuf);
			} else
				printf("swkbd event: %d\n", swkbdGetResult(&swkbd));
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
