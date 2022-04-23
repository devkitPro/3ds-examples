#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <string.h>
#include "vshader_shbin.h"

#include "mesh.h"
#include "chunk.h"

#define CLEAR_COLOR 0x6A667DFF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))


float angleX = 0.0, angleY = 0.0;
C3D_Mtx modelView;
Chunk *planes;

void SceneInit()
{
	planes = new Chunk();
}

void SceneRender()
{
	planes->MoveAll(angleX, angleY);
	planes->FullUpdate(modelView);
}

void SceneExit()
{
	delete planes;
}

int main()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	SceneInit();

	while (aptMainLoop())
	{
		hidScanInput();
		
		u32 kDown = hidKeysDown();
		u32 keyHeld = hidKeysHeld();
        
		if (kDown & KEY_START)
			break;

		if (keyHeld & KEY_CPAD_UP)
		{
			angleY = 1.0f / 64;
		}

		else if (keyHeld & KEY_CPAD_DOWN)
		{
			angleY = -1.0f / 64;
		}

		else
		{
			angleY = 0.0;
		}

		if (keyHeld & KEY_CPAD_RIGHT)
		{
			angleX = -1.0f / 64;
		}

		else if (keyHeld & KEY_CPAD_LEFT)
		{
			angleX = 1.0f / 64;
		}

		else
		{
			angleX = 0.0f;
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(target);
			Mtx_Identity(&modelView);
			Mtx_Translate(&modelView, 0.0, -0.2f, -0.2f, true);
			Mtx_RotateY(&modelView, 90.0 * TAU / 360.0f, true);

			SceneRender();

		C3D_FrameEnd(0);
	}

	SceneExit();
	C3D_Fini();
	gfxExit();
	return 0;
}
