#pragma once

#include <GopherEngine/Core/Service.hpp>
#include <GopherEngine/Core/FileLoader.hpp>
#include <GopherEngine/Resource/Texture.hpp>
#include <GopherEngine/Resource/Mesh.hpp>

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

            bool poll();

        private:
            std::vector<LoadHandle> load_handles_;
    };
}