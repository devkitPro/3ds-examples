# Use
It is **required** that you have the devKitARM environment variable set up on your machine to build these examples.
- This requires that you install [devKitPro](https://devkitpro.org/ "devKitPro").
 - You can find information about installing devKitPro on the [wiki](https://devkitpro.org/wiki/Getting_Started "wiki").
- To set the environment variable on Windows, copy the absolute file path of the `devkitARM\bin` folder and add it to the Path variable in environmental variables.

With the envrionment set, navigate to your desired example folder and run  `make` in your command line interface where there exists a makefile.

This will generate:
- A `.3dsx` file, which is 3ds executable file for use with the Homebrew launcher or Citra
- A `.elf` file, a binary executable file
- A `.smdh` file, containing metadata about the executables (Version, Title, Region, etc)

#### Use with Citra
Citra is an open-source 3DS emulator you can download [here](https://citra-emu.org/download/ "here")

To test generated executables, simply drag the `.3dsx` or `.elf` file into Citra.

Please note that since these examples were developed with the intention of being used on actual 3ds hardware, some examples may not be fully functional on Citra.

#### Use with 3ds
Assuming you have [Homebrew](https://3ds.hacks.guide/ "Homebrew") installed on the 3ds, move or copy the generated `.3dsx` file into the `/3ds` folder your 3ds SD/microsd card.

Open your Homebrew launcher, navigate to your application, and launch.

# Examples and Descriptions
- `app_launch/` : Example for launching other apps inside your homebrew application. Must change titleID if your 3ds is not European.
- `audio/` : Examples for using the 3ds microphone and outputting audio using filters/changing pitch.
- `camera/` : Examples for using the 3ds camera to take pictures and videos.
- `get_system_language/` : Example for getting the 3ds system language.
- `graphics/` : Examples related to graphics.
	- `graphics/bitmap/24bit-color/` : Creates a bitmap example from image on bottom screen.
	- `graphics/printing/` : Examples related to text output (font. size, color, custom).
	- `graphics/gpu/` : Examples related to drawing shapes in 2d and 3d, movement, and textures output.
- `input/` : Includes examples with 3ds input (keyboard, touch screen, reading).
- `libapplet_launch/` : Examples for launching built in 3ds libraries (Photo Selector, Sound Selector, eShop, keyboard, Mii Editor, etc).
- `mii_selector/` : Example for launching the 3ds mii selector.
- `mvd/` : Video processing and color-format conversion for New3DS systems.
- `network/` : Examples for network related functions such as sending POST requests, SpotPass, downloading, etc.
- `nfc/` : Example for using the New3DS nfc reader for amiibo functionality.
- `physics/box2d` : Small physics example using box2d library.
- `qtm/` : Example for using the New3DS system module in charge of handling head tracking.
- `romfs/` : Example of reading a file using the 3ds file stream.
- `sdmc/` : Example for accessing SD contents using libctru.
- `templates/` : Contains template code for starting a 3DS libctru and library project.
- `threads/` : Contains examples for threading (creating, handling threads).
- `time/rtc/` : Example for getting the current time, date, weekday, month, and year of the 3ds.

# Questions
Please ask questions using the [3ds development forum on devKitPro](https://devkitpro.org/viewforum.php?f=39 "3ds development forum on devKitPro").


<p align="center"><a href="http://creativecommons.org/publicdomain/mark/1.0/"><img src="http://i.creativecommons.org/p/mark/1.0/88x31.png" alt="Public Domain Mark"></a></p>
