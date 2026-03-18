#pragma once

#include <GopherEngine/Core/Service.hpp>
#include <GopherEngine/Core/FileLoader.hpp>
#include <GopherEngine/Resource/Texture.hpp>
#include <GopherEngine/Resource/Mesh.hpp>
#include <GopherEngine/Core/Guid.hpp>

#include <vector>
#include <memory>
#include <filesystem>

namespace GopherEngine
{
    class ResourceManager: public Service<ResourceManager>
    {
        public:

            void load_texture(
                std::filesystem::path path,
                std::function<void(std::shared_ptr<Texture>)> on_success = nullptr,
                std::function<void(const std::string&)> on_error = nullptr);

            void load_mesh(
                std::filesystem::path path,
                std::function<void(std::shared_ptr<Mesh>)> on_success = nullptr,
                std::function<void(const std::string&)> on_error = nullptr);

            bool poll();

            std::shared_ptr<Texture> get_texture(const Guid& guid) const;
            std::shared_ptr<Mesh> get_mesh(const Guid& guid) const;

            bool has_texture(const Guid& guid) const;
            bool has_mesh(const Guid& guid) const;

            std::shared_ptr<Texture> register_texture(const std::shared_ptr<Texture>& texture);
            std::shared_ptr<Mesh> register_mesh(const std::shared_ptr<Mesh>& mesh);

            void remove_all_meshes();
            void remove_all_textures();
            void remove_all();

        private:
            std::vector<LoadHandle> load_handles_;
            std::unordered_map<Guid, std::shared_ptr<Texture>, GuidHasher> texture_registry_;
            std::unordered_map<Guid, std::shared_ptr<Mesh>, GuidHasher> mesh_registry_;
    };
}