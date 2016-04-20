#include <stdio.h>
#include <string.h>

#include <3ds.h>

u8* inaddr;
u8* outaddr;

void mvd_colorconvert()
{
	Result ret;

	FILE *f = NULL;

	u8* bufAdr = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	u8* gfxtopadr = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	MVDSTD_Config config;

	printf("mvd color-format-conversion example.\n");

	f = fopen("sdmc:/mvd_indata.bin", "r");
	if(f)
	{
		fread(inaddr, 1, 0x46500, f);
		fclose(f);
	}
	else
	{
		memcpy(inaddr, bufAdr, 320*240*3);
	}

	memset(gfxtopadr, 0, 0x46500);
	GSPGPU_FlushDataCache(inaddr, 0x46500);
	GSPGPU_FlushDataCache(gfxtopadr, 0x46500);

	ret = mvdstdInit(MVDMODE_COLORFORMATCONV, MVD_INPUT_YUYV422, MVD_OUTPUT_RGB565, 0);
	printf("mvdstdInit(): 0x%08x\n", (unsigned int)ret);

	if(ret>=0)
	{
		mvdstdGenerateDefaultConfig(&config, 320, 240, 320, 240, (u32*)inaddr, (u32*)outaddr, (u32*)&outaddr[0x12c00]);

		ret = mvdstdProcessFrame(&config, NULL, 0, 0);
		printf("mvdstdProcessFrame(): 0x%08x\n", (unsigned int)ret);
	}

	f = fopen("sdmc:/mvd_outdata.bin", "w");
	if(f)
	{
		fwrite(outaddr, 1, 0x100000, f);
		fclose(f);
	}

	memcpy(gfxtopadr, outaddr, 0x46500);

	mvdstdExit();

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();
}

int main()
{
	int draw=1;
	int ready=0;
	int type=0;

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	inaddr = linearAlloc(0x100000);
	outaddr = linearAlloc(0x100000);

	if(inaddr && outaddr)ready=1;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		if(draw && type==0)
		{
			consoleClear();
			draw = 0;

			printf("mvd example\n");

			if(!ready)
			{
				printf("Failed to allocate memory.\n");
			}

			printf("Press START to exit.\n");
			if(ready)
			{
				printf("Press A for color-format-conversion.\n");
			}
		}

		u32 kDown = hidKeysDown();

		if(type)
		{
			if(kDown & KEY_A)
			{
				type = 0;
				continue;
			}
		}

		if(type)continue;

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if(ready)
		{
			type = 0;
			if (kDown & KEY_A)type = 1;

			if(type)
			{
				memset(inaddr, 0, 0x100000);
				memset(outaddr, 0, 0x100000);

				if(type==1)mvd_colorconvert();

				draw = 1;
				printf("Press A to continue.\n");
			}
		}
	}

	if(inaddr)linearFree(inaddr);
	if(outaddr)linearFree(outaddr);

	gfxExit();
	return 0;
}

