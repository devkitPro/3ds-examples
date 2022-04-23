#pragma once
#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include "vshader_shbin.h"
#include "hyperMath.h"

class SubdividedPlane
{
    public:
        SubdividedPlane();
        ~SubdividedPlane();

        Vertex vertex_list[24];
        DVLB_s *vshader_dvlb;

        shaderProgram_s s_program;
        void *vbo_data;
        C3D_Mtx projection;
        C3D_Mtx material =
        {
            {
                {{0.0f, 0.2f, 0.2f, 0.2f}}, // Ambient
                {{0.0f, 0.9f, 0.9f, 0.9f}}, // Diffuse
                {{0.0f, 0.8f, 0.8f, 0.8f}}, // Specular
                {{1.0f, 0.0f, 0.0f, 0.0f}}, // Emission
            }
        };
        C3D_Tex mTex;
        C3D_TexEnv *env;

        int uLoc_projection, uLoc_modelView;
        int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;

        void LoadShaders();
        void GetUniformLocations();
        void GenerateAndBindBuffers();
        void GetSceneInfo(C3D_Mtx modelView);
        void LoadVertex();
        void UpdateVertices();
        void Draw();
        void PrintVertex(Vertex *v, int length);
        bool LoadTextureFromMem(C3D_TexCube *cube, const void *data, size_t size);
        void ApplyTexture(C3D_TexCube *cube, const void *data, size_t size, C3D_TexEnv *env);
};
