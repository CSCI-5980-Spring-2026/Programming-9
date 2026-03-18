#pragma once

#include <GopherEngine/Resource/Mesh.hpp>
using namespace GopherEngine;

#include <memory>
#include <string>
#include <initializer_list>

namespace GopherEngine {

    class MeshFactory {
        public:
            static std::shared_ptr<Mesh> create_cube(float width = 1.f, float height = 1.f, float depth = 1.f);
            
            static std::vector<float> interleave_vertex_data(
                const std::vector<glm::vec3>& vertices, 
                const std::vector<glm::vec3>& normals, 
                const std::vector<glm::vec4>& colors, 
                const std::vector<glm::vec2>& uvs);

        private:
            static std::string make_generated_mesh_name(
                const std::string& type,
                std::initializer_list<std::pair<std::string, float>> params);

            static std::string make_canonical_float(float value, int precision = 6);
    };

} 