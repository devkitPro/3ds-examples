This example is for using New3DS MVD, for hardware color-format conversion + video processing.

To build this you need "romfs:/video.h264", this isn't included in git currently. See the source for size limits. You can use this for generating that: "avconv -i {input video} -s 240x400 -vf transpose=1 romfs/video.h264"

To actually use this example on New3DS, you must either run under modded-FIRM which allows you to access MVDSTD, or ninjhax 1.x(\*hax payloads >=v2.x currently don't have a way implemented to access MVDSTD). The process you run this under must also have kernel-release-version >=2.44(FIRM v8.0) in the exheader, such as System Settings.

Video playback with this is *very* *slow* currently due to displaying each frame immediately after rendering without any buffering.
