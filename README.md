# 3ds-examples

Examples for 3DS using devkitARM, libctru and citro3d

<p align="center"><a href="http://creativecommons.org/publicdomain/mark/1.0/"><img src="http://i.creativecommons.org/p/mark/1.0/88x31.png" alt="Public Domain Mark"></a></p>

## Build instructions

[Box2d](https://github.com/erincatto/box2d), [OpusFile](https://opus-codec.org/) and [ModPlug](http://modplug-xmms.sourceforge.net/) are already built for 3DS using [pacman packages](https://github.com/devkitPro/pacman-packages/tree/master/3ds).

Compile all of the examples above using (either with `dkp-pacman` or `pacman`):

```bash
sudo pacman -S 3ds-box2d 3ds-opusfile 3ds-libmodplug
make
```
