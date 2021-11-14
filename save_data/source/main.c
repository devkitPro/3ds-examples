/*
	Save data example made by TheGag96 for libctru
	This code was modified for the last time on: 11/13/2021 19:52 CST
*/

#include <3ds.h>
#include <stdio.h>

typedef struct {
  s32 currentLevel;
  u32 totalCoins;
  u16 hours;
  u8 minutes;
  u8 seconds;
  u8 lives;
} SaveFile;

static const FS_Path EMPTY_PATH = { PATH_EMPTY, 0, NULL };

bool createSave() {
  FS_Archive archiveHandle = 0;

  //The third argument needs to be less than or equal to SaveDataSize in resources/cia-info.rsf and be a multiple of 64 KiB
  Result rc = FSUSER_FormatSaveData(ARCHIVE_SAVEDATA, EMPTY_PATH, 512 /* bytes */, 7, 7, 7, 7, false);

  if (R_FAILED(rc)) {
    printf("\x1b[16;1HCouldn't format: %08lX\x1b[K", rc);
    return false;
  }

  rc = FSUSER_OpenArchive(&archiveHandle, ARCHIVE_SAVEDATA, EMPTY_PATH);

  if (R_FAILED(rc)) {
    printf("\x1b[16;1HCouldn't open archive: %08lX\x1b[K", rc);
    return false;
  }

  SaveFile saveFile;
  saveFile.hours = 7;

  FS_Path sgPath = fsMakePath(PATH_ASCII, "/save.dat");

  rc = FSUSER_CreateFile(archiveHandle, sgPath, 0, sizeof(saveFile));

  if (R_FAILED(rc)) {
    printf("\x1b[16;1HCouldn't create file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return false;
  }

  Handle fileHandle;

  rc = FSUSER_OpenFile(&fileHandle, archiveHandle, sgPath, 7, 0);

  if (R_FAILED(rc)) {
    printf("\x1b[16;1HCouldn't open file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return false;
  }

  u32 bytesWritten;
  rc = FSFILE_Write(fileHandle, &bytesWritten, 0, &saveFile, sizeof(saveFile), 0);

  if (R_FAILED(rc) || bytesWritten != sizeof(saveFile)) {
    printf("\x1b[16;1HCouldn't write file: %08lX\x1b[K", rc);
    FSFILE_Close(fileHandle);
    FSUSER_CloseArchive(archiveHandle);
    return false;
  }

  printf("\x1b[16;1HSuccess!\x1b[K");

  FSUSER_CloseArchive(archiveHandle);

  return true;
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	fsInit();

	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_TOP, NULL);

  //Create the save file like a real 3DS app would
  createSave();

	//Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		//Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
