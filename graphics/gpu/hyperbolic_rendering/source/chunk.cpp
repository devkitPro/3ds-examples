#include "chunk.h"

Chunk::Chunk()
{
    planes.clear();
    
    SubdividedPlane *plane = new SubdividedPlane;
	SubdividedPlane *plane_left = new SubdividedPlane;
	SubdividedPlane *plane_down = new SubdividedPlane;
	SubdividedPlane *plane_right = new SubdividedPlane;
	SubdividedPlane *plane_up = new SubdividedPlane;
	SubdividedPlane *plane_upleft = new SubdividedPlane;
	SubdividedPlane *plane_leftup = new SubdividedPlane;
	SubdividedPlane *plane_upright = new SubdividedPlane;
	SubdividedPlane *plane_rightup = new SubdividedPlane;
	SubdividedPlane *plane_leftdown = new SubdividedPlane;
	SubdividedPlane *plane_downleft = new SubdividedPlane;
	SubdividedPlane *plane_rightdown = new SubdividedPlane;
	SubdividedPlane *plane_downright = new SubdividedPlane;

	plane->ApplyTexture(NULL, center_t3x, center_t3x_size, plane->env);
    planes.push_back(plane);

	plane_up->ApplyTexture(NULL, up_t3x, up_t3x_size, plane_up->env);
	MoveDiscrete(plane_up->vertex_list, -FUNDAMENTAL_REGION_OFFSET, 0.0, true);
    planes.push_back(plane_up);

	plane_left->ApplyTexture(NULL, left_t3x, left_t3x_size, plane_left->env);
	MoveDiscrete(plane_left->vertex_list, 0.0, -FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_left);

	plane_right->ApplyTexture(NULL, right_t3x, right_t3x_size, plane_right->env);
	MoveDiscrete(plane_right->vertex_list, 0.0, FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_right);

	plane_down->ApplyTexture(NULL, down_t3x, down_t3x_size, plane_down->env);
	MoveDiscrete(plane_down->vertex_list, FUNDAMENTAL_REGION_OFFSET, 0.0,true);
    planes.push_back(plane_down);

	plane_upleft->ApplyTexture(NULL, upleft_t3x, upleft_t3x_size, plane_upleft->env);
	MoveDiscrete(plane_upleft->vertex_list, -FUNDAMENTAL_REGION_OFFSET, -FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_upleft);

	plane_leftup->ApplyTexture(NULL, leftup_t3x, leftup_t3x_size, plane_leftup->env);
	MoveDiscrete(plane_leftup->vertex_list, -FUNDAMENTAL_REGION_OFFSET, -FUNDAMENTAL_REGION_OFFSET, false);
    planes.push_back(plane_leftup);

	plane_upright->ApplyTexture(NULL, upright_t3x, upright_t3x_size, plane_upright->env);
	MoveDiscrete(plane_upright->vertex_list, -FUNDAMENTAL_REGION_OFFSET, FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_upright);

	plane_rightup->ApplyTexture(NULL, rightup_t3x, rightup_t3x_size, plane_rightup->env);
	MoveDiscrete(plane_rightup->vertex_list, -FUNDAMENTAL_REGION_OFFSET, FUNDAMENTAL_REGION_OFFSET, false);
    planes.push_back(plane_rightup);

	plane_leftdown->ApplyTexture(NULL, leftdown_t3x, leftdown_t3x_size, plane_leftdown->env);
	MoveDiscrete(plane_leftdown->vertex_list, FUNDAMENTAL_REGION_OFFSET, -FUNDAMENTAL_REGION_OFFSET, false);
    planes.push_back(plane_leftdown);

	plane_downleft->ApplyTexture(NULL, downleft_t3x, downleft_t3x_size, plane_downleft->env);
	MoveDiscrete(plane_downleft->vertex_list, FUNDAMENTAL_REGION_OFFSET, -FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_downleft);

	plane_rightdown->ApplyTexture(NULL, rightdown_t3x, rightdown_t3x_size, plane_rightdown->env);
	MoveDiscrete(plane_rightdown->vertex_list, FUNDAMENTAL_REGION_OFFSET, FUNDAMENTAL_REGION_OFFSET, false);
    planes.push_back(plane_rightdown);

	plane_downright->ApplyTexture(NULL, downright_t3x, downright_t3x_size, plane_downright->env);
	MoveDiscrete(plane_downright->vertex_list, FUNDAMENTAL_REGION_OFFSET, FUNDAMENTAL_REGION_OFFSET, true);
    planes.push_back(plane_downright);
}

Chunk::~Chunk()
{
    for(int i = 0; i < (int)planes.size(); i++)
    {
        delete planes[i];
    }
    planes.clear();
}

void Chunk::UpdateVertices()
{
    for (int i = 0; i < (int)planes.size(); i++)
    {
        planes[i]->UpdateVertices();
    }
}

void Chunk::Draw()
{
    for (int i = 0; i < (int)planes.size(); i++)
    {
        planes[i]->Draw();
    }
}

void Chunk::FullUpdate(C3D_Mtx modelView)
{
    for (int i = 0; i < (int)planes.size(); i++)
    {
        planes[i]->UpdateVertices();
		planes[i]->GetSceneInfo(modelView);
		planes[i]->Draw();
    }
}

void Chunk::MoveAll(float x, float y)
{
    for (int i = 0; i < (int)planes.size(); i++)
    {
        Move(planes[i]->vertex_list, x, y);
    }
}