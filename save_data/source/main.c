/*
	Save data example made by TheGag96 for libctru
	This code was modified for the last time on: 11/10/2021 19:42 CST
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

  Result rc = FSUSER_FormatSaveData(ARCHIVE_SAVEDATA, EMPTY_PATH, 128 /* x 512 bytes */, 7, 7, 7, 7, false);

  if (rc) {
    printf("\x1b[16;1Hcouldn't format: %08lX\x1b[K", rc);
    return false;
  }

  rc = FSUSER_OpenArchive(&archiveHandle, ARCHIVE_SAVEDATA, EMPTY_PATH);

  if (rc) {
    printf("\x1b[16;1Hcouldn't open archive: %08lX\x1b[K", rc);
    return false;
  }

  SaveFile saveFile;
  saveFile.hours = 7;

  const char* sgPathText = "/save1.dat";
  FS_Path sgPath = fsMakePath(PATH_ASCII, sgPathText);

  rc = FSUSER_CreateFile(archiveHandle, sgPath, 0, sizeof(saveFile));

  if (rc) {
    printf("\x1b[16;1Hcouldn't create file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return false;
  }

  Handle fileHandle;

  rc = FSUSER_OpenFile(&fileHandle, archiveHandle, sgPath, 7, 0);

  if (rc) {
    printf("\x1b[16;1Hcouldn't open file: %08lX\x1b[K", rc);
    FSUSER_CloseArchive(archiveHandle);
    return false;
  }

  u32 bytesWritten;
  rc = FSFILE_Write(fileHandle, &bytesWritten, 0, &saveFile, sizeof(saveFile), 0);

  if (rc) {
    printf("\x1b[16;1Hcouldn't write file: %08lX\x1b[K", rc);
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

  createSave();

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

	gfxExit();
	return 0;
}
