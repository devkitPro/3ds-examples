# Opus decoding example

This is a heavily-commented example of how to carry out fast, threaded Opus audio streaming from the filesystem (in this case, RomFS) on 3DS using libopusfile, even on O3DS.

## Necessary packages

This example uses `libopusfile` (which in turn depends on `libogg` and `libopus`) to read and decode Opus audio files. The makefile also uses `pkg-config` to discover the necessary include paths and flags for the library.

You can install the necessary packages with the following command:
```bash
pacman -S 3ds-opusfile 3ds-pkg-config
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

In addition to the detailed comments in `main.c` and the docs for [opusfile](https://www.opus-codec.org/docs/#developing-with-opusfile-api-reference) and [libctru](https://libctru.devkitpro.org/), see also [David Gow's OpenAL-based example](https://davidgow.net/hacks/opusal.html) which was indispensable when writing this code.

## Credits

Originally written by [Lauren Kelly (thejsa)](https://github.com/thejsa), with lots of help from [mtheall](https://github.com/mtheall) who re-architected the decoding and buffer logic to be much more efficient as well as overall making the code half decent :)

The sample audio included as `sample.opus` is a clip from [Paul Pitman's recording of Ludwig van Beethoven's Moonlight Sonata, Op. 27 No. 2 - I. Adagio sostenuto, sourced from Musopen](https://musopen.org/music/2547-piano-sonata-no-14-in-c-sharp-minor-moonlight-sonata-op-27-no-2/) and in the public domain (trimmed and amplified using Audacity for file size reasons, then transcoded to 96 kb/s Opus using ffmpeg).

All links retrieved on 2020-05-16.
