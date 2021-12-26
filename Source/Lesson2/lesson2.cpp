#include <iostream>
#include <vector>

#include "Utilities/geometry.h"
#include "Utilities/model.h"
#include "Utilities/tgaimage.h"

const TGAColor white  = TGAColor(255, 255, 255, 255);
const TGAColor red    = TGAColor(255, 0, 0, 255);
Model*         model  = NULL;
const int      width  = 1024;
const int      height = 1024;

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
    // i dont care about degenerate triangles
    if (t0.y == t1.y && t0.y == t2.y)
        return;

    // t0 is lowest, t2 is highest
    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);

    int total_height = t2.y - t0.y;

    for (int i = 0; i < total_height; i++)
    {
        bool isInSecondHalf = i > t1.y - t0.y || t1.y == t0.y;

        int   currentSegmentHeight = isInSecondHalf ? t2.y - t1.y : t1.y - t0.y;
        float percentScanned       = (float)i / total_height;

        // Be careful: with above conditions no division by zero here
        float percentSegmentScanned = (float)(i - (isInSecondHalf ? t1.y - t0.y : 0)) / currentSegmentHeight;

        Vec2i distAlongMajorSide = t0 + (t2 - t0) * percentScanned;
        Vec2i distAlongSegmentSide =
            isInSecondHalf ? t1 + (t2 - t1) * percentSegmentScanned : t0 + (t1 - t0) * percentSegmentScanned;

        // We always want distAlongSegmentSide.x to be higher
        if (distAlongMajorSide.x > distAlongSegmentSide.x)
        {
            std::swap(distAlongMajorSide, distAlongSegmentSide);
        }

        for (int j = distAlongMajorSide.x; j <= distAlongSegmentSide.x; j++)
        {
            // attention, due to int casts t0.y+i != A.y
            image.set(j, t0.y + i, color);
        }
    }
}

void RenderModel(const std::string& path, const std::string& ouputName)
{
    model = new Model(path.c_str());

    float halfWidth  = width / 2.0f;
    float halfHeight = height / 2.0f;

    TGAImage image(width, height, TGAImage::RGB);
    Vec3f    lightDirection(0, 0, -1);

    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        Vec2i            screenCoords[3];
        Vec3f            worldCoords[3];
        for (int j = 0; j < 3; j++)
        {
            Vec3f vertex    = model->vert(face[j]);
            screenCoords[j] = Vec2i((vertex.x + 1.) * halfWidth, (vertex.y + 1.) * halfHeight);
            worldCoords[j]  = vertex;
        }
        Vec3f normal = (worldCoords[2] - worldCoords[0]) ^ (worldCoords[1] - worldCoords[0]);
        normal.normalize();

        float lightIntensity = normal * lightDirection;
        if (lightIntensity > 0)
        {
            triangle(screenCoords[0], screenCoords[1], screenCoords[2], image,
                     TGAColor(lightIntensity * 255, lightIntensity * 255, lightIntensity * 255, 255));
        }
    }

    std::string outputPath = "../Renders/Lesson2/" + ouputName + ".tga";

    image.write_tga_file(outputPath);

    delete model;
}

int main()
{
    RenderModel("../Assets/obj/african_head/african_head.obj", "Head");
    RenderModel("../Assets/obj/diablo3_pose/diablo3_pose.obj", "Diablo");
    return 0;
}