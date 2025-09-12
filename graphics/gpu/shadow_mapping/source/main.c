// Shadow mapping example
// based on the following guide:
// https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping

#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "shadow_caster_shbin.h"
#include "shadow_receiver_shbin.h"
#include "teapot.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
    GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

const float plane_vertices[] =
{
    -2,  0, -2,  0, 1, 0,
     2,  0, -2,  0, 1, 0,
    -2,  0,  2,  0, 1, 0,
     2,  0,  2,  0, 1, 0
};

#define plane_index_count 6
const u16 plane_indices[] =
{
    2, 1, 0,
    1, 2, 3
};

// Shadow map
static C3D_RenderTarget *shadow_map_rt;
static C3D_Tex shadow_map_tex;
static int cull_back_faces, filter;
// Adjust until shadow acne is gone
static float bias;

// Shaders
static DVLB_s* shadow_caster_dvlb;
static shaderProgram_s shadow_caster_program;
static int shadow_caster_uLoc_model, shadow_caster_uLoc_viewproj;

static DVLB_s* shadow_receiver_dvlb;
static shaderProgram_s shadow_receiver_program;
static int shadow_receiver_uLoc_model, shadow_receiver_uLoc_view, shadow_receiver_uLoc_proj, shadow_receiver_uLoc_light_viewproj;

static C3D_LightEnv lightEnv;
static C3D_Light light;
static C3D_LightLut lut_diffuse;
static C3D_Mtx light_view;
static C3D_Mtx light_proj;

static C3D_BufInfo plane_vbo_buf_info;
static void* plane_vbo_data;
static void* plane_ibo_data;

static C3D_BufInfo teapot_vbo_buf_info;
static void* teapot_vbo_data;
static void* teapot_ibo_data;

static float elapsed;

static float diffuse(float x, float arg)
{
    return x;
}

static void drawShadowMap()
{
    C3D_BindProgram(&shadow_caster_program);

    if(cull_back_faces)
        C3D_CullFace(GPU_CULL_BACK_CCW);
    else
        C3D_CullFace(GPU_CULL_FRONT_CCW);

    // Disable alpha blending 
    C3D_ColorLogicOp(GPU_LOGICOP_COPY);

    // Only the alpha value is compared against the fragment depth
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvColor(env, 0xFFFFFFFF);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);

    // Keep it tight
    Mtx_Ortho(&light_proj, -2.0f, 2.0f, -2.0f, 2.0f, 5.0f, 10.0f, false);

    C3D_FVec light_pos = FVec3_New(5, 5, 5);
    C3D_FVec light_tar = FVec3_New(0, 0, 0);
    C3D_FVec light_upv = FVec3_New(0, 1, 0);
    Mtx_LookAt(&light_view, light_pos, light_tar, light_upv, false);

    C3D_Mtx viewproj;
    Mtx_Multiply(&viewproj, &light_proj, &light_view);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_caster_uLoc_viewproj, &viewproj);

    C3D_Mtx model;
    Mtx_Identity(&model);
    Mtx_RotateY(&model, elapsed, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_caster_uLoc_model,  &model);

    // Draw the teapot
    C3D_SetBufInfo(&teapot_vbo_buf_info);
    C3D_DrawElements(GPU_TRIANGLES, vertex_element_count, C3D_UNSIGNED_SHORT, teapot_ibo_data);
}

