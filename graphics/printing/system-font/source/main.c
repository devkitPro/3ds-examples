#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>

static const char teststring[] =
	"Hello World - now with citro2d!\n"
	"The quick brown fox jumps over the lazy dog.\n"
	"\n"
	"En français: Vous ne devez pas éteindre votre console.\n"
	"日本語文章を見せるのも出来ますよ。\n"
	"Un poco de texto en español nunca queda mal.\n"
	"Πού είναι η τουαλέτα;\n"
	"Я очень рад, ведь я, наконец, возвращаюсь домой\n";

C2D_TextBuf g_staticBuf, g_dynamicBuf;
C2D_Text g_staticText[4];

static void sceneInit(void)
{
	// Create two text buffers: one for static text, and another one for
	// dynamic text - the latter will be cleared at each frame.
	g_staticBuf  = C2D_TextBufNew(4096); // support up to 4096 glyphs in the buffer
	g_dynamicBuf = C2D_TextBufNew(4096);

	// Parse the static text strings
	C2D_TextParse(&g_staticText[0], g_staticBuf, teststring);
	C2D_TextParse(&g_staticText[1], g_staticBuf, "I am red skinny text!");
	C2D_TextParse(&g_staticText[2], g_staticBuf, "I am blue fat text!");
	C2D_TextParse(&g_staticText[3], g_staticBuf, "I am justified text!");

	// Optimize the static text strings
	C2D_TextOptimize(&g_staticText[0]);
	C2D_TextOptimize(&g_staticText[1]);
	C2D_TextOptimize(&g_staticText[2]);
	C2D_TextOptimize(&g_staticText[3]);
}

static void sceneRender(float size)
{
	// Clear the dynamic text buffer
	C2D_TextBufClear(g_dynamicBuf);

	// Draw static text strings
	C2D_DrawText(&g_staticText[0], 0, 8.0f, 8.0f, 0.5f, size, size);
	C2D_DrawText(&g_staticText[1], C2D_AtBaseline | C2D_WithColor, 16.0f, 210.0f, 0.5f, 0.5f, 0.75f, C2D_Color32f(1.0f,0.0f,0.0f,1.0f));
	C2D_DrawText(&g_staticText[2], C2D_AtBaseline | C2D_WithColor | C2D_AlignRight, 384.0f, 210.0f, 0.5f, 0.75f, 0.5f, C2D_Color32f(0.0f,0.0f,1.0f,0.625f));
	C2D_DrawText(&g_staticText[3], C2D_AtBaseline | C2D_AlignJustified | C2D_WordWrap, 100.0f, 170.0f, 0.5f, 0.75f, 0.75f, 200.0f);

	// Generate and draw dynamic text
	char buf[160];
	C2D_Text dynText;
	snprintf(buf, sizeof(buf), "Current text size: %f (Use  to change)", size);
	C2D_TextParse(&dynText, g_dynamicBuf, buf);
	C2D_TextOptimize(&dynText);
	C2D_DrawText(&dynText, C2D_AlignCenter, 200.0f, 220.0f, 0.5f, 0.5f, 0.5f);
}

static void sceneExit(void)
{
	// Delete the text buffers
	C2D_TextBufDelete(g_dynamicBuf);
	C2D_TextBufDelete(g_staticBuf);
}

int main()
{
	// Initialize the libs
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	// Create screen
	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	// Initialize the scene
	sceneInit();

	float size = 0.5f;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kHeld & KEY_L)
			size -= 1.0f/128;
		else if (kHeld & KEY_R)
			size += 1.0f/128;
		else if (kHeld & KEY_X)
			size = 0.5f;
		else if (kHeld & KEY_Y)
			size = 1.0f;

		if (size < 0.25f)
			size = 0.25f;
		else if (size > 2.0f)
			size = 2.0f;

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32(0x68, 0xB0, 0xD8, 0xFF));
		C2D_SceneBegin(top);
		sceneRender(size);
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize the libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	return 0;
}
