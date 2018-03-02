#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <string.h>
#include "vshader_shbin.h"
#include "lenny.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static C3D_Mtx projection;

static C3D_LightEnv lightEnv;
static C3D_Light light;
static C3D_LightLut lut_Spec;

static C3D_AttrInfo vbo_attrInfo;
static C3D_BufInfo vbo_bufInfo;
static void* vbo_data;
static float angleX = 0.0, angleY = 0.0;

static C2D_TextBuf staticTextBuf;
static C2D_Text txt_helloWorld;

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);

	// Get the location of the uniforms
	uLoc_projection   = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView    = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	// Configure attributes for use with the vertex shader
	AttrInfo_Init(&vbo_attrInfo);
	AttrInfo_AddLoader(&vbo_attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(&vbo_attrInfo, 1, GPU_FLOAT, 3); // v1=normal

	// Create the VBO (vertex buffer object)
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));

	// Configure buffers
	BufInfo_Init(&vbo_bufInfo);
	BufInfo_Add(&vbo_bufInfo, vbo_data, sizeof(vertex), 2, 0x10);

	static const C3D_Material material =
	{
		{ 0.1f, 0.1f, 0.1f }, //ambient
		{ 0.4f, 0.4f, 0.4f }, //diffuse
		{ 0.5f, 0.5f, 0.5f }, //specular0
		{ 0.0f, 0.0f, 0.0f }, //specular1
		{ 0.0f, 0.0f, 0.0f }, //emission
	};

	C3D_LightEnvInit(&lightEnv);
	C3D_LightEnvMaterial(&lightEnv, &material);

	LightLut_Phong(&lut_Spec, 20.0f);
	C3D_LightEnvLut(&lightEnv, GPU_LUT_D0, GPU_LUTINPUT_NH, false, &lut_Spec);

	C3D_LightInit(&light, &lightEnv);

	// Create text buffer
	staticTextBuf = C2D_TextBufNew(128);
	C2D_TextParse(&txt_helloWorld, staticTextBuf, "Hello, citro2d world!");
	C2D_TextOptimize(&txt_helloWorld);
}

static void sceneBind(void)
{
	C3D_BindProgram(&program);
	C3D_SetAttrInfo(&vbo_attrInfo);
	C3D_SetBufInfo(&vbo_bufInfo);
	C3D_LightEnvBind(&lightEnv);
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	C3D_CullFace(GPU_CULL_BACK_CCW);

	// Configure the first fragment shading substage to blend the fragment primary color
	// with the fragment secondary color.
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_ADD);

	// Clear out the other texenvs
	C3D_TexEnvInit(C3D_GetTexEnv(1));
	C3D_TexEnvInit(C3D_GetTexEnv(2));
	C3D_TexEnvInit(C3D_GetTexEnv(3));
	C3D_TexEnvInit(C3D_GetTexEnv(4));
	C3D_TexEnvInit(C3D_GetTexEnv(5));
}

static void sceneRender(float iod)
{
	// Bind the program to render the VBO scene
	sceneBind();

	// Compute the projection matrix
	Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, iod, 2.0f, false);

	C3D_FVec objPos   = FVec4_New(0.0f, 0.0f, -3.0f, 1.0f);
	C3D_FVec lightPos = FVec4_New(0.0f, 0.0f, -0.5f, 1.0f);

	// Calculate the modelView matrix
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_Translate(&modelView, objPos.x, objPos.y, objPos.z, true);
	Mtx_RotateY(&modelView, C3D_Angle(angleY), true);
	Mtx_Scale(&modelView, 2.0f, 2.0f, 2.0f);

	C3D_LightPosition(&light, &lightPos);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);

	// Draw the VBO
	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_list_count);

	// Draw the 2d scene
	C2D_Prepare();
	C2D_DrawText(&txt_helloWorld, 0, 8.0f, 8.0f, 1.0f, 1.0f, 1.0f);
	C2D_Flush();
}

static void sceneExit(void)
{
	// Free the VBO
	linearFree(vbo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

int main()
{
	// Initialize graphics
	gfxInitDefault();
	gfxSet3D(true); // Enable stereoscopic 3D
	consoleInit(GFX_BOTTOM, NULL);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);

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
		float iod = slider/3;

		// Rotate the model
		if (!(kHeld & KEY_A))
		{
			angleX += 1.0f/64;
			angleY += 1.0f/256;
		}

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{
			C3D_RenderTargetClear(targetLeft, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(targetLeft);
			C2D_SceneTarget(targetLeft);
			sceneRender(-iod);

			if (iod > 0.0f)
			{
				C3D_RenderTargetClear(targetRight, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
				C3D_FrameDrawOn(targetRight);
				C2D_SceneTarget(targetRight);
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
