#pragma once
#define TAU 6.283185307179586f
#include <3ds.h>
#include <citro3d.h>
#include "vector.h"
#include "matrix.h"

typedef struct
{
    float position[3];
    float texcoord[2];
    float normal[3];
} Vertex;

void KleinToMinkowski(Vertex *v, int length);

void MinkowskiToPoincare(Vertex *v, int length);

Matrix4x4 HyperTranslateX(float angle);

Matrix4x4 HyperTranslateY(float angle);

Matrix4x4 RotateZ(float angle);

void MoveDiscrete(Vertex *mesh, float x, float y, bool xFirst);

void Move(Vertex *mesh, float x, float y);