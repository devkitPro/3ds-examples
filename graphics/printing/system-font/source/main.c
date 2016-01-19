#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
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
static C3D_Tex* glyphSheets;

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
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

	// Compute the projection matrix
	Mtx_OrthoTilt(&projection, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0);

	// Configure depth test to overwrite pixels with the same depth (needed to draw overlapping glyphs)
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

	// Load the glyph texture sheets
	int i;
	TGLP_s* glyphInfo = fontGetGlyphInfo();
	glyphSheets = malloc(sizeof(C3D_Tex)*glyphInfo->nSheets);
	for (i = 0; i < glyphInfo->nSheets; i ++)
	{
		C3D_Tex* tex = &glyphSheets[i];
		tex->data = fontGetGlyphSheetTex(i);
		tex->fmt = glyphInfo->sheetFmt;
		tex->size = glyphInfo->sheetSize;
		tex->width = glyphInfo->sheetWidth;
		tex->height = glyphInfo->sheetHeight;
		tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
			| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
	}
}

static void setTextColor(u32 color)
{
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(env, color);
}

static void renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text)
{
	ssize_t  units;
	uint32_t code;

	const uint8_t* p = (const uint8_t*)text;
	float firstX = x;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			x = firstX;
			y += scaleY*fontGetInfo()->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);

			C3D_TexBind(0, &glyphSheets[data.sheetIndex]);

			// Draw the glyph directly
			C3D_ImmDrawBegin(GPU_TRIANGLES);

			C3D_ImmSendAttrib(x+data.vtxcoord.left, y+data.vtxcoord.top, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.left,   data.texcoord.top, 0.0, 0.0);

			C3D_ImmSendAttrib(x+data.vtxcoord.left, y+data.vtxcoord.bottom, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.left,   data.texcoord.bottom, 0.0, 0.0);

			C3D_ImmSendAttrib(x+data.vtxcoord.right, y+data.vtxcoord.bottom, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.right,   data.texcoord.bottom, 0.0, 0.0);

			C3D_ImmSendAttrib(x+data.vtxcoord.left, y+data.vtxcoord.top, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.left,   data.texcoord.top, 0.0, 0.0);

			C3D_ImmSendAttrib(x+data.vtxcoord.right, y+data.vtxcoord.bottom, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.right,   data.texcoord.bottom, 0.0, 0.0);

			C3D_ImmSendAttrib(x+data.vtxcoord.right, y+data.vtxcoord.top, 0.5f, 0.0f);
			C3D_ImmSendAttrib(  data.texcoord.right,   data.texcoord.top, 0.0, 0.0);

			C3D_ImmDrawEnd();

			x += data.xAdvance;

		}
	} while (code > 0);
}

static void sceneRender(float size)
{
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

	const char* teststring =
		"Hello World - working text rendering!\n"
		"The quick brown fox jumped over the lazy dog.\n"
		"\n"
		"En français: Vous ne devez pas éteindre votre console.\n"
		"日本語も見せるのが出来ますよ。\n"
		"Un poco de texto en español nunca queda mal.\n"
		"Some more random text in English.\n"
		"В чащах юга жил бы цитрус? Да, но фальшивый экземпляр!\n"
		;

	setTextColor(0xFF000000); // black
	renderText(8.0f, 8.0f, size, size, false, teststring);

	setTextColor(0xFF0000FF); // red
	renderText(16.0f, 210.0f, 0.5f, 0.75f, true, "I am red skinny text!");
	setTextColor(0xA0FF0000); // semi-transparent blue
	renderText(150.0f, 210.0f, 0.75f, 0.5f, true, "I am blue fat text!");

	char buf[160];
	sprintf(buf, "Current text size: %f (Use  to change)", size);
	setTextColor(0xFF000000); // black
	renderText(8.0f, 220.0f, 0.5f, 0.5f, false, buf);
}

static void sceneExit(void)
{
	// Free the textures
	free(glyphSheets);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

int main()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	consoleInit(GFX_BOTTOM, NULL);

	// Initialize the render target
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	printf("System font example\n");
	Result res = fontEnsureMapped();

	if (R_FAILED(res))
		printf("fontEnsureMapped: %08lX\n", res);

	// Initialize the scene
	sceneInit();

	float size = 0.5f;

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if (kHeld & KEY_L)
			size -= 0.01f;
		else if (kHeld & KEY_R)
			size += 0.01f;
		else if (kHeld & KEY_X)
			size = 0.5f;
		else if (kHeld & KEY_Y)
			size = 1.0f;

		if (size < 0.25f)
			size = 0.25f;
		else if (size > 2.0f)
			size = 2.0f;

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_FrameDrawOn(target);
			sceneRender(size);
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();
	C3D_Fini();
	gfxExit();
	return 0;
}
