#ifndef _VECTORF3_
#define _VECTORF3_

struct Vector3
{
    public:
        Vector3() = default;

        float x, y, z;
    
        Vector3(float a, float b, float c)
        {
            x = a;
            y = b;
            z = c;
        }
};

Vector3 operator-(Vector3 a, Vector3 b);

#endif