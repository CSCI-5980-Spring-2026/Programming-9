#include <GopherEngine/Resource/ResourceManager.hpp>
#include <GopherEngine/Resource/MeshFactory.hpp>

#include <SFML/Graphics/Image.hpp>

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

        // If the same texture is already loaded, invoke the success callback immediately with
        // the cached resource from the registry. This allows multiple resources that reference
        // the same texture file to share a single Texture instance in memory.
        Guid guid = Guid::from_name(path.string());
        if(has_texture(guid)) {
            if(on_success)
                on_success(get_texture(guid));
            return;
        }
        
        LoadHandle handle = FileLoader::load_file_async(path);

        handle.on_complete([this, guid, on_success = std::move(on_success), on_error = std::move(on_error)](const FileData& file_data)
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

            // Create a texture resource. The name should be the original file path, since
            // that's the canonical identity of the resource. The GUID is derived from
            // the name, so it will be stable and deterministic across runs.
            auto texture = std::make_shared<Texture>();
            texture->name_ = file_data.path_.string();
            texture->guid_ = guid;

            // Add the loaded texture to the registry so it can be retrieved by GUID later
            texture_registry_[texture->guid_] = texture;

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

    std::shared_ptr<Texture> ResourceManager::get_texture(const Guid& guid) const
    {
        auto it = texture_registry_.find(guid);
        if (it != texture_registry_.end())
            return it->second;
        return nullptr;
    }

    bool ResourceManager::has_texture(const Guid& guid) const
    {
        return texture_registry_.find(guid) != texture_registry_.end();
    }

    std::shared_ptr<Texture> ResourceManager::register_texture(const std::shared_ptr<Texture>& texture)
    {
        if(!texture)
            return nullptr;
            
        auto it = texture_registry_.find(texture->guid_);
        if (it != texture_registry_.end())
            return it->second;

        
        texture_registry_[texture->guid_] = texture;
        return texture;
    }

}