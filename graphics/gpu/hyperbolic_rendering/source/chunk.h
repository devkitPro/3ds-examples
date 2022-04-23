#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "mesh.h"
#include <vector>
#include <string>

#include "center_t3x.h"
#include "up_t3x.h"
#include "upleft_t3x.h"
#include "upright_t3x.h"
#include "down_t3x.h"
#include "downleft_t3x.h"
#include "downright_t3x.h"
#include "left_t3x.h"
#include "leftup_t3x.h"
#include "leftdown_t3x.h"
#include "right_t3x.h"
#include "rightup_t3x.h"
#include "rightdown_t3x.h"

#define FUNDAMENTAL_REGION_OFFSET 1.062054

class Chunk
{
    public:
        Chunk();
        ~Chunk();

        float x, y;
        std::vector<SubdividedPlane *> planes;
        void UpdateVertices();
        void Draw();
        void MoveAll(float x, float y);
        void FullUpdate(C3D_Mtx modelView);
};
#endif