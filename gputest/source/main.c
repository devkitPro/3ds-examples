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

#define EXTENDED_TOPSCR_RESOLUTION

#ifndef EXTENDED_TOPSCR_RESOLUTION
#define TOPSCR_WIDTH 240
#define TOPSCR_COPYFLAG 0x00001000
#else
#define TOPSCR_WIDTH (240*2)
#define TOPSCR_COPYFLAG 0x01001000
#endif

#define CLEAR_COLOR 0x80FF80FF

C3D_RenderBuf rbTop, rbBot;

static DVLB_s *pVsh, *pGsh;
static shaderProgram_s shader;

C3D_MtxStack projMtx, mdlvMtx;

typedef struct
{
	float position[3];
	float texCoord[2];
	float color[3];
} vertex_t;

static const vertex_t vertex_list[] =
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

#define vertex_list_count (sizeof(vertex_list)/sizeof(vertex_list[0]))

#define FOVY (2.0f/15)

static void* myVbo;
static C3D_Tex myTex;

static void drawScene(float trX, float trY)
{
	C3D_RenderBufBind(&rbTop);

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

	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);

	C3D_Flush();
}

static void drawSceneBottom(float trX, float trY)
{
	C3D_RenderBufBind(&rbBot);

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

	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);

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

	C3D_RenderBufInit(&rbTop, TOPSCR_WIDTH, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderBufInit(&rbBot, 240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	rbTop.clearColor = CLEAR_COLOR;
	rbBot.clearColor = CLEAR_COLOR;

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	MtxStack_Init(&projMtx);
	MtxStack_Bind(&projMtx, GPU_VERTEX_SHADER, VSH_FVEC_projMtx, VSH_ULEN_projMtx);
	MtxStack_Init(&mdlvMtx);
	MtxStack_Bind(&mdlvMtx, GPU_VERTEX_SHADER, VSH_FVEC_mdlvMtx, VSH_ULEN_mdlvMtx);

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
	C3D_BindProgram(&shader);

	// Configure attributes
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // texcoord
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // vertex color

	// Configure VBO
	myVbo = linearAlloc(sizeof(vertex_list));
	memcpy(myVbo, vertex_list, sizeof(vertex_list));

	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, myVbo, sizeof(vertex_t), 3, 0x210);

	// Clear renderbuffers
	C3D_RenderBufClear(&rbTop);
	C3D_RenderBufClear(&rbBot);

	float trX = 0, trY = 0;
	float zDist = 0.1f;

	// Main loop
	while (aptMainLoop())
	{
		C3D_VideoSync();
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

		float slider = osGet3DSliderState();
		float czDist = zDist*slider/2;

		drawScene(trX-czDist, trY);
		C3D_RenderBufTransfer(&rbTop, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), TOPSCR_COPYFLAG);

		if (slider > 0.0f)
		{
			C3D_RenderBufClear(&rbTop);
			drawScene(trX+czDist, trY);
			C3D_RenderBufTransfer(&rbTop, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), TOPSCR_COPYFLAG);
		}

		C3D_RenderBufClear(&rbTop); // In theory this could be async but meh...
		
#ifndef DEBUG
		drawSceneBottom(trX, trY);
		C3D_RenderBufTransfer(&rbBot, (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), 0x1000);
		C3D_RenderBufClear(&rbBot); // Same here
#endif
	}

	C3D_Fini();
	gfxExit();
	return 0;
}
