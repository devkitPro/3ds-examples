#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "program_shbin.h"
#include "texture_bin.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float position[3]; float texcoord[2]; u8 valence[4]; } vertex;
#define FACE(x,y,z,u,v,_val) \
	{ {(x), (y), (z)}, {(u), (v)}, {(_val),0,0,0} }

#include "model.obj.h"
#define vertex_list_count (sizeof(vertex_list)/sizeof(vertex_list[0]))
#define index_list_count  (sizeof(index_list) /sizeof(index_list[0]))

static DVLB_s* program_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_passes;
static C3D_Mtx projection;
static C3D_Tex texture;

static void* vbo_data;
static void* index_data;
static float angleX = 0.0, angleY = 0.0;
static int sub_level = 1;

static void sceneInit(void)
{
	// Load the shaders and create a shader program
	program_dvlb = DVLB_ParseFile((u32*)program_shbin, program_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &program_dvlb->DVLE[0]);
	shaderProgramSetGsh(&program, &program_dvlb->DVLE[1], 0);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader,   "projection");
	uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader,   "modelView");
	uLoc_passes     = shaderInstanceGetUniformLocation(program.geometryShader, "passes");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT,         3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT,         2); // v1=texcoord
	AttrInfo_AddLoader(attrInfo, 2, GPU_UNSIGNED_BYTE, 4); // v2=valence

	// Compute the projection matrix
	Mtx_PerspTilt(&projection, 80.0f*M_PI/180.0f, 400.0f/240.0f, 0.01f, 1000.0f, false);

	// Create the VBO (vertex buffer object)
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));
	index_data = linearAlloc(sizeof(index_list));
	memcpy(index_data, index_list, sizeof(index_list));

	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 3, 0x210);

	// Configure texture
	// Load the texture and bind it to the first texture unit
	C3D_TexInit(&texture, 32, 32, GPU_RGBA8);
	C3D_TexUpload(&texture, texture_bin);
	C3D_TexSetFilter(&texture, GPU_LINEAR, GPU_NEAREST);
	C3D_TexSetWrap(&texture, GPU_REPEAT, GPU_REPEAT);
	C3D_TexBind(0, &texture);

	// Configure the first fragment shading substage to just pass through the vertex color
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
	Mtx_Translate(&modelView, 0.0, -1.25, -2.5 - angleX, true);
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Rotate the object each frame
	angleY += M_TAU / 360;
	angleX = (M_TAU / 8) * sinf(angleY);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);
	C3D_FVUnifSet (GPU_GEOMETRY_SHADER, uLoc_passes,     sub_level, 0, 0, 0);

	// Draw the VBO
	C3D_DrawElements(GPU_GEOMETRY_PRIM, index_list_count, C3D_UNSIGNED_SHORT, index_data);
}

static void sceneExit(void)
{
	// Free the texture
	C3D_TexDelete(&texture);

	// Free the VBO
	linearFree(vbo_data);
	linearFree(index_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(program_dvlb);
}

int main()
{
	// Initialize graphics
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
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kDown & KEY_UP)
			sub_level++;
		else if (kDown & KEY_DOWN)
			sub_level--;
		if (sub_level < 0)
			sub_level = 0;
		else if (sub_level > 2)
			sub_level = 2;

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
