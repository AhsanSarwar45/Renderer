#include "model.h"

#include <iostream>

#include <tinyobjloader/tiny_obj_loader.h>

Model::Model(const std::string& filename) : m_Vertices(), m_Faces()
{
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename))
    {
        if (!reader.Error().empty())
        {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty())
    {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    const tinyobj::attrib_t&                attrib    = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>&    shapes    = reader.GetShapes();
    const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

    for (size_t i = 0; i < attrib.vertices.size(); i += 3)
    {
        m_Vertices.push_back(glm::vec3(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]));
    }
    for (size_t i = 0; i < attrib.normals.size(); i += 3)
    {
        m_Normals.push_back(glm::vec3(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]));
    }
    for (size_t i = 0; i < attrib.texcoords.size(); i += 2)
    {
        m_TexCoords.push_back(glm::vec2(attrib.texcoords[i], attrib.texcoords[i + 1]));
    }
    for (size_t i = 0; i < shapes[0].mesh.indices.size(); i += 3)
    {
        tinyobj::index_t i1 = shapes[0].mesh.indices[i];
        tinyobj::index_t i2 = shapes[0].mesh.indices[i + 1];
        tinyobj::index_t i3 = shapes[0].mesh.indices[i + 2];

        Index index1 = {i1.vertex_index, i1.normal_index, i1.texcoord_index};
        Index index2 = {i2.vertex_index, i2.normal_index, i2.texcoord_index};
        Index index3 = {i3.vertex_index, i3.normal_index, i3.texcoord_index};

        m_Faces.push_back({index1, index2, index3});
    }

    m_Material.DiffuseTextureName = materials[0].diffuse_texname;

    std::cout << "Model: " << filename << "\n";
    std::cout << "Vertices: " << m_Vertices.size() << "\n";
    std::cout << "Normals: " << m_Normals.size() << "\n";
    std::cout << "TexCoords: " << m_TexCoords.size() << "\n";
    std::cout << "Faces: " << m_Faces.size() << "\n";
}

Model::~Model() {}
