/*
	Save data example made by TheGag96 for libctru
	This code was modified for the last time on: 11/13/2021 19:52 CST
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  s32 value;
  u32 totalCoins;
  u16 hours;
  u8 minutes;
  u8 seconds;
  u8 lives;
} SaveFile;

static const FS_Path EMPTY_PATH = { PATH_EMPTY, 0, NULL };
FS_Path saveFilePath;

SaveFile saveFile;

Result writeSave()
{
  FS_Archive archiveHandle = 0;

  Result rc = FSUSER_OpenArchive(&archiveHandle, ARCHIVE_SAVEDATA, EMPTY_PATH);

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't open archive: %08lX\x1b[K", rc);
    return rc;
  }

  Handle fileHandle;

  rc = FSUSER_OpenFile(&fileHandle, archiveHandle, saveFilePath, FS_OPEN_WRITE, 0);

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't open file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  u32 bytesWritten;
  rc = FSFILE_Write(fileHandle, &bytesWritten, 0, &saveFile, sizeof(saveFile), 0);

  if (R_FAILED(rc) || bytesWritten != sizeof(saveFile))
  {
    printf("\x1b[16;1HCouldn't write file: %08lX\x1b[K", rc);
    FSFILE_Close(fileHandle);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  FSUSER_CloseArchive(archiveHandle);

  return 0;
}

Result createSave()
{
  FS_Archive archiveHandle = 0;

  //The third argument needs to be less than or equal to SaveDataSize in resources/cia-info.rsf and be a multiple of 64 KiB
  Result rc = FSUSER_FormatSaveData(ARCHIVE_SAVEDATA, EMPTY_PATH, 512 /* bytes */, 7, 7, 7, 7, false);

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't format: %08lX\x1b[K", rc);
    return rc;
  }

  rc = FSUSER_OpenArchive(&archiveHandle, ARCHIVE_SAVEDATA, EMPTY_PATH);

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't open archive: %08lX\x1b[K", rc);
    return rc;
  }

  memset(&saveFile, 0, sizeof(saveFile));

  rc = FSUSER_CreateFile(archiveHandle, saveFilePath, 0, sizeof(saveFile));

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't create file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  FSUSER_CloseArchive(archiveHandle);

  rc = writeSave();

  if (!R_FAILED(rc))
  {
    printf("\x1b[16;1HCreate success!\x1b[K");
  }

  return rc;
}

Result loadOrCreateSave()
{
  FS_Archive archiveHandle = 0;

  Result rc = FSUSER_OpenArchive(&archiveHandle, ARCHIVE_SAVEDATA, EMPTY_PATH);

  if (R_FAILED(rc))
  {
    return createSave();
  }

  Handle fileHandle;

  rc = FSUSER_OpenFile(&fileHandle, archiveHandle, saveFilePath, FS_OPEN_READ, 0);

  if (R_FAILED(rc))
  {
    printf("\x1b[16;1HCouldn't open file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  u64 fileSize;
  rc = FSFILE_GetSize(fileHandle, &fileSize);

  if (R_FAILED(rc) || fileSize != sizeof(saveFile))
  {
    printf("\x1b[16;1HEither couldn't get save file size or it mismatches: %08lX\x1b[K", rc);
    FSFILE_Close(fileHandle);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  u32 bytesRead;
  rc = FSFILE_Read(fileHandle, &bytesRead, 0, &saveFile, sizeof(saveFile));

  if (R_FAILED(rc) || bytesRead != sizeof(saveFile))
  {
    printf("\x1b[16;1HCouldn't read save: %08lX\x1b[K", rc);
    FSFILE_Close(fileHandle);
    FSUSER_CloseArchive(archiveHandle);
    return rc;
  }

  printf("\x1b[16;1HRead success!\x1b[K");

  FSFILE_Close(fileHandle);
  FSUSER_CloseArchive(archiveHandle);

  return 0;
}

int main(int argc, char **argv)
{
	gfxInitDefault();
	fsInit();

	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_TOP, NULL);

  //Load or create the save file like a real 3DS app would
  saveFilePath = fsMakePath(PATH_ASCII, "/save.dat");
  loadOrCreateSave();

	//Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
    u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

    if (kHeld & KEY_UP)
    {
      saveFile.value++;
    }
    else if (kHeld & KEY_DOWN)
    {
      saveFile.value--;
    }

    printf("\x1b[11;1HValue: %ld\x1b[K", saveFile.value);
    printf("\x1b[12;1HPress up/down to increment/decrement value.\x1b[K");
    printf("\x1b[13;1HThe value will be saved on exit and restored on relaunch.\x1b[K");

		//Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}

  //Write out the save the file on exit
  Result rc = writeSave();

  //If there was an error, allow us to see the error message
  if (rc)
  {
    while (aptMainLoop())
    {
      hidScanInput();
      u32 kDown = hidKeysDown();
      if (kDown & KEY_START) break; // break in order to return to hbmenu
      gfxFlushBuffers();
      gfxSwapBuffers();
      gspWaitForVBlank();
    }
  }

	gfxExit();
	return 0;
}
