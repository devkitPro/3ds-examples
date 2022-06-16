#include <3ds.h>

#include "filesystem.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>

static void listDirItems(const char* directory)
{
    std::vector<std::string> items;
    FileSystem::GetDirectoryItems(directory, items);

    printf("Directory listing of %s", directory);
    for (const auto& filename : items)
        printf("  -> %s", filename.c_str());
}

static const char* getInfoType(const FileSystem::Info& info)
{
    switch (info.type)
    {
        case FileSystem::FileType_File:
            return "file";
        case FileSystem::FileType_Directory:
            return "directory";
        case FileSystem::FileType_SymLink:
            return "symlink";
        case FileSystem::FileType_Other:
            return "other";
        default:
            return "";
    }
}

static void getInfo(const char* filepath)
{
    FileSystem::Info info;

    bool success = FileSystem::GetInfo(filepath, info);

    printf("File Info of %s", filepath);

    if (success)
    {
        printf("  FileType: %s", getInfoType(info));
        printf("  File Size: %lld", info.size);
    }
    else
        printf("  Failed to get info for %s!", filepath);
}

int main(int argc, char** argv)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    if (!PHYSFS_init(argv[0]))
        printf("physfs failure: %s!", FileSystem::GetPhysfsError());

    FileSystem::SetSource("game");
    FileSystem::SetIdentity("game", true);

    /* create a directory */
    bool success = FileSystem::CreateDirectory("MyDir");
    printf("Directory Created: %d", success);

    /* create a file and write to it */
    FileSystem::File file{};
    FileSystem::OpenFile(file, "MyFile.txt", FileSystem::FileMode_Write);

    const char* message = "HELLO WORLD!";
    size_t length       = strlen(message);

    FileSystem::WriteFile(file, message, length);
    success = FileSystem::CloseFile(file);
    printf("File Closed: %d", success);

    /* open our file for reading */
    FileSystem::OpenFile(file, "MyFile.txt", FileSystem::FileMode_Read);
    char buffer[file.GetSize()];

    /* read the file contents and print them */
    int64_t size = FileSystem::ReadFile(file, buffer, file.GetSize());

    if (size != 0)
        printf("File Contents: %s", buffer);

    while (aptMainLoop())
    {
        hidScanInput();
        uint32_t kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;
        else if (kDown & KEY_A)
            listDirItems("");
        else if (kDown & KEY_B)
            getInfo("MyFile.txt");
        else if (kDown & KEY_X)
            getInfo("WEEYOU");

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();

        // Wait for VBlank
        gspWaitForVBlank();
    }

    // Exit services
    gfxExit();

    return 0;
}
