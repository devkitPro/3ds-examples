#include <string.h>
#include <3ds.h>
#include <citro3d.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "test_vsh_shbin.h"
#include "test_vsh.shader.h"
#include "test_gsh_shbin.h"
#include "test_gsh.shader.h"
#include "grass_bin.h"

#define VAR_3D_SLIDERSTATE (*(volatile float*)0x1FF81080)
#define EXTENDED_TOPSCR_RESOLUTION

#ifndef EXTENDED_TOPSCR_RESOLUTION
#define TOPSCR_WIDTH 240
#define TOPSCR_COPYFLAG 0x00001000
#else
#define TOPSCR_WIDTH (240*2)
#define TOPSCR_COPYFLAG 0x01001000
#endif

u32* gpuDepthBuf;
u32* gpuColorBuf;

static DVLB_s *pVsh, *pGsh;
static shaderProgram_s shader;

C3D_MtxStack projMtx, mdlvMtx;

typedef struct
{
	float position[3];
	float texCoord[2];
	float color[3];
} vertex_t;

static const vertex_t vertices[] =
{
	// First triangle
	{{-0.5f, +0.5f, -4.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-0.5f, -0.5f, -4.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{+0.5f, -0.5f, -4.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},

	// Second triangle
	{{+0.5f, -0.5f, -4.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{+0.5f, +0.5f, -4.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
	{{-0.5f, +0.5f, -4.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
};

//#define BG_COLOR 0x68B0D8FF
#define BG_COLOR 0x80FF80FF

#define FOVY (2.0f/15)

static inline void clearGpuBuffers(u32 color)
{
	GX_SetMemoryFill(NULL, gpuColorBuf, color, gpuColorBuf+0x2EE00, 0x201, gpuDepthBuf, 0x00000000, gpuDepthBuf+0x2EE00, 0x201);
	gspWaitForPSC0();
}

static C3D_VBO myVbo;
static C3D_Tex myTex;

static void drawScene(float trX, float trY)
{
	GPU_SetViewport((u32*)osConvertVirtToPhys((u32)gpuDepthBuf), (u32*)osConvertVirtToPhys((u32)gpuColorBuf), 0, 0, TOPSCR_WIDTH, 400);

	C3D_TexBind(0, &myTex);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_TEXTURE0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

	Mtx_PerspTilt(MtxStack_Cur(&projMtx), C3D_Angle(FOVY), C3D_AspectRatioTop, 0.01f, 1000.0f);
	Mtx_Identity(MtxStack_Cur(&mdlvMtx));
	Mtx_Translate(MtxStack_Cur(&mdlvMtx), trX, trY, 0.0f);

	MtxStack_Update(&projMtx);
	MtxStack_Update(&mdlvMtx);

	C3D_DrawArray(&myVbo, GPU_TRIANGLES);

	C3D_Flush();
}

static void drawSceneBottom(float trX, float trY)
{
	GPU_SetViewport((u32*)osConvertVirtToPhys((u32)gpuDepthBuf), (u32*)osConvertVirtToPhys((u32)gpuColorBuf), 0, 0, 240/**2*/, 320);

	C3D_TexBind(0, NULL);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

	Mtx_PerspTilt(MtxStack_Cur(&projMtx), C3D_Angle(FOVY), C3D_AspectRatioBot, 0.01f, 1000.0f);
	Mtx_Identity(MtxStack_Cur(&mdlvMtx));
	Mtx_Translate(MtxStack_Cur(&mdlvMtx), trX, trY, 0.0f);

	MtxStack_Update(&projMtx);
	MtxStack_Update(&mdlvMtx);

	C3D_DrawArray(&myVbo, GPU_TRIANGLES);

	C3D_Flush();
}

int main()
{
	gfxInitDefault();
	gfxSet3D(true); // uncomment if using stereoscopic 3D

#ifdef DEBUG
	consoleInit(GFX_BOTTOM, NULL);
	printf("Testing...\n");
#endif

	gpuDepthBuf = (u32*)vramAlloc(400*240*2*sizeof(float));
	gpuColorBuf = (u32*)vramAlloc(400*240*2*sizeof(u32));

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	MtxStack_Init(&projMtx);
	MtxStack_Bind(&projMtx, VSH_FVEC_projMtx, VSH_ULEN_projMtx);
	MtxStack_Init(&mdlvMtx);
	MtxStack_Bind(&mdlvMtx, VSH_FVEC_mdlvMtx, VSH_ULEN_mdlvMtx);

	C3D_TexInit(&myTex, 64, 64, GPU_RGBA8);
	C3D_TexUpload(&myTex, grass_bin);
	//C3D_TexFlush(&myTex)
	// ^ Not needed! FlushAndRun() already does it

	// Load shaders
	pVsh = DVLB_ParseFile((u32*)test_vsh_shbin, test_vsh_shbin_size);
	pGsh = DVLB_ParseFile((u32*)test_gsh_shbin, test_gsh_shbin_size);
	shaderProgramInit(&shader);
	shaderProgramSetVsh(&shader, &pVsh->DVLE[0]);
	shaderProgramSetGsh(&shader, &pGsh->DVLE[0], 3*5); // Comment this out to disable the geoshader
	shaderProgramUse(&shader);

	// Configure attributes
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddParam(attrInfo, GPU_FLOAT, 3); // position
	AttrInfo_AddParam(attrInfo, GPU_FLOAT, 2); // texcoord
	AttrInfo_AddParam(attrInfo, GPU_FLOAT, 3); // vertex color
	AttrInfo_AddBuffer(attrInfo, 0, sizeof(vertex_t), 3, 0x210);

	// Configure VBO
	C3D_VBOInit(&myVbo, sizeof(vertices));
	C3D_VBOAddData(&myVbo, vertices, sizeof(vertices), sizeof(vertices)/sizeof(vertex_t));

	clearGpuBuffers(BG_COLOR);

	float trX = 0, trY = 0;
	float zDist = 0.1f;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffersGpu();
		hidScanInput();

		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kHeld & KEY_UP)
			trY += 0.05f;
		if (kHeld & KEY_DOWN)
			trY -= 0.05f;
		if (kHeld & KEY_LEFT)
			trX -= 0.05f;
		if (kHeld & KEY_RIGHT)
			trX += 0.05f;
		if (kHeld & KEY_L)
			zDist -= 0.005f;
		if (kHeld & KEY_R)
			zDist += 0.005f;

		float slider = VAR_3D_SLIDERSTATE;
		float czDist = zDist*slider/2;

		drawScene(trX-czDist, trY);

		GX_SetDisplayTransfer(NULL, gpuColorBuf, GX_BUFFER_DIM(TOPSCR_WIDTH,400), (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), GX_BUFFER_DIM(TOPSCR_WIDTH,400), TOPSCR_COPYFLAG);
		gspWaitForPPF();

		if (slider > 0.0f)
		{
			clearGpuBuffers(BG_COLOR);

			drawScene(trX+czDist, trY);

			GX_SetDisplayTransfer(NULL, gpuColorBuf, GX_BUFFER_DIM(TOPSCR_WIDTH,400), (u32*)gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), GX_BUFFER_DIM(TOPSCR_WIDTH,400), TOPSCR_COPYFLAG);
			gspWaitForPPF();
		}
		
#ifndef DEBUG
		clearGpuBuffers(BG_COLOR);

		drawSceneBottom(trX, trY);

		GX_SetDisplayTransfer(NULL, gpuColorBuf, GX_BUFFER_DIM(240,320), (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), GX_BUFFER_DIM(240,320), 0x1000);
		gspWaitForPPF();
#endif

		clearGpuBuffers(BG_COLOR);
	}

	C3D_Fini();
	gfxExit();
	return 0;
}
