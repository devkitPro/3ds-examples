#ifndef _MATRIXF4_
#define _MATRIXF4_
#include "vector.h"

struct Matrix4x4
{
    public:
        Matrix4x4() = default;

        float m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33;

        Matrix4x4(float n00, float n01, float n02, float n03,
                  float n10, float n11, float n12, float n13,
                  float n20, float n21, float n22, float n23,
                  float n30, float n31, float n32, float n33)
        {
            m00 = n00; m01 = n01; m02 = n02; m03 = n03;
            m10 = n10; m11 = n11; m12 = n12; m13 = n13;
            m20 = n20; m21 = n21; m22 = n22; m23 = n23;
            m30 = n30; m31 = n31; m32 = n32; m33 = n33;
        }

        Matrix4x4 Identity()
        {
            return Matrix4x4(1.0, 0.0, 0.0, 0.0,
                             0.0, 1.0, 0.0, 0.0,
                             0.0, 0.0, 1.0, 0.0,
                             0.0, 0.0, 0.0, 1.0);
        };
};

Matrix4x4 operator*(const Matrix4x4 a, const Matrix4x4 b);

Vector3 operator*(Matrix4x4 a, Vector3 b);
#endif