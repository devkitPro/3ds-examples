#include "hyperMath.h"

void KleinToMinkowski(Vertex *v, int length)
{
    for (int i = 0; i < length; i++)
    {
        float factor = sqrtf(1.0 / (1.0 - ((v[i].position[0] * v[i].position[0]) + (v[i].position[2] * v[i].position[2]))));
        float x1 = v[i].position[0] * factor;
        float z1 = v[i].position[2] * factor;
        v[i].position[0] = x1;
        v[i].position[1] = factor;
        v[i].position[2] = z1;
    }
}

void MinkowskiToPoincare(Vertex *v, int length)
{
    for (int i = 0; i < length; i++)
    {
        float vy = v[i].position[1] + 1.0;
        float factor = -1.0 / vy;
        float px = factor * v[i].position[0];
        float pz = factor * v[i].position[2];
        v[i].position[0] = px;
        v[i].position[1] = 0.0;
        v[i].position[2] = pz;
    }
}

Matrix4x4 HyperTranslateX(float angle)
{
    Matrix4x4 m;
    m = m.Identity();
    m.m11 = coshf(angle);
    m.m21 = sinhf(angle);
    m.m12 = sinhf(angle);
    m.m22 = coshf(angle);
    return m;
}

Matrix4x4 HyperTranslateY(float angle)
{
    Matrix4x4 m;
    m = m.Identity();
    m.m00 = coshf(angle);
    m.m02 = sinhf(angle);
    m.m20 = sinhf(angle);
    m.m22 = coshf(angle);
    return m;
}

Matrix4x4 RotateZ(float angle)
{
    Matrix4x4 m;
    m = m.Identity();
    m.m00 = cosf(angle);
    m.m10 = sinf(angle);
    m.m01 = -sinf(angle);
    m.m11 = cosf(angle);
    return m;
}

void MoveDiscrete(Vertex *mesh, float x, float y, bool xFirst)
{
    for (int i = 0; i < 24; i++)
    {
        Matrix4x4 result;
        if(x != 0.0 || y !=0.0){
            if (xFirst)
                result = HyperTranslateX(x) * HyperTranslateY(y);
            else
                result = HyperTranslateY(y) * HyperTranslateX(x);
            Vector3 currentPos = Vector3(mesh[i].position[0], mesh[i].position[2], mesh[i].position[1]);
            Vector3 resultPos = result * currentPos;
            mesh[i].position[0] = resultPos.x;
            mesh[i].position[1] = resultPos.z;
            mesh[i].position[2] = resultPos.y;
        }
    }
}

void Move(Vertex *mesh, float x, float y)
{
    for (int i = 0; i < 24; i++)
    {
        Matrix4x4 result;
        if (x != 0.0 || y !=0.0)
        {
            result = RotateZ(x * 2) * HyperTranslateY(y);
            Vector3 currentPos = Vector3(mesh[i].position[0], mesh[i].position[2], mesh[i].position[1]);
            Vector3 resultPos = result * currentPos;
            mesh[i].position[0] = resultPos.x;
            mesh[i].position[1] = resultPos.z;
            mesh[i].position[2] = resultPos.y;
        }
    }
}