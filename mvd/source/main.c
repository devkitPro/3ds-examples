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

	ret = mvdstdInit(MVDMODE_COLORFORMATCONV, MVD_INPUT_YUYV422, MVD_OUTPUT_BGR565, 0, NULL);
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
	gfxSwapBuffersGpu();
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

	u32 flagval=0;

	FILE *f = NULL;

	u8* gfxtopadr=NULL;

	MVDSTD_Config config;

	u32 prefix_offset;
	u8 prefix[4] = {0x00, 0x00, 0x00, 0x01};

	printf("Loading video...\n");

	//This loads the entire video into memory, normally you'd use a library to stream it.
	f = fopen("romfs:/video.h264", "r");
	if(f==NULL)
	{
		printf("Faile to open the video in romfs.\n");
		return;
	}

	video = &inaddr[0x100000];
	video_size = fread(video, 1, 0xF00000, f);
	fclose(f);

	if(video_size==0 || video_size>=0xF00000)
	{
		printf("Failed to read video / video is too large.\n");
		return;
	}

	ret = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, MVD_DEFAULT_WORKBUF_SIZE, NULL);
	printf("mvdstdInit(): 0x%08x\n", (unsigned int)ret);
	if(ret!=0)return;

	printf("Processing 0x%08x-byte video...\n", (unsigned int)video_size);

	mvdstdGenerateDefaultConfig(&config, 240, 400, 240, 400, NULL, (u32*)outaddr, (u32*)outaddr);//Normally you'd set the input dimensions here to dimensions loaded from the actual video.

	//Normally you'd use a library to load each NAL-unit, this example just parses the data manually.
	while(video_pos < video_size+1)
	{
		cur_nalunit_pos = video_pos;
		video_pos++;

		prefix_offset = 1;

		if(cur_nalunit_pos<video_size)
		{
			/*
			{
				if(memcmp(&video[cur_nalunit_pos], &prefix[1], 3)==0 && cur_nalunit_pos==0x2dd)
				{
					prefix_offset = 0;
				}
			}
			if(prefix_offset)*/
			//else
			{
				if(memcmp(&video[cur_nalunit_pos], prefix, 4))
				{
					continue;
				}
				else
				{
					video_pos++;
				}
			}
		}

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

			MVDSTD_ProcessNALUnitOut tmpout;//Normally you don't really need to use this.

			//printf("Processing NAL-unit at offset 0x%08x size 0x%08x...\n", (unsigned int)prev_nalunit_pos, (unsigned int)nalunitsize);
			ret = mvdstdProcessVideoFrame(inaddr, nalunitsize, flagval, &tmpout);
			if(!MVD_CHECKNALUPROC_SUCCESS(ret))
			{
				printf("mvdstdProcessVideoFrame() at NAL-unit offset 0x%08x size 0x%08x returned: 0x%08x. remaining_size=0x%08x.\n", (unsigned int)prev_nalunit_pos, (unsigned int)nalunitsize, (unsigned int)ret, (unsigned int)tmpout.remaining_size);
				break;
			}

			if(ret!=MVD_STATUS_PARAMSET && ret!=MVD_STATUS_INCOMPLETEPROCESSING)
			{
				gfxtopadr = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
				config.physaddr_outdata0 = osConvertVirtToPhys(gfxtopadr);

				//This sets the MVD output to the framebuffer directly. This is to avoid doing the video->framebuffer image rotation on the ARM11.
				//Normally you'd use a seperate MVD output buffer, then transfer that to the framebuffer(such as with GPU rendering).

				ret = mvdstdRenderVideoFrame(&config, true);
				if(ret!=MVD_STATUS_OK)
				{
					printf("mvdstdRenderVideoFrame() at NAL-unit offset 0x%08x returned: 0x%08x\n", (unsigned int)prev_nalunit_pos, (unsigned int)ret);
					break;
				}

				hidScanInput();
				if(hidKeysDown() & KEY_B)break;

				//Enable/disable the flag passed to mvdstdProcessVideoFrame().
				if(hidKeysDown() & KEY_DOWN)
				{
					flagval-= 0x1;
					printf("0x%08x\n", (unsigned int)flagval);
				}
				if(hidKeysDown() & KEY_UP)
				{
					flagval+= 0x1;
					printf("0x%08x\n", (unsigned int)flagval);
				}

				//gspWaitForVBlank();//Can't use this under this example without a major slowdown. This is due to this example not doing any buffering for the frames.
				gfxSwapBuffersGpu();
			}
		}

		nalcount++;

		prev_nalunit_pos = cur_nalunit_pos;
	}

	mvdstdExit();
}

int main()
{
	Result ret=0;

	int draw=1;
	int ready=0;
	int type=0;

	gfxInit(GSP_RGB565_OES,GSP_BGR8_OES,false);
	consoleInit(GFX_BOTTOM, NULL);

	ret = romfsInit();
	if(R_FAILED(ret))
	{
		printf("romfsInit() failed: 0x%08x\n", (unsigned int)ret);
		ready = -1;
	}

	if(ready==0)
	{
		inaddr = linearMemAlign(0x1000000, 0x40);
		outaddr = linearMemAlign(0x100000, 0x40);

		if(!(inaddr && outaddr))
		{
			ready = -2;
			printf("Failed to allocate memory.\n");
		}
	}

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		if(draw && type==0)
		{
			consoleClear();
			draw = 0;

			if(ready==0)printf("mvd example\n");

			printf("Press START to exit.\n");
			if(ready==0)
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

		if(ready==0)
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

	if(ready!=-1)romfsExit();

	gfxExit();
	return 0;
}

