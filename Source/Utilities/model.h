#pragma once

#include "glm/glm.hpp"

#include <array>
#include <string>
#include <vector>

struct Texture
{
    unsigned char* Data;
    int            Width;
    int            Height;
    int            NumComponents;
};

struct Material
{
    std::string DiffuseTextureName;
};

struct Index
{
    int VertexIndex;
    int NormalIndex;
    int TexCoordIndex;
};

using Face = std::array<Index, 3>;

class Model
{

  public:
    Model(const std::string& filename);
    ~Model();
    inline int       GetNumVertices() const { return static_cast<int>(m_Vertices.size()); }
    inline int       GetNumFaces() const { return static_cast<int>(m_Faces.size()); }
    inline glm::vec3 GetVertexAtIndex(int index) const { return m_Vertices[index]; }
    inline glm::vec2 GetTexCoordAtIndex(int index) const { return m_TexCoords[index]; }
    inline glm::vec3 GetNormalAtIndex(int index) const { return m_Normals[index]; }
    inline Face      GetFaceAtIndex(int index) const { return m_Faces[index]; }
    inline Material  GetMaterial() const { return m_Material; }

  private:
    std::vector<glm::vec3> m_Vertices;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    std::vector<Face>      m_Faces;
    Material               m_Material;
};
