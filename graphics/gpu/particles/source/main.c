#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "particle_shbin.h"

#define CLEAR_COLOR 0

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float center[3]; float radius[3]; float attrib[4]; } controlPoint;

static const controlPoint point_list[] =
{
	{ { -1.0f,  0.0f, -2.0f }, { 0.0f, 0.0f, 0.0f }, { 3*8/240.0f, 3*8/400.0f, 0.0f, 1.0f } },
	{ { -0.3f, +1.5f, -4.0f }, { 0.0f, 0.1f, 0.1f }, { 3*12/240.0f, 3*12/400.0f, 0.0f, 1.0f } },
	{ { +0.3f, -1.5f, -4.0f }, { 0.1f, 0.3f, 0.3f }, { 3*16/240.0f, 3*16/400.0f, 0.0f, 0.8f } },
	{ { +1.0f,  0.0f, -2.0f }, { 0.2f, 0.5f, 0.5f }, { 3*32/240.0f, 3*32/400.0f, 0.0f, 0.0f } },
};

static const u8 point_index_list[] = { 0, 1, 2, 3 };

#define point_index_count (sizeof(point_index_list)/sizeof(point_index_list[0]))

static DVLB_s* particle_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_gsh_param, uLoc_gsh_randParam, uLoc_gsh_randSeed, uLoc_gsh_uvCoords;
static int uLoc_gsh_multiplyW, uLoc_gsh_randSphere, uLoc_gsh_noRespawn;
static C3D_Mtx projection, modelView;
static float curTime = 0.0f;
static bool multiplyW = true, randSphere = true, noRespawn = false, additiveBlend = true;

static void* vbo_data;
static void* ibo_data;

static C3D_ProcTex pt;
static C3D_ProcTexLut pt_map;
static C3D_ProcTexLut pt_noise;
static C3D_ProcTexColorLut pt_clr;

static void setupProcTex(void)
{
#define NUMCOLORS 2
	C3D_ProcTexInit(&pt, 0, NUMCOLORS);
	C3D_ProcTexClamp(&pt, GPU_PT_MIRRORED_REPEAT, GPU_PT_MIRRORED_REPEAT);
	C3D_ProcTexNoiseCoefs(&pt, C3D_ProcTex_UV, 0.1f, 0.5f, 0.1f);
	C3D_ProcTexCombiner(&pt, false, GPU_PT_SQRT2, GPU_PT_SQRT2);
	C3D_ProcTexFilter(&pt, GPU_PT_LINEAR);
	C3D_ProcTexBind(0, &pt);

	float data[129];
	int i;
	for (i = 0; i <= 128; i ++)
	{
		float x = i/128.0f;
		data[i] = x;
	}
	ProcTexLut_FromArray(&pt_map, data);
	C3D_ProcTexLutBind(GPU_LUT_RGBMAP, &pt_map);

	// Noise smooth step equation
	for (i = 0; i <= 128; i ++)
	{
		float x = i/128.0f;
		data[i] = x*x*(3-2*x);
	}
	ProcTexLut_FromArray(&pt_noise, data);
	C3D_ProcTexLutBind(GPU_LUT_NOISE, &pt_noise);

	u32 colors[NUMCOLORS];
	colors[0] = 0xFFBABABA;
	colors[1] = 0x00888888;

	ProcTexColorLut_Write(&pt_clr, colors, 0, NUMCOLORS);
	C3D_ProcTexColorLutBind(&pt_clr);
}

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	particle_dvlb = DVLB_ParseFile((u32*)particle_shbin, particle_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &particle_dvlb->DVLE[0]);
	shaderProgramSetGsh(&program, &particle_dvlb->DVLE[1], 0);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection     = shaderInstanceGetUniformLocation(program.vertexShader,   "projection");
	uLoc_modelView      = shaderInstanceGetUniformLocation(program.vertexShader,   "modelView");
	uLoc_gsh_param      = shaderInstanceGetUniformLocation(program.geometryShader, "param");
	uLoc_gsh_randParam  = shaderInstanceGetUniformLocation(program.geometryShader, "randParam");
	uLoc_gsh_randSeed   = shaderInstanceGetUniformLocation(program.geometryShader, "randSeed");
	uLoc_gsh_uvCoords   = shaderInstanceGetUniformLocation(program.geometryShader, "uvCoords");
	uLoc_gsh_multiplyW  = shaderInstanceGetUniformLocation(program.geometryShader, "multiplyW");
	uLoc_gsh_randSphere = shaderInstanceGetUniformLocation(program.geometryShader, "randSphere");
	uLoc_gsh_noRespawn  = shaderInstanceGetUniformLocation(program.geometryShader, "noRespawn");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=center
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1=radius
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 4); // v2=attrib

	// Create the VBO (vertex buffer object)
	vbo_data = linearAlloc(sizeof(point_list));
	memcpy(vbo_data, point_list, sizeof(point_list));
	ibo_data = linearAlloc(sizeof(point_index_list));
	memcpy(ibo_data, point_index_list, sizeof(point_index_list));

	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(controlPoint), 3, 0x210);

	// Configure the first fragment shading substage to just pass through the proctex color
	// and blend the alpha coming from proctex and the vertex color
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE3, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE3, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
	setupProcTex();
}

