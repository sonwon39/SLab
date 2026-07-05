#pragma once

#include "Mesh.h"
#include "Vertex.h"

struct GeometryGenerator
{
    static Mesh<SimpleVertex, uint16_t> MakeSimpleRect(float x, float y);
    static Mesh<SimpleVertex, uint16_t> MakeSimpleCube(float x, float y, float z);

    static Mesh<Vertex, uint16_t> MakeCube(float x, float y, float z);

    static Mesh<Vertex, uint16_t> MakePlane(float x, float z, int c);

    static Mesh<Vertex, uint16_t> MakeRect(float x, float y);

    static Mesh<Vertex, uint16_t> MakeSphere(int c, float r);

    static Mesh<PBRVertex, uint16_t> PBRPlane(float halfX, float halfZ, int cX, int cY);
    static Mesh<PBRVertex, uint16_t> PbrSphere(const float& radius, const int& x, const int& y);
    static Mesh<PBRVertex, uint16_t> PBRCube(float x, float y, float z, int xCount, int yCount, int zCount);

    static void ComputeTangent(PBRVertex& v0, PBRVertex& v1, PBRVertex& v2);

    static Mesh<PointCloudVertex, uint16_t> MakePointCube(float x, float y, float z);

    static Mesh<SimpleVertex, uint16_t> MakePoint();

    static Mesh<PositionVertex, uint16_t> MakePositionPoint();
};
