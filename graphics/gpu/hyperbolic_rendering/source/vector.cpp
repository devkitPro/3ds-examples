#include "vector.h"

Vector3 operator-(Vector3 a, Vector3 b)
{
    return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}