static void sceneRender(float iod)
{
	// Compute the matrices
	Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.01f, 20.0f, iod, 3.0f, false);
	Mtx_Identity(&modelView);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,   uLoc_projection,     &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,   uLoc_modelView,      &modelView);
	C3D_FVUnifSet   (GPU_GEOMETRY_SHADER, uLoc_gsh_param,      100.0f, curTime, 1.0f, 1.0f);
	C3D_FVUnifSet   (GPU_GEOMETRY_SHADER, uLoc_gsh_randParam,  17, 37, 65535, 1.0f/65535);
	C3D_FVUnifSet   (GPU_GEOMETRY_SHADER, uLoc_gsh_randSeed,   45, 47.564, 46.15, 45.9875);
	C3D_FVUnifSet   (GPU_GEOMETRY_SHADER, uLoc_gsh_uvCoords+0, 3.0, 1.0, 3.0, 3.0);
	C3D_FVUnifSet   (GPU_GEOMETRY_SHADER, uLoc_gsh_uvCoords+1, 1.0, 1.0, 1.0, 3.0);
	C3D_BoolUnifSet (GPU_GEOMETRY_SHADER, uLoc_gsh_multiplyW,  multiplyW);
	C3D_BoolUnifSet (GPU_GEOMETRY_SHADER, uLoc_gsh_randSphere, randSphere);
	C3D_BoolUnifSet (GPU_GEOMETRY_SHADER, uLoc_gsh_noRespawn,  noRespawn);

	if (additiveBlend)
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE, GPU_SRC_ALPHA, GPU_ONE);
	else
		C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);

	// Draw the particles
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_COLOR);
	C3D_DrawElements(GPU_GEOMETRY_PRIM, point_index_count, C3D_UNSIGNED_BYTE, ibo_data);
}

static void sceneExit(void)
{
	// Free the data
	linearFree(vbo_data);
	linearFree(ibo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(particle_dvlb);
}

int main()
{
	// Initialize graphics
	gfxInitDefault();
	gfxSet3D(true); // Enable stereoscopic 3D
	consoleInit(GFX_BOTTOM, NULL);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the render targets
	C3D_RenderTarget* targetLeft  = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTarget* targetRight = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(targetLeft,  GFX_TOP, GFX_LEFT,  DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTargetSetOutput(targetRight, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);

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

		float slider = osGet3DSliderState();
		float iod = slider/4;

		if (!(kHeld & KEY_L))
			curTime += 1/128.0f;
		if (kDown & KEY_A)
			curTime = 0.0f;

		if (kDown & KEY_X)
			noRespawn = !noRespawn;
		if (kDown & KEY_Y)
			randSphere = !randSphere;
		if (kDown & KEY_B)
			multiplyW = !multiplyW;
		if (kDown & KEY_R)
			additiveBlend = !additiveBlend;

		printf("\x1b[3;1Htime: %f  \n", curTime);
		printf("\x1b[4;1HnoRespawn:     %s \n", noRespawn     ? "Yes" : "No");
		printf("\x1b[5;1HrandSphere:    %s \n", randSphere    ? "Yes" : "No");
		printf("\x1b[6;1HmultiplyW:     %s \n", multiplyW     ? "Yes" : "No");
		printf("\x1b[7;1HadditiveBlend: %s \n", additiveBlend ? "Yes" : "No");
		printf("\x1b[29;1Hgpu: %5.2f%%  cpu: %5.2f%%  buf:%5.2f%%\n", C3D_GetDrawingTime()*6, C3D_GetProcessingTime()*6, C3D_GetCmdBufUsage()*100);

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{
			C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(targetLeft);
			sceneRender(-iod);

			if (iod > 0.0f)
			{
				C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
				C3D_FrameDrawOn(targetRight);
				sceneRender(iod);
			}
		}
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize graphics
	C3D_Fini();
	gfxExit();
	return 0;
}
