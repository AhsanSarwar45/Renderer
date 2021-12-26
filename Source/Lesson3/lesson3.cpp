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

Vec3f CalcBarycentricCoords(const Vec3f& a, const Vec3f& b, const Vec3f& c, const Vec3f& point)
{
    Vec3f s[2];
    for (int i = 2; i--;)
    {
        s[i][0] = c[i] - a[i];
        s[i][1] = b[i] - a[i];
        s[i][2] = a[i] - point[i];
    }
    Vec3f u = cross(s[0], s[1]);

    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
    {
        return Vec3f(1.0f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    }

    return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

void DrawTriangle(const Vec3f* const vertices, float* zbuffer, TGAImage& image, TGAColor color)
{
    Vec2f bboxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxMin[j] = std::max(0.0f, std::min(bboxMin[j], vertices[i][j]));
            bboxMax[j] = std::min(clamp[j], std::max(bboxMax[j], vertices[i][j]));
        }
    }

    Vec3f point;
    for (point.x = bboxMin.x; point.x <= bboxMax.x; point.x++)
    {
        for (point.y = bboxMin.y; point.y <= bboxMax.y; point.y++)
        {
            Vec3f barycentricCoords = CalcBarycentricCoords(vertices[0], vertices[1], vertices[2], point);
            if (barycentricCoords.x < 0 || barycentricCoords.y < 0 || barycentricCoords.z < 0)
            {
                continue;
            }

            point.z = 0;

            for (int i = 0; i < 3; i++)
            {
                point.z += vertices[i][2] * barycentricCoords[i];
            }
            if (zbuffer[int(point.x + point.y * width)] < point.z)
            {
                zbuffer[int(point.x + point.y * width)] = point.z;
                image.set(point.x, point.y, color);
            }
        }
    }
}

void DrawLine(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    { // if the line is steep, we transpose the image
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1)
    { // make it left−to−right
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx      = x1 - x0;
    int dy      = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2  = 0;
    int y       = y0;
    for (int x = x0; x <= x1; x++)
    {
        if (steep)
        {
            image.set(y, x, color);
        }
        else
        {
            image.set(x, y, color);
        }
        error2 += derror2;
        if (error2 > dx)
        {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

Vec3f WorldToScreen(const Vec3f& vec)
{
    return Vec3f(int((vec.x + 1.0f) * width / 2.0f + 0.5f), int((vec.y + 1.0f) * height / 2.0f + 0.5f), vec.z);
}

void RenderModel(const std::string& path, const std::string& ouputName)
{
    model = new Model(path.c_str());

    float halfWidth  = width / 2.0f;
    float halfHeight = height / 2.0f;

    float* zBuffer = new float[width * height];

    for (int i = (width * height) - 1; i >= 0; i--)
    {
        zBuffer[i] = -std::numeric_limits<float>::max();
    }

    TGAImage wireframeImage(width, height, TGAImage::RGB);
    TGAImage renderImage(width, height, TGAImage::RGB);

    Vec3f lightDirection(0, 0, -1);

    for (int i = 0; i < model->GetNumFaces(); i++)
    {
        std::vector<int> face = model->GetFaceAtIndex(i);
        Vec3f            vertices[3];
        Vec3f            screenCoords[3];
        for (int i = 0; i < 3; i++)
        {
            vertices[i]     = model->GetVertexAtIndex(face[i]);
            screenCoords[i] = WorldToScreen(vertices[i]);

            Vec3f v0 = vertices[i];
            Vec3f v1 = model->GetVertexAtIndex(face[(i + 1) % 3]);
            int   x0 = (v0.x + 1.) * halfWidth;
            int   y0 = (v0.y + 1.) * halfHeight;
            int   x1 = (v1.x + 1.) * halfWidth;
            int   y1 = (v1.y + 1.) * halfHeight;
            DrawLine(x0, y0, x1, y1, wireframeImage, white * ((v0.z + 1) / 2));
        }

        Vec3f normal = (vertices[2] - vertices[0]) ^ (vertices[1] - vertices[0]);
        normal.normalize();

        float lightIntensity = normal * lightDirection;
        if (lightIntensity > 0)
        {

            DrawTriangle(screenCoords, zBuffer, renderImage,
                         TGAColor(lightIntensity * 255, lightIntensity * 255, lightIntensity * 255, 255));
        }
    }

    std::string wireframeOutputPath = "../Renders/Lesson3/" + ouputName + "Wireframe.tga";
    std::string renderOutputPath    = "../Renders/Lesson3/" + ouputName + "Render.tga";

    wireframeImage.write_tga_file(wireframeOutputPath);
    renderImage.write_tga_file(renderOutputPath);

    delete model;
}

int main()
{
    RenderModel("../Assets/obj/african_head/african_head.obj", "Head");
    RenderModel("../Assets/obj/diablo3_pose/diablo3_pose.obj", "Diablo");
    return 0;
}