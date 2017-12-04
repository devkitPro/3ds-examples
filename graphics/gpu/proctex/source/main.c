#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include "vshader_shbin.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection;
static C3D_Mtx projection;

static C3D_ProcTex pt;
static C3D_ProcTexLut pt_map;
static C3D_ProcTexLut pt_noise;
static C3D_ProcTexColorLut pt_clr;

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

	// Configure attributes for use with the vertex shader
	// Attribute format and element count are ignored in immediate mode
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1=texcoord0

	// Compute the projection matrix
	Mtx_OrthoTilt(&projection, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0, true);

#define NUMCOLORS 2
	C3D_ProcTexInit(&pt, 0, NUMCOLORS);
	C3D_ProcTexClamp(&pt, GPU_PT_MIRRORED_REPEAT, GPU_PT_MIRRORED_REPEAT);
	C3D_ProcTexNoiseCoefs(&pt, C3D_ProcTex_UV, 0.1f, 0.3f, 0.1f);
	C3D_ProcTexCombiner(&pt, false, GPU_PT_SQRT2, GPU_PT_SQRT2); // <- for wood rings
	//C3D_ProcTexCombiner(&pt, false, GPU_PT_V, GPU_PT_V); // <- for wood stripes (or also _U)
	C3D_ProcTexFilter(&pt, GPU_PT_LINEAR);
	C3D_ProcTexBind(0, &pt);

	float data[129];
	int i;
	for (i = 0; i <= 128; i ++)
	{
		float x = i/128.0f;
		data[i] = fabsf(sinf(C3D_Angle(6*(x+0.125f)))); // 6*2 = 12 stripes
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
	colors[0] = 0xFF1F1F4F; // dark brown
	colors[1] = 0xFF1F9ED1; // light brown

	ProcTexColorLut_Write(&pt_clr, colors, 0, NUMCOLORS);
	C3D_ProcTexColorLutBind(&pt_clr);

	// Configure the first fragment shading substage to just pass through the procedural texture color
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE3, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

static void sceneRender(void)
{
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

	// Draw the triangle directly
	//float min = -1.0f;  //<-- for (symmetrical) wood rings
	//float max = +1.0f;
	float min = 1.0f; //<-- for asymmetrical wood rings
	float max = 3.0f;
	//float min =  0.0f; //<-- for wood stripes
	//float max = +3.0f; // triplicate stripes
	float size = 200.0f/2;
	C3D_ImmDrawBegin(GPU_TRIANGLE_STRIP);
		C3D_ImmSendAttrib(200.0f-size, 120.0f+size, 0.5f, 0.0f); // v0=position
		C3D_ImmSendAttrib(min, min, 0.0f, 0.0f);                   // v1=texcoord0

		C3D_ImmSendAttrib(200.0f+size, 120.0f+size, 0.5f, 0.0f);
		C3D_ImmSendAttrib(max, min, 0.0f, 0.0f);

		C3D_ImmSendAttrib(200.0f-size, 120.0f-size, 0.5f, 0.0f);
		C3D_ImmSendAttrib(min, max, 0.0f, 0.0f);

		C3D_ImmSendAttrib(200.0f+size, 120.0f-size, 0.5f, 0.0f);
		C3D_ImmSendAttrib(max, max, 0.0f, 0.0f);
	C3D_ImmDrawEnd();
}

static void sceneExit(void)
{
	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

int main()
{
	// Initialize graphics
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
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
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		printf("\x1b[29;1Hgpu: %5.2f%%  cpu: %5.2f%%  buf:%5.2f%%\n", C3D_GetDrawingTime()*3, C3D_GetProcessingTime()*3, C3D_GetCmdBufUsage()*100);

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(target);
			sceneRender();
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize graphics
	C3D_Fini();
	gfxExit();
	return 0;
}
