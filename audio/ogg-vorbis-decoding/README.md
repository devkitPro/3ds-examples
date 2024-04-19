# Ogg Vorbis decoding example

This is a heavily-commented example of how to carry out fast, threaded Ogg Vorbis audio streaming from the filesystem (in this case, RomFS) on 3DS using libvorbisidec, even on O3DS.

## Necessary packages

This example uses `libvorbisidec` (which in turn depends on `libogg`) to read and decode Ogg Vorbis audio files. The makefile also uses `pkg-config` to discover the necessary include paths and flags for the library.

You can install the necessary packages with the following command:
```bash
pacman -S 3ds-libvorbisidec 3ds-pkg-config
```

Note that on some systems, you may need to use `dkp-pacman` instead of `pacman`, and you may need to prefix the installation commands with `sudo`.

Additionally, if you do not already have `pkg-config` installed on your *host system*, you will need to install it using your package manager.

On Windows:
```bash
pacman -S pkg-config
```

On macOS:
```bash
sudo dkp-pacman -S pkg-config
```

## Further reading

In addition to the detailed comments in `main.c`, see the docs for [libctru](https://libctru.devkitpro.org/) and the not-quite-matching-but-close ones for [libvorbisfile](https://www.xiph.org/vorbis/doc/vorbisfile/index.html).

## Credits

Originally written for Opus decoding by [Lauren Kelly (thejsa)](https://github.com/thejsa), with help from [mtheall](https://github.com/mtheall), adapted to Vorbis decoding by [Théo B. (LiquidFenrir)](https://github.com/LiquidFenrir)

The sample audio included as `sample.ogg` is [The Internet Memory Foundation's recording of Johan Sebastian Bach's Orchestral Suite no. 2 in B minor, BWV 1067 - 7. Badinerie, sourced from Musopen](https://musopen.org/music/3774-orchestral-suite-no-2-in-b-minor-bwv-1067/) and in the public domain (transcoded to mono Ogg Vorbis using Audacity, as ffmpeg's vorbis encoder only supports stereo).
