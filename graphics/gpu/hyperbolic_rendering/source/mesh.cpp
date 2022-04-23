#include "mesh.h"

SubdividedPlane::SubdividedPlane()
{
    this->LoadVertex();
    KleinToMinkowski(this->vertex_list, 24);
    this->LoadShaders();
    this->GetUniformLocations();
    this->GenerateAndBindBuffers();
}

SubdividedPlane::~SubdividedPlane()
{
    C3D_TexDelete(&this->mTex);
    free(vbo_data);
    free(env);
    free(vshader_dvlb);
}

bool SubdividedPlane::LoadTextureFromMem(C3D_TexCube *cube, const void *data, size_t size)
{
	Tex3DS_Texture t3x = Tex3DS_TextureImport(data, size, &this->mTex, cube, false);
	if (!t3x)
		return false;

	Tex3DS_TextureFree(t3x);
	return true;
}

void SubdividedPlane::ApplyTexture(C3D_TexCube *cube, const void *data, size_t size, C3D_TexEnv *env)
{
    if (!LoadTextureFromMem(NULL, data, size))
		svcBreak(USERBREAK_PANIC);
	C3D_TexSetFilter(&this->mTex, GPU_LINEAR, GPU_NEAREST);
	C3D_TexBind(0, &this->mTex);

	this->env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(this->env);
	C3D_TexEnvSrc(this->env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_FRAGMENT_PRIMARY_COLOR);
	C3D_TexEnvFunc(this->env, C3D_Both, GPU_MODULATE);
}

void SubdividedPlane::LoadShaders()
{
    this->vshader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&this->s_program);
    shaderProgramSetVsh(&this->s_program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&this->s_program);
}

void SubdividedPlane::GetUniformLocations()
{
    this->uLoc_projection = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "projection");
    this->uLoc_modelView = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "modelView");
    this->uLoc_lightVec = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "lightVec");
    this->uLoc_lightHalfVec = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "lightHalfVec");
    this->uLoc_lightClr = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "lightClr");
    this->uLoc_material = shaderInstanceGetUniformLocation(this->s_program.vertexShader, "material");
}

void SubdividedPlane::UpdateVertices()
{
    Vertex poinc[24];
    memcpy(poinc, this->vertex_list, sizeof(this->vertex_list));
    MinkowskiToPoincare(poinc, 24);
    memcpy(this->vbo_data, poinc, sizeof(poinc));
    C3D_BufInfo *bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, this->vbo_data, sizeof(Vertex), 3, 0x210);
}

void SubdividedPlane::GenerateAndBindBuffers()
{
    C3D_AttrInfo *attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3);

    Mtx_PerspTilt(&this->projection, C3D_AngleFromDegrees(80.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);

    this->vbo_data = linearAlloc(sizeof(this->vertex_list));
    memcpy(this->vbo_data, this->vertex_list, sizeof(this->vertex_list));

    C3D_BufInfo *bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, this->vbo_data, sizeof(Vertex), 3, 0x210);
}

void SubdividedPlane::Draw()
{
    C3D_TexBind(0, &this->mTex);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 24);
}

Vertex vertexData[24] = 
{
    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.487000, 0.5, 1.0, 0.000000, 1.000000, -0.000000, },
    {  0.487000, 0.000000, 0.000000, 1.0, 0.5, 0.000000, 1.000000, -0.000000, },

    {  -0.487000, 0.000000, 0.000000, 0.0, 0.5, 0.000000, 1.000000, -0.000000, },
    {  -0.487000, 0.000000, 0.487000, 0.0, 1.0, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },

    {  -0.487000, 0.000000, -0.487000, 0.0, 0.0, 0.000000, 1.000000, -0.000000, },
    {  -0.487000, 0.000000, 0.000000, 0.0, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, -0.487000, 0.5, 0.0, 0.000000, 1.000000, -0.000000, },

    {  0.000000, 0.000000, -0.487000, 0.5, 0.0, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.487000, 0.000000, -0.487000, 1.0, 0.0, 0.000000, 1.000000, -0.000000, },

    {  0.487000, 0.000000, 0.000000, 1.0, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.487000, 0.5, 1.0, 0.000000, 1.000000, -0.000000, },
    {  0.487000, 0.000000, 0.487000, 1.0, 1.0, 0.000000, 1.000000, -0.000000, },

    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },
    {  -0.487000, 0.000000, 0.487000, 0.0, 1.0, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.487000, 0.5, 1.0, 0.000000, 1.000000, -0.000000, },

    {  0.000000, 0.000000, -0.487000, 0.5, 0.0, 0.000000, 1.000000, -0.000000, },
    {  -0.487000, 0.000000, 0.000000, 0.0, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },

    {  0.487000, 0.000000, -0.487000, 1.0, 0.0, 0.000000, 1.000000, -0.000000, },
    {  0.000000, 0.000000, 0.000000, 0.5, 0.5, 0.000000, 1.000000, -0.000000, },
    {  0.487000, 0.000000, 0.000000, 1.0, 0.5, 0.000000, 1.000000, -0.000000, },
};

void SubdividedPlane::LoadVertex()
{
    memcpy(this->vertex_list, vertexData, sizeof(vertexData));
}

void SubdividedPlane::GetSceneInfo(C3D_Mtx modelView)
{
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_projection, &this->projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_modelView, &modelView);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, this->uLoc_material, &this->material);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightVec, 0.0f, -1.0f, 0.0f, 10.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightHalfVec, 0.0f, 0.0f, -1.0f, 0.0f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, this->uLoc_lightClr, 1.0f, 1.0f, 1.0f, 1.0f);
}