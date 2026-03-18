#include <GopherEngine/Resource/ResourceManager.hpp>
#include <GopherEngine/Resource/MeshFactory.hpp>

#include <SFML/Graphics/Image.hpp>

#include <assimp/Importer.hpp>      // Mesh importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <iostream>
using namespace std;

namespace GopherEngine
{

    // Loads a single texture file from disk asynchronously (for demonstration purposes only).
    // This method uses SFML to decode the image file. This functionality would not be used
    // by the runtime engine; it should performed by the asset conditioning pipeline instead. 
    // The runtime engine should only have to deal with raw pixel data, not image files.
    void ResourceManager::load_texture(
                std::filesystem::path path,
                std::function<void(std::shared_ptr<Texture>)> on_success,
                std::function<void(const std::string&)> on_error) {

        LoadHandle handle = FileLoader::load_file_async(path);

        handle.on_complete([on_success = std::move(on_success), on_error = std::move(on_error)](const FileData& file_data)
        {
            if(!file_data.ok_) {
                if(on_error) 
                    on_error(file_data.error_);
                return;
            }

            sf::Image image;
            if (!image.loadFromMemory(file_data.bytes_.data(), file_data.bytes_.size())) {
                if(on_error) 
                    on_error("Failed to decode image: " + file_data.path_.string());
                return;
            }

            // Create a texture resource. 
            auto texture = std::make_shared<Texture>();

            // Copy pixel data from the SFML image into our Texture structure. 
            texture->format_ = Texture::Format::RGBA;
            texture->width_ = image.getSize().x;
            texture->height_ = image.getSize().y;
            texture->pixels_.assign(
                image.getPixelsPtr(),
                image.getPixelsPtr() + texture->width_ * texture->height_ * 4
            );

            if(on_success)
                on_success(texture);
        });

        load_handles_.push_back(std::move(handle));
    }

    bool ResourceManager::poll()
    {
        // Partition load handles into "not ready" and "ready to fire"
        auto it = std::partition(
            load_handles_.begin(), load_handles_.end(),
            [](const LoadHandle& h) { return !h.is_ready(); }
        );

        // Fire callbacks for all ready handles (in original order)
        for (auto fired = it; fired != load_handles_.end(); ++fired)
            fired->fire_callback();

        // Erase all ready handles from the vector
        load_handles_.erase(it, load_handles_.end());

        // Return true if no more pending loads
        return load_handles_.empty();
    }
}