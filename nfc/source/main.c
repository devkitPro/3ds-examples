#include <stdio.h>
#include <string.h>

#include <3ds.h>

Result nfc_test()
{
	Result ret=0;
	u32 pos;
	FILE *f;
	size_t tmpsize;

	NFC_TagState prevstate, curstate;
	NFC_TagInfo taginfo;
	NFC_AmiiboSettings amiibosettings;
	NFC_AmiiboConfig amiiboconfig;

	u32 amiibo_appid = 0x10110E00;//Hard-coded for Super Smash Bros. See also: https://www.3dbrew.org/wiki/Amiibo
	u32 appdata_initialized;

	u8 appdata[0xd8];

	char uidstr[16];
	char tmpstr[64];

	printf("NFC initialized successfully, starting scanning...\n");

	ret = nfcStartScanning(NFC_STARTSCAN_DEFAULTINPUT);
	if(R_FAILED(ret))
	{
		printf("nfcStartScanning() failed.\n");
		return ret;
	}

	ret = nfcGetTagState(&curstate);
	if(R_FAILED(ret))
	{
		printf("nfcGetTagState() failed.\n");
		nfcStopScanning();
		return ret;
	}

	prevstate = curstate;

	printf("Press B to stop scanning or Y to restart scanning.\n");
	printf("Press X after an amiibo was successfully\nprocessed while still in range, to reprocess it.\n");
	printf("Hold down A while processing an amiibo to write\nappdata from SD to it. The amiibo appdata will be\ninitialized if it wasn't initialized already.\n");
	printf("Scanning must be restarted after a NFC tag goes\nout-of-range, if you want to scan more tag(s).\n");

	while(1)
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if(kDown & KEY_B)break;

		if(kDown & KEY_Y)//If you want this to be done automatically, you could run this when the TagState changes to NFC_TagState_OutOfRange.
		{
			printf("Restarting scanning...\n");

			nfcStopScanning();
			ret = nfcStartScanning(NFC_STARTSCAN_DEFAULTINPUT);
			if(R_FAILED(ret))
			{
				printf("nfcStartScanning() failed.\n");
				return ret;
			}

			printf("Scanning restarted.\n");

			continue;
		}

		if(kDown & KEY_X)
		{
			ret = nfcResetTagScanState();
			if(R_FAILED(ret))
			{
				printf("nfcResetTagScanState() failed.\n");
				break;
			}

			printf("nfcResetTagScanState() was successful.\n");
		}

		ret = nfcGetTagState(&curstate);
		if(R_FAILED(ret))
		{
			printf("nfcGetTagState() failed.\n");
			break;
		}

		if(curstate!=prevstate)//See nfc.h for the TagState values.
		{
			printf("TagState changed from %d to %d.\n", prevstate, curstate);
			prevstate = curstate;

			if(curstate==NFC_TagState_InRange)
			{
				memset(&taginfo, 0, sizeof(NFC_TagInfo));
				memset(&amiibosettings, 0, sizeof(NFC_AmiiboSettings));

				ret = nfcGetTagInfo(&taginfo);
				if(R_FAILED(ret))
				{
					printf("nfcGetTagInfo() failed.\n");
					break;
				}

				printf("taginfo id_offset_size: 0x%x.\n", (unsigned int)taginfo.id_offset_size);
				printf("NFC tag UID:\n");

				memset(uidstr, 0, sizeof(uidstr));
				for(pos=0; pos<7; pos++)snprintf(&uidstr[pos*2], 3, "%02x", taginfo.id[pos]);
				printf("%s\n", uidstr);

				ret = nfcLoadAmiiboData();
				if(R_FAILED(ret))
				{
					printf("nfcLoadAmiiboData() failed.\n");
					break;
				}

				memset(&amiibosettings, 0, sizeof(NFC_AmiiboSettings));
				memset(&amiiboconfig, 0, sizeof(NFC_AmiiboConfig));

				ret = nfcGetAmiiboSettings(&amiibosettings);
				if(R_FAILED(ret))
				{
					printf("nfcGetAmiiboSettings() failed.\n");
					if(ret==NFC_ERR_AMIIBO_NOTSETUP)printf("This amiibo wasn't setup by the amiibo Settings applet.\n");
					break;
				}

				ret = nfcGetAmiiboConfig(&amiiboconfig);
				if(R_FAILED(ret))
				{
					printf("nfcGetAmiiboConfig() failed.\n");
					break;
				}

				printf("amiibo data successfully loaded.\n");

				printf("amiiboconfig: lastwritedate year=%u month=%u day=%u.\n", amiiboconfig.lastwritedate_year, amiiboconfig.lastwritedate_month, amiiboconfig.lastwritedate_day);
				printf("amiiboconfig.write_counter=%u\n", amiiboconfig.write_counter);

				printf("Opening appdata...\n");

				appdata_initialized = 1;

				ret = nfcOpenAppData(amiibo_appid);
				if(R_FAILED(ret))
				{
					printf("nfcOpenAppData() failed.\n");
					if(ret==NFC_ERR_APPDATA_UNINITIALIZED)
					{
						printf("This appdata isn't initialized.\n");
						appdata_initialized = 0;
					}
					if(ret==NFC_ERR_APPID_MISMATCH)printf("This appdata is for a different appid(non-Super-Smash-Bros).\n");
					if(appdata_initialized)break;
				}

				memset(appdata, 0, sizeof(appdata));

				if(!appdata_initialized)
				{
					printf("Skipping appdata reading since it's uninitialized.\n");
				}
				else
				{
					printf("Reading appdata...\n");

					ret = nfcReadAppData(appdata, sizeof(appdata));
					if(R_FAILED(ret))
					{
						printf("nfcReadAppData() failed.\n");
						break;
					}

					printf("Appdata:\n");
					for(pos=0; pos<sizeof(appdata); pos++)printf("%02x", appdata[pos]);
					printf("\n");

					memset(tmpstr, 0, sizeof(tmpstr));
					snprintf(tmpstr, sizeof(tmpstr)-1, "amiibo_appdata_out_%s.bin", uidstr);

					printf("Writing appdata to file '%s'...\n", tmpstr);
					f = fopen(tmpstr, "w");
					if(f)
					{
						tmpsize = fwrite(appdata, 1, sizeof(appdata), f);
						fclose(f);

						if(tmpsize!=sizeof(appdata))
						{
							printf("Failed to write to the file, only 0x%x of 0x%x bytes were written.\n", tmpsize, sizeof(appdata));
						}
						else
						{
							printf("Writing finished.\n");
						}
					}
					else
					{
						printf("Failed to open the file.\n");
					}
				}

				if(hidKeysHeld() & KEY_A)
				{
					memset(tmpstr, 0, sizeof(tmpstr));
					snprintf(tmpstr, sizeof(tmpstr)-1, "amiibo_appdata_in_%s.bin", uidstr);

					printf("Loading appdata from file '%s', which will be written to the amiibo...\n", tmpstr);

					ret = 1;

					f = fopen(tmpstr, "r");
					if(f)
					{
						memset(appdata, 0, sizeof(appdata));
						tmpsize = fread(appdata, 1, sizeof(appdata), f);
						fclose(f);

						if(tmpsize!=sizeof(appdata))
						{
							printf("Failed to read to the file, only 0x%x of 0x%x bytes were written.\n", tmpsize, sizeof(appdata));
						}
						else
						{
							printf("File reading finished.\n");
							ret = 0;
						}
					}
					else
					{
						printf("Failed to open the file.\n");
					}

					if(ret==0)
					{
						if(appdata_initialized)
						{
							printf("Writing the appdata...\n");

							ret = nfcWriteAppData(appdata, sizeof(appdata), &taginfo);
							if(R_FAILED(ret))
							{
								printf("nfcWriteAppData() failed.\n");
								break;
							}

							printf("Writing to the amiibo NFC tag...\n");

							ret = nfcUpdateStoredAmiiboData();
							if(R_FAILED(ret))
							{
								printf("nfcUpdateStoredAmiiboData() failed.\n");
								break;
							}
						}
						else
						{
							printf("Initializing the appdata...\n");

							ret = nfcInitializeWriteAppData(amiibo_appid, appdata, sizeof(appdata));
							if(R_FAILED(ret))
							{
								printf("nfcInitializeWriteAppData() failed.\n");
								break;
							}
						}

						printf("Writing finished.\n");
					}
				}

				printf("amiibo NFC tag processing has finished, you can\nremove the tag from NFC-range now.\n");
			}
		}
	}

	nfcStopScanning();

	return ret;
}

int main()
{
	Result ret=0;

	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("libctru nfc example.\n");

	ret = nfcInit(NFC_OpType_NFCTag);
	if(R_FAILED(ret))
	{
		printf("nfcInit() failed: 0x%08x.\n", (unsigned int)ret);
	}
	else
	{
		ret = nfc_test();
		printf("nfc_test() returned: 0x%08x.\n", (unsigned int)ret);

		nfcExit();
	}

	printf("Press START to exit.\n");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	return 0;
}

