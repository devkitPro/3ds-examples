#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skybox_shbin.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float position[3]; } vertex;

static const vertex vertex_list[] =
{
	// First face (PZ)
	// First triangle
	{ {-0.5f, -0.5f, +0.5f} },
	{ {+0.5f, -0.5f, +0.5f} },
	{ {+0.5f, +0.5f, +0.5f} },
	// Second triangle
	{ {+0.5f, +0.5f, +0.5f} },
	{ {-0.5f, +0.5f, +0.5f} },
	{ {-0.5f, -0.5f, +0.5f} },

	// Second face (MZ)
	// First triangle
	{ {-0.5f, -0.5f, -0.5f} },
	{ {-0.5f, +0.5f, -0.5f} },
	{ {+0.5f, +0.5f, -0.5f} },
	// Second triangle
	{ {+0.5f, +0.5f, -0.5f} },
	{ {+0.5f, -0.5f, -0.5f} },
	{ {-0.5f, -0.5f, -0.5f} },

	// Third face (PX)
	// First triangle
	{ {+0.5f, -0.5f, -0.5f} },
	{ {+0.5f, +0.5f, -0.5f} },
	{ {+0.5f, +0.5f, +0.5f} },
	// Second triangle
	{ {+0.5f, +0.5f, +0.5f} },
	{ {+0.5f, -0.5f, +0.5f} },
	{ {+0.5f, -0.5f, -0.5f} },

	// Fourth face (MX)
	// First triangle
	{ {-0.5f, -0.5f, -0.5f} },
	{ {-0.5f, -0.5f, +0.5f} },
	{ {-0.5f, +0.5f, +0.5f} },
	// Second triangle
	{ {-0.5f, +0.5f, +0.5f} },
	{ {-0.5f, +0.5f, -0.5f} },
	{ {-0.5f, -0.5f, -0.5f} },

	// Fifth face (PY)
	// First triangle
	{ {-0.5f, +0.5f, -0.5f} },
	{ {-0.5f, +0.5f, +0.5f} },
	{ {+0.5f, +0.5f, +0.5f} },
	// Second triangle
	{ {+0.5f, +0.5f, +0.5f} },
	{ {+0.5f, +0.5f, -0.5f} },
	{ {-0.5f, +0.5f, -0.5f} },

	// Sixth face (MY)
	// First triangle
	{ {-0.5f, -0.5f, -0.5f} },
	{ {+0.5f, -0.5f, -0.5f} },
	{ {+0.5f, -0.5f, +0.5f} },
	// Second triangle
	{ {+0.5f, -0.5f, +0.5f} },
	{ {-0.5f, -0.5f, +0.5f} },
	{ {-0.5f, -0.5f, -0.5f} },
};

#define vertex_list_count (sizeof(vertex_list)/sizeof(vertex_list[0]))

static DVLB_s* skybox_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static C3D_Mtx projection;

static void* vbo_data;
static float angleX = 0.0, angleY = 0.0;

static C3D_Tex skybox_tex;
static C3D_TexCube skybox_cube;

// Helper function for loading a texture from a t3x file
static bool loadTextureFromFile(C3D_Tex* tex, C3D_TexCube* cube, const char* path)
{
	FILE* f = fopen(path, "rb");
	if (!f)
		return false;

	Tex3DS_Texture t3x = Tex3DS_TextureImportStdio(f, tex, cube, false);
	fclose(f);
	if (!t3x)
		return false;

	// Delete the t3x object since we don't need it
	Tex3DS_TextureFree(t3x);
	return true;
}

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	skybox_dvlb = DVLB_ParseFile((u32*)skybox_shbin, skybox_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &skybox_dvlb->DVLE[0]);
	C3D_BindProgram(&program);
	C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);

	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position

	// Compute the projection matrix
	Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(45.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

	// Create the VBO (vertex buffer object)
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));

	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 1, 0x0);

	// Load the skybox texture and bind it to the first texture unit
	if (!loadTextureFromFile(&skybox_tex, &skybox_cube, "romfs:/gfx/skybox.t3x"))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&skybox_tex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&skybox_tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
	C3D_TexBind(0, &skybox_tex);

	// Configure the first fragment shading substage to use the texture color.
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

static void sceneRender(void)
{
	// Calculate the modelView matrix
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);

	// Draw the VBO
	C3D_CullFace(GPU_CULL_FRONT_CCW);
	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);
}

static void sceneExit(void)
{
	// Free the texture
	C3D_TexDelete(&skybox_tex);

	// Free the VBO
	linearFree(vbo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(skybox_dvlb);
}

int main()
{
	// Initialize libs
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the render target
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// Initialize the scene
	sceneInit();

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kHeld & KEY_UP)
		{
			angleX -= C3D_AngleFromDegrees(2.0f);
			if (angleX < C3D_AngleFromDegrees(-90.0f))
				angleX = C3D_AngleFromDegrees(-90.0f);
		}
		else if (kHeld & KEY_DOWN)
		{
			angleX += C3D_AngleFromDegrees(2.0f);
			if (angleX > C3D_AngleFromDegrees(90.0f))
				angleX = C3D_AngleFromDegrees(90.0f);
		}

		if (kHeld & KEY_LEFT)
			angleY -= C3D_AngleFromDegrees(2.0f);
		else if (kHeld & KEY_RIGHT)
			angleY += C3D_AngleFromDegrees(2.0f);

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(target);
			sceneRender();
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize libs
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}