static void sceneInit(void)
{
    cull_back_faces = true;
    filter = GPU_NEAREST;
    bias = 0.004f;
    elapsed = 0.0f;
    
    // Create the render target and texture
    C3D_TexInitShadow(&shadow_map_tex, 512, 512);
    shadow_map_rt = C3D_RenderTargetCreateFromTex(&shadow_map_tex, GPU_TEXFACE_2D, 0, GPU_RB_DEPTH16);
    // Anything outside the shadow map will be lit
    shadow_map_tex.border = 0xFFFFFFFF;
    C3D_TexSetWrap(&shadow_map_tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);


    // Load the shadow_caster shader
    shadow_caster_dvlb = DVLB_ParseFile((u32*)shadow_caster_shbin, shadow_caster_shbin_size);
    shaderProgramInit(&shadow_caster_program);
    shaderProgramSetVsh(&shadow_caster_program, &shadow_caster_dvlb->DVLE[0]);

    // Get the location of the shadow_caster uniforms
    shadow_caster_uLoc_viewproj = shaderInstanceGetUniformLocation(shadow_caster_program.vertexShader, "viewproj");
    shadow_caster_uLoc_model    = shaderInstanceGetUniformLocation(shadow_caster_program.vertexShader, "model");


    // Load the shadow_receiver shader
    shadow_receiver_dvlb = DVLB_ParseFile((u32*)shadow_receiver_shbin, shadow_receiver_shbin_size);
    shaderProgramInit(&shadow_receiver_program);
    shaderProgramSetVsh(&shadow_receiver_program, &shadow_receiver_dvlb->DVLE[0]);

    // Get the location of the shadow_receiver uniforms
    shadow_receiver_uLoc_proj  = shaderInstanceGetUniformLocation(shadow_receiver_program.vertexShader, "proj");
    shadow_receiver_uLoc_view  = shaderInstanceGetUniformLocation(shadow_receiver_program.vertexShader, "view");
    shadow_receiver_uLoc_model = shaderInstanceGetUniformLocation(shadow_receiver_program.vertexShader, "model");
    shadow_receiver_uLoc_light_viewproj = shaderInstanceGetUniformLocation(shadow_receiver_program.vertexShader, "light_viewproj");


    // Configure attributes for use with the vertex shaders
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 3); // v1=normal


    // Create the teapot VBO (vertex buffer object)
    teapot_vbo_data = linearAlloc(sizeof(vertex_array));
    memcpy(teapot_vbo_data, vertex_array, sizeof(vertex_array));
    teapot_ibo_data = linearAlloc(sizeof(vertex_elements));
    memcpy(teapot_ibo_data, vertex_elements, sizeof(vertex_elements));

    // Configure teapot buffer
    BufInfo_Init(&teapot_vbo_buf_info);
    BufInfo_Add(&teapot_vbo_buf_info, teapot_vbo_data, sizeof(float[6]), 2, 0x10);


    // Create the plane VBO
    plane_vbo_data = linearAlloc(sizeof(plane_vertices));
    memcpy(plane_vbo_data, plane_vertices, sizeof(plane_vertices));
    plane_ibo_data = linearAlloc(sizeof(plane_indices));
    memcpy(plane_ibo_data, plane_indices, sizeof(plane_indices));

    // Configure plane buffer
    BufInfo_Init(&plane_vbo_buf_info);
    BufInfo_Add(&plane_vbo_buf_info, plane_vbo_data, sizeof(float[6]), 2, 0x10);


    C3D_Material shadow_material =
    {
        { 0.5f, 0.5f, 0.5f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.5f, 0.5f, 0.5f },
        { 0.0f, 0.0f, 0.0f },
    };
    C3D_LightEnvInit(&lightEnv);
    C3D_LightEnvMaterial(&lightEnv, &shadow_material);

    LightLut_FromFunc(&lut_diffuse, diffuse, 0.0f, false);
    C3D_LightEnvLut(&lightEnv, GPU_LUT_D1, GPU_LUTINPUT_LN, false, &lut_diffuse);

    C3D_LightInit(&light, &lightEnv);
    C3D_FVec lightPos = FVec4_New(1.0f, 1.0f, 1.0f, 0.0f);
    C3D_LightPosition(&light, &lightPos);

    C3D_LightShadowEnable(&light, true);
    C3D_LightEnvShadowMode(&lightEnv, GPU_SHADOW_SECONDARY);
    // Shadow map must be on unit 0
    C3D_LightEnvShadowSel(&lightEnv, 0);
}

static void sceneRender(float iod)
{
    C3D_BindProgram(&shadow_receiver_program);

    C3D_CullFace(GPU_CULL_BACK_CCW);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, 0);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_ADD);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_REPLACE);

    C3D_Mtx proj;
    Mtx_PerspStereoTilt(&proj, C3D_AngleFromDegrees(20.0f), C3D_AspectRatioTop, 1.0f, 100.0f, iod, 8.0f, false);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_receiver_uLoc_proj,  &proj);

    C3D_Mtx view;
    C3D_FVec cam_pos = FVec3_New(0, 5, 5);
    C3D_FVec cam_tar = FVec3_New(0, 0, 0);
    C3D_FVec cam_upv = FVec3_New(0, 1, 0);
    Mtx_LookAt(&view, cam_pos, cam_tar, cam_upv, false);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_receiver_uLoc_view,  &view);

    C3D_Mtx light_viewproj;
    Mtx_Multiply(&light_viewproj, &light_proj, &light_view);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_receiver_uLoc_light_viewproj,  &light_viewproj);


    // false if light uses orthographic projection
    C3D_TexShadowParams(false, bias);
    C3D_TexBind(0, &shadow_map_tex);
    C3D_TexSetFilter(&shadow_map_tex, filter, filter);
    C3D_LightEnvBind(&lightEnv);


    C3D_Mtx model;
    Mtx_Identity(&model);
    Mtx_RotateY(&model, elapsed, true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_receiver_uLoc_model,  &model);

    // Draw the teapot
    C3D_SetBufInfo(&teapot_vbo_buf_info);
    C3D_DrawElements(GPU_TRIANGLES, vertex_element_count, C3D_UNSIGNED_SHORT, teapot_ibo_data);


    Mtx_Identity(&model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, shadow_receiver_uLoc_model,  &model);

    // Draw the plane
    C3D_SetBufInfo(&plane_vbo_buf_info);
    C3D_DrawElements(GPU_TRIANGLES, plane_index_count, C3D_UNSIGNED_SHORT, plane_ibo_data);
}

static void sceneExit(void)
{
    // Free the render target and texture
    C3D_RenderTargetDelete(shadow_map_rt);
    C3D_TexDelete(&shadow_map_tex);

    // Free the VBOs
    linearFree(teapot_vbo_data);
    linearFree(teapot_ibo_data);

    linearFree(plane_vbo_data);
    linearFree(plane_ibo_data);

    // Free the shader programs
    shaderProgramFree(&shadow_caster_program);
    DVLB_Free(shadow_caster_dvlb);
    shaderProgramFree(&shadow_receiver_program);
    DVLB_Free(shadow_receiver_dvlb);
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
        float iod = slider/2;

        // Rotate the model
        if (!(kHeld & KEY_A))
            elapsed += 1 / 60.0f;
        if (kDown & KEY_B)
            cull_back_faces = !cull_back_faces;
        if (kDown & KEY_Y)
            filter = !filter;
        if (kDown & KEY_UP)
            bias += 0.001;
        if (kDown & KEY_DOWN)
            bias -= 0.001;


        // Render the scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            C3D_RenderTargetClear(shadow_map_rt, C3D_CLEAR_ALL, 0xFFFFFFFF, 0);
            C3D_FrameDrawOn(shadow_map_rt);
            drawShadowMap();

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
