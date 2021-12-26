#include <algorithm>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>

#include "Utilities/model.h"
#include "Utilities/tgaimage.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"

const int WIDTH  = 1024;
const int HEIGHT = 1024;

const TGAColor WHITE = TGAColor(255, 255, 255);

const float HALF_WIDTH  = WIDTH / 2.0f;
const float HALF_HEIGHT = HEIGHT / 2.0f;

float EdgeFunctionCW(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}
float EdgeFunctionCCW(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    return (a.x - b.x) * (c.y - a.y) - (a.y - b.y) * (c.x - a.x);
}

TGAColor GetPixelColor(const Texture& texture, const glm::vec2& uv)
{
    int x = static_cast<int>(uv.x * texture.Width);
    int y = static_cast<int>(uv.y * texture.Height);

    unsigned char* pixelOffset = texture.Data + (x + texture.Width * y) * texture.NumComponents;

    return TGAColor(pixelOffset[0], pixelOffset[1], pixelOffset[2]);
}

struct Color
{
    int R;
    int G;
    int B;
};

void DrawTriangle(const glm::vec3 vertices[3], const glm::vec2 uvs[3], const glm::vec3 normals[3],
                  const Texture& texture, float* zbuffer, TGAImage& image, const glm::vec3& lightDirection)
{

    float area = EdgeFunctionCCW(vertices[0], vertices[1], vertices[2]);

    // An area of 0 means that the triangle is degenerate, so does not need to be rendered
    if (area == 0)
    {
        return;
    }

    glm::vec2 bboxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    glm::vec2 bboxMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    // clamping the bounding box of the triangle to the edges of the screen.
    glm::vec2 clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxMin[j] = std::max(0.0f, std::min(bboxMin[j], vertices[i][j]));
            bboxMax[j] = std::min(clamp[j], std::max(bboxMax[j], vertices[i][j]));
        }
    }

    glm::vec3 point;
    TGAColor  color;
    glm::vec2 texCoord;
    glm::vec3 normal;
    float     lightIntensity;

    // Loop throught all the pixels within the bounding box
    for (point.x = bboxMin.x; point.x <= bboxMax.x; point.x++)
    {
        for (point.y = bboxMin.y; point.y <= bboxMax.y; point.y++)
        {

            // for each weigth, we take the edge function of the edge opposite it
            float w0 = EdgeFunctionCCW(vertices[1], vertices[2], point);
            float w1 = EdgeFunctionCCW(vertices[2], vertices[0], point);
            float w2 = EdgeFunctionCCW(vertices[0], vertices[1], point);

            // if point p is inside triangles defined by vertices v0, v1, v2
            if (w0 <= 0 && w1 <= 0 && w2 <= 0)
            {
                // barycentric coordinates are the areas of the sub-triangles divided by the area of the main triangle
                w0 /= area;
                w1 /= area;
                w2 /= area;

                // interpolating using the barycentric coordinated to find the z value of the pixel
                point.z = vertices[0].z * w0 + vertices[1].z * w1 + vertices[2].z * w2;

                // if the z value of the current pixel is more than the value store in the zBuffer, we draw the
                // pixel and set the new value of the zBuffer at the current pixel
                if (zbuffer[int(point.x + point.y * WIDTH)] < point.z)
                {
                    // Updating the zBuffer to hold the depth value of the current pixel
                    zbuffer[int(point.x + point.y * WIDTH)] = point.z;

                    // finding the fragment normal by interpolating the barycentric coordinates
                    normal = glm::normalize(normals[0] * w0 + normals[1] * w1 + normals[2] * w2);

                    // If the angle between normal and light direction is more than 90 (i.e. the light does not
                    // illuinate the surface), then the dot product will be less than 0
                    float lightIntensity = glm::dot(lightDirection, normal);

                    // If fragment is not illuminated, then don't draw it
                    if (lightIntensity > 0)
                    {
                        // Finding the diffuse texture coordinates by interpolating the vertex texCoords using
                        // barycentric coordinates
                        texCoord = uvs[0] * w0 + uvs[1] * w1 + uvs[2] * w2;

                        // Changing the brightness of the pixel based on the light intensity
                        color = GetPixelColor(texture, texCoord) * lightIntensity;

                        // Draw the pixel
                        image.set(point.x, point.y, color);
                    }
                }
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

glm::vec3 WorldToScreen(const glm::vec3& vec)
{
    int x = static_cast<int>((vec.x + 1.0f) * HALF_WIDTH + 0.5f);
    int y = static_cast<int>((vec.y + 1.0f) * HALF_HEIGHT + 0.5f);

    return glm::vec3(x, y, vec.z);
}

glm::vec3 CalculateSurfaceNormal(const glm::vec3* const vertices)
{
    glm::vec3 u = vertices[2] - vertices[0];
    glm::vec3 v = vertices[1] - vertices[0];

    return glm::normalize(glm::cross(u, v));
}

void RenderModel(const std::string& path, const std::string& filename, const std::string& ouputName)
{
    Model* model = new Model(path + filename);

    TGAImage wireframeImage(WIDTH, HEIGHT, TGAImage::RGB);
    TGAImage renderImage(WIDTH, HEIGHT, TGAImage::RGB);
    TGAImage depthBufferImage(WIDTH, HEIGHT, TGAImage::RGB);

    Material material = model->GetMaterial();

    // Texture Loading
    stbi_set_flip_vertically_on_load(true);

    Texture texture;
    texture.Data = stbi_load(std::string(path + material.DiffuseTextureName).c_str(), &texture.Width, &texture.Height,
                             &texture.NumComponents, 0);

    if (texture.Data == NULL)
    {
        std::cerr << "Failed to load texture " << material.DiffuseTextureName << "\n";
        return;
    }

    // Initialize the zBuffer and set all values to negative infinity
    float* zBuffer = new float[WIDTH * HEIGHT];

    for (int i = 0; i < WIDTH * HEIGHT; i++)
    {
        zBuffer[i] = -std::numeric_limits<float>::max();
    }

    glm::vec3 lightDirection(0, 0, 1);

    // Loop through all triangles
    for (int i = 0; i < model->GetNumFaces(); i++)
    {
        Face face = model->GetFaceAtIndex(i);

        glm::vec3 vertices[3];
        glm::vec3 normals[3];
        glm::vec2 uvs[3];

        glm::vec3 screenCoords[3];

        for (int j = 0; j < 3; j++)
        {
            vertices[j] = model->GetVertexAtIndex(face[j].VertexIndex);
            uvs[j]      = model->GetTexCoordAtIndex(face[j].TexCoordIndex);
            normals[j]  = model->GetNormalAtIndex(face[j].NormalIndex);

            screenCoords[j] = WorldToScreen(vertices[j]);

            glm::vec3 v0 = vertices[j];
            glm::vec3 v1 = model->GetVertexAtIndex(face[(j + 1) % 3].VertexIndex);
            int       x0 = (v0.x + 1.0f) * HALF_WIDTH;
            int       y0 = (v0.y + 1.0f) * HALF_HEIGHT;
            int       x1 = (v1.x + 1.0f) * HALF_WIDTH;
            int       y1 = (v1.y + 1.0f) * HALF_HEIGHT;
            DrawLine(x0, y0, x1, y1, wireframeImage, WHITE * ((v0.z + 1) / 2));
        }

        DrawTriangle(screenCoords, uvs, normals, texture, zBuffer, renderImage, lightDirection);
    }

    // Render the zBuffer
    for (size_t x = 0; x < WIDTH; x++)
    {
        for (size_t y = 0; y < HEIGHT; y++)
        {
            float zVal = zBuffer[x + y * WIDTH];
            if (zVal > -100000000)
            {
                depthBufferImage.set(x, y, WHITE * ((zVal + 1) / 2));
            }
        }
    }

    std::string wireframeOutputPath = "../Renders/Lesson4/" + ouputName + "Wireframe.tga";
    std::string depthBufferPath     = "../Renders/Lesson4/" + ouputName + "DepthBuffer.tga";
    std::string renderOutputPath    = "../Renders/Lesson4/" + ouputName + "Render.tga";

    wireframeImage.write_tga_file(wireframeOutputPath);
    renderImage.write_tga_file(renderOutputPath);
    depthBufferImage.write_tga_file(depthBufferPath);

    delete model;
}

int main()
{
    GLFWwindow* window;

    if (!glfwInit())
    {
        std::cout << "Could not initialize GLFW!\n";
        return -1;
    }

    window = glfwCreateWindow(640, 480, "Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        std::cout << "Could not create window!\n";
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glBegin(GL_POINTS);
        glColor3f(1, 1, 1);
        glVertex2i(100, 100);
        glVertex2i(100, 101);
        glVertex2i(100, 102);
        glEnd();

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // RenderModel("../Assets/obj/african_head/", "african_head.obj", "Head");
    // RenderModel("../Assets/obj/diablo3_pose/diablo3_pose.obj", "Diablo");
    // RenderModel("../Assets/obj/Gun/", "Gun.obj", "Gun");

    glfwTerminate();
    return 0;
}