/*
	Lua example made by pggkun for libctru
	This code was modified for the last time on: 29/04/2022
*/

#include <3ds.h>
#include <stdio.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define FILE_PATH "scripts/example.lua"

int main(int argc, char **argv)
{
    // Initialize services
	gfxInitDefault();
	PrintConsole topScreen, bottomScreen;
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
	consoleSelect(&topScreen);

    // Create a new lua instance
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // Load the .lua file
    luaL_dofile(L, FILE_PATH);

    // Create an int to inject in .lua file
    printf("Int from C to Lua example\n\n");
    int x = 123456;
    lua_pushnumber(L, x);
    lua_setglobal(L, "x");

    // Calling a function from .lua file 
    lua_getglobal(L, "print_x");
    lua_call(L, 0, 0);

    consoleSelect(&bottomScreen);
    printf("\x1b[30;16HPress Start to exit.");

	// Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}

    // Close lua instance
    lua_close(L);

    // Exit services
	gfxExit();

    return 0;
}
