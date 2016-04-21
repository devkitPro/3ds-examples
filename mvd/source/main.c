#include <stdio.h>
#include <string.h>
#include <errno.h>

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

	ret = mvdstdInit(MVDMODE_COLORFORMATCONV, MVD_INPUT_YUYV422, MVD_OUTPUT_BGR565, 0);
	printf("mvdstdInit(): 0x%08x\n", (unsigned int)ret);

	if(ret==0)
	{
		mvdstdGenerateDefaultConfig(&config, 320, 240, 320, 240, (u32*)inaddr, (u32*)outaddr, (u32*)&outaddr[0x12c00]);

		ret = mvdstdConvertImage(&config);
		printf("mvdstdConvertImage(): 0x%08x\n", (unsigned int)ret);
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

void mvd_video()
{
	Result ret;
	size_t video_size, nalunitsize;
	u32 video_pos=0;
	u32 cur_nalunit_pos=0, prev_nalunit_pos=0;
	u32 nalcount=0;
	u8 *video;
	bool parameter_set = true;
	u32 pos;
	u32 x, y, pos2;

	FILE *f = NULL;

	u8* gfxtopadr;

	MVDSTD_Config config;

	u32 prefix_offset;
	u8 prefix[4] = {0x00, 0x00, 0x00, 0x01};

	printf("Loading video...\n");

	ret = romfsInit();//Normally you don't need to use romfsInit() outside of app-startup either.
	if(R_FAILED(ret))
	{
		printf("romfsInit() failed: 0x%08x\n", (unsigned int)ret);
		return;
	}

	//This loads the entire video into memory, normally you'd use a library to stream it.
	f = fopen("romfs:/video.h264", "r");
	if(f==NULL)
	{
		printf("Faile to open the video in romfs, errno=0x%08x.\n", errno);
		romfsExit();
		return;
	}

	video = &inaddr[0x100000];
	video_size = fread(video, 1, 0xF00000, f);
	fclose(f);
	ret = romfsExit();
	printf("romfsExit(): 0x%08x\n", (unsigned int)ret);

	if(video_size==0 || video_size>=0xF00000)
	{
		printf("Failed to read video / video is too large.\n");
		return;
	}

	ret = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, 0x9006C8);//This size is what the New3DS Internet Browser uses(0x9006C8 from the MVDSTD:CalculateWorkBufSize output).
	printf("mvdstdInit(): 0x%08x\n", (unsigned int)ret);
	if(ret!=0)return;

	printf("Processing 0x%08x-byte video...\n", (unsigned int)video_size);

	mvdstdGenerateDefaultConfig(&config, 400, 240, 400, 240, NULL, (u32*)outaddr, NULL);

	//Normally you'd use a library to load each NAL-unit, this example just parses the data manually.
	while(video_pos < video_size+1)
	{
		cur_nalunit_pos = video_pos;
		video_pos++;

		prefix_offset = 1;

		if(cur_nalunit_pos<video_size)
		{
			/*if(memcmp(&video[cur_nalunit_pos], &prefix[1], 3)==0)
			{
				prefix_offset = 0;
			}
			else
			{*/
				if(memcmp(&video[cur_nalunit_pos], prefix, 4))continue;
			//}
		}

		video_pos+= 3+prefix_offset;

		if(nalcount && prev_nalunit_pos!=cur_nalunit_pos)
		{
			nalunitsize = cur_nalunit_pos - prev_nalunit_pos - prefix_offset;
			if(nalunitsize > 0x100000)
			{
				printf("The NAL-unit at offset 0x%08x is too large.\n", (unsigned int)nalunitsize);
				break;
			}

			memcpy(inaddr, &video[prev_nalunit_pos+prefix_offset], nalunitsize);
			GSPGPU_FlushDataCache(inaddr, nalunitsize);

			//printf("Processing NAL-unit at offset 0x%08x size 0x%08x...\n", (unsigned int)prev_nalunit_pos, (unsigned int)nalunitsize);
			ret = mvdstdProcessVideoFrame(&config, parameter_set, inaddr, nalunitsize);
			if(R_FAILED(ret))//Ignore MVD status-codes for now.
			{
				printf("mvdstdProcessVideoFrame() at NAL-unit offset 0x%08x returned: 0x%08x\n", (unsigned int)prev_nalunit_pos, (unsigned int)ret);
				break;
			}

			if(!parameter_set)
			{
				GSPGPU_InvalidateDataCache(outaddr, 0x46500);

				hidScanInput();
				if(hidKeysHeld() & KEY_B)break;

				//Code for testing various config adjustments.
				if(hidKeysDown() & KEY_DOWN)config.unk_x6c[(0x10c-0x6c)>>2]-= 0x10;
				if(hidKeysDown() & KEY_UP)config.unk_x6c[(0x10c-0x6c)>>2]+= 0x10;
				if(hidKeysDown() & KEY_LEFT)config.unk_x6c[(0x104-0x6c)>>2]-= 0x1;
				if(hidKeysDown() & KEY_RIGHT)config.unk_x6c[(0x104-0x6c)>>2]+= 0x1;

				gfxtopadr = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
				for(x=0; x<400; x++)//Copy the output image which has the width aligned to 512, to the framebuffer. This needs replaced with a GPU-related copy later since it's very slow.
				{
					for(y=0; y<240; y++)
					{
						pos = (y*512 + x) * 2;
						pos2 = (x*240 + 239-y) * 2;

						*((u16*)&gfxtopadr[pos2+0]) = *((u16*)&outaddr[pos+0]);
					}
				}

				gfxFlushBuffers();
				gfxSwapBuffers();
			}
		}

		nalcount++;
		if(nalcount>=3 && parameter_set)
		{
			parameter_set = false;
		}

		prev_nalunit_pos = cur_nalunit_pos;
	}

	mvdstdExit();
}

int main()
{
	int draw=1;
	int ready=0;
	int type=0;

	gfxInit(GSP_RGB565_OES,GSP_BGR8_OES,false);
	consoleInit(GFX_BOTTOM, NULL);

	inaddr = linearMemAlign(0x1000000, 0x40);
	outaddr = linearMemAlign(0x100000, 0x40);

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
				printf("Press B for video(no sound).\n");
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
			if (kDown & KEY_B)type = 2;

			if(type)
			{
				memset(inaddr, 0, 0x100000);
				memset(outaddr, 0, 0x100000);

				if(type==1)mvd_colorconvert();
				if(type==2)mvd_video();

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

