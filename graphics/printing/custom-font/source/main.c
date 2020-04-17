#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>

C2D_TextBuf g_staticBuf;
C2D_Text g_staticText[3];
C2D_Font font[3];

static void sceneInit(void)
{
	g_staticBuf  = C2D_TextBufNew(4096); // support up to 4096 glyphs in the buffer
	font[0] = C2D_FontLoadSystem(CFG_REGION_USA);
	font[1] = C2D_FontLoadSystem(CFG_REGION_KOR);
	font[2] = C2D_FontLoad("romfs:/liberationitalic.bcfnt");

	// Parse the text strings
	// Loads system font
	C2D_TextFontParse(&g_staticText[0], font[0], g_staticBuf, "A boring system font.");
	// Uses loaded font
	C2D_TextFontParse(&g_staticText[1], font[1], g_staticBuf, "이 텍스트는 한국어입니다.");
	// Uses other loaded font
	C2D_TextFontParse(&g_staticText[2], font[2], g_staticBuf, "Wow, this is interesting.");

	// Optimize the text strings
	C2D_TextOptimize(&g_staticText[0]);
	C2D_TextOptimize(&g_staticText[1]);
	C2D_TextOptimize(&g_staticText[2]);
}

static void sceneRender(float size)
{
	// Draw static text strings
	float text2PosX = 400.0f - 16.0f - g_staticText[2].width*0.75f; // right-justify
	C2D_DrawText(&g_staticText[0], 0, 8.0f, 8.0f, 0.5f, size, size);
	C2D_DrawText(&g_staticText[1], C2D_AtBaseline, 108.0f, 36.0f, 0.5f, size, size);
	C2D_DrawText(&g_staticText[2], C2D_AtBaseline, text2PosX, 210.0f, 0.5f, size, size);
}

static void sceneExit(void)
{
	// Delete the text buffers
	C2D_TextBufDelete(g_staticBuf);
	C2D_FontFree(font[0]);
	C2D_FontFree(font[1]);
	C2D_FontFree(font[2]);
}

int main()
{
	romfsInit();
	cfguInit(); // Allow C2D_FontLoadSystem to work
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
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

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
	romfsExit();
	cfguExit();
	gfxExit();
	return 0;
}
