#include <string.h>
#include <stdio.h>

#include <3ds.h>

char regionids_table[7][4] = {//https://3dbrew.org/wiki/Nandrw/sys/SecureInfo_A
"JPN",
"USA",
"EUR",
"JPN", //"AUS"
"CHN",
"KOR",
"TWN"
};

Result setup_boss()
{
	Result ret=0;
	u32 test_NsDataId = 0x57524248;//This can be anything.

	u8 region=0;

	bossContext ctx;
	u8 status=0;
	u32 tmp=0;
	u32 contentsize=0;

	char *taskID = "tmptask";//Should be unique per task(and unique per homebrew when running under *hax payload >=v2.8), unless the task is deleted afterwards.

	char tmpstr[256];

	printf("Running BOSS setup...\n");

	ret = cfguInit();
	if(ret!=0)
	{
		printf("Failed to init cfg: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	ret = CFGU_SecureInfoGetRegion(&region);
	if(ret!=0)
	{
		printf("Failed to get region from cfg: 0x%08x.\n", (unsigned int)ret);
		return ret;
	}
	if(region>=7)
	{
		printf("Region value from cfg is invalid: 0x%02x.\n", (unsigned int)region);
		ret = -9;
		return ret;
	}
	cfguExit();

	ret = bossInit(0, false);
	if(R_FAILED(ret))return ret;

	//If you're using background tasks you should check whether the task exists first, and skip task setup if so.

	bossDeleteTask(taskID, 0);
	bossDeleteNsData(test_NsDataId);

	//HTTP is used here since it's currently unknown how to setup a non-default rootCA cert for BOSS.
	//This uses a 60-second interval, with background tasks you will probably want to change that.
	//You can change the below URL to your own, the URL should include region info(see README).

	memset(tmpstr, 0, sizeof(tmpstr));
	snprintf(tmpstr, sizeof(tmpstr)-1, "http://yls8.mtheall.com/boss/libctru_example/%s_bossdata0.bin", regionids_table[region]);

	bossSetupContextDefault(&ctx, 60, tmpstr);

	ret = bossSendContextConfig(&ctx);
	if(R_FAILED(ret))printf("bossSendContextConfig returned 0x%08x.\n", (unsigned int)ret);

	if(R_SUCCEEDED(ret))
	{
		ret = bossRegisterTask(taskID, 0, 0);
		if(R_FAILED(ret))printf("bossRegisterTask returned 0x%08x.\n", (unsigned int)ret);

		if(R_SUCCEEDED(ret))
		{
			ret = bossStartTaskImmediate(taskID);
			if(R_FAILED(ret))printf("bossStartTaskImmediate returned 0x%08x.\n", (unsigned int)ret);

			//The below is only needed if you want to process the content immediately.

			if(R_SUCCEEDED(ret))
			{
				printf("Waiting for the task to finish running...\n");

				while(1)
				{
					ret = bossGetTaskState(taskID, 0, &status, NULL, NULL);
					if(R_FAILED(ret))
					{
						printf("bossGetTaskState returned 0x%08x.\n", (unsigned int)ret);
						break;
					}
					if(R_SUCCEEDED(ret))
					{
						printf("...\n");
					}

					if(status!=BOSSTASKSTATUS_STARTED)break;

					svcSleepThread(1000000000LL);//Delay 1s.
				}
			}

			if(R_SUCCEEDED(ret) && status==BOSSTASKSTATUS_ERROR)
			{
				printf("BOSS task failed. This usually indicates a network failure.\n");
				ret = -9;
			}

			if(R_SUCCEEDED(ret))
			{
				printf("Reading BOSS content...\n");

				ret = bossGetNsDataHeaderInfo(test_NsDataId, bossNsDataHeaderInfoType_ContentSize, &contentsize, bossNsDataHeaderInfoTypeSize_ContentSize);
				if(R_FAILED(ret))printf("bossGetNsDataHeaderInfo returned 0x%08x.\n", (unsigned int)ret);

				if(R_SUCCEEDED(ret))
				{
					printf("Content size: 0x%x.\n", (unsigned int)contentsize);
					if(contentsize > sizeof(tmpstr)-1)contentsize = sizeof(tmpstr)-1;

					tmp = 0;
					memset(tmpstr, 0, sizeof(tmpstr));
					ret = bossReadNsData(test_NsDataId, 0, tmpstr, contentsize, &tmp, NULL);
					if(R_FAILED(ret))printf("bossReadNsData returned 0x%08x, transfer_total=0x%x.\n", (unsigned int)ret, (unsigned int)tmp);
				}

				if(R_SUCCEEDED(ret) && tmp!=contentsize)ret = -10;

				//This verifies that the first 4-bytes matches the expected data, but you can do whatever with the loaded content here.
				if(R_SUCCEEDED(ret) && strncmp(tmpstr, "Test", 4))ret = -11;

				if(R_SUCCEEDED(ret))printf("Content data at offset 0x4: %s\n", &tmpstr[4]);

				if(R_FAILED(ret))printf("BOSS data reading failed: 0x%08x.\n", (unsigned int)ret);
			}

			bossDeleteTask(taskID, 0);//Don't run this if you want a background task.
			bossDeleteNsData(test_NsDataId);//Don't run this if the SpotPass content will be used again later.
		}
	}

	if(R_SUCCEEDED(ret))printf("Done.\n");

	bossExit();

	return ret;
}

int main(int argc, char **argv) {

	Result ret = 0;

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	ret = setup_boss();
	if(R_FAILED(ret))printf("setup_boss() failed: 0x%08x.\n", (unsigned int)ret);

	printf("Press START to exit.\n");

	// Main loop
	while (aptMainLoop()) {

		gspWaitForVBlank();
		hidScanInput();

		// Your code goes here

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	return 0;
}

