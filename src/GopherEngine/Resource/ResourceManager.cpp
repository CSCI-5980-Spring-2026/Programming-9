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

    // Loads a single mesh file from disk asynchronously (for demonstration purposes only).
    // This method uses Assimp to decode the mesh file. This functionality would not be used
    // by the runtime engine; it should performed by the asset conditioning pipeline instead.
    // Note that if the file contains multiple meshes, only the first one is loaded, and
    // Embedded materials and textures are ignored. The asset conditioning pipeline should be
    // responsible for flattening complex source assets into simple runtime resources with 
    // explicit dependencies (e.g. separate mesh and texture files).
    void ResourceManager::load_mesh(
                std::filesystem::path path,
                std::function<void(std::shared_ptr<Mesh>)> on_success,
                std::function<void(const std::string&)> on_error) {

        // If the same mesh is already loaded, invoke the success callback immediately with
        // the cached resource from the registry. This allows multiple resources that reference
        // the same mesh file to share a single Mesh instance in memory.
        Guid guid = Guid::from_name(path.string());
        if(has_mesh(guid)) {
            if(on_success)
                on_success(get_mesh(guid));
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

            // The copying of vertex data from Assimp's structures into our own Mesh structure
            // is much too slow, resulting in a long initialization time just for a single mesh.
            // The asset conditioning pipeline should be responsible for this kind of data transformation,
            // so that the runtime engine only has to deal with simple, consistent resource formats.
            Assimp::Importer importer;
            auto scene = importer.ReadFileFromMemory(
                file_data.bytes_.data(),
                file_data.bytes_.size(),
                aiProcess_Triangulate,
                "obj");

            if(!scene || scene->mNumMeshes == 0) {
                if(on_error) 
                    on_error("Failed to decode mesh: " + file_data.path_.string());
                return;
            }

            // Create a mesh resource. The name should be the original file path, since
            // that's the canonical identity of the resource. The GUID is derived from
            // the name, so it will be stable and deterministic across runs.
            auto mesh = std::make_shared<Mesh>();
            mesh->name_ = file_data.path_.string();
            mesh->guid_ = guid;

            // Add the loaded mesh to the registry so it can be retrieved by GUID later
            mesh_registry_[mesh->guid_] = mesh;

            // For demonstration purposes, we only load the first mesh in the file. 
            auto ai_mesh = scene->mMeshes[0];

            // Reserve enough space in the array buffer to hold all the interleaved vertex attributes 
            mesh->array_buffer_.reserve(ai_mesh->mNumVertices * 12);

            // Interleave vertex attributes into a single array buffer
            for(unsigned int i = 0; i < ai_mesh->mNumVertices; i++) {

                if(ai_mesh->HasPositions()) {
                    mesh->array_buffer_.push_back(ai_mesh->mVertices[i].x);
                    mesh->array_buffer_.push_back(ai_mesh->mVertices[i].y);
                    mesh->array_buffer_.push_back(ai_mesh->mVertices[i].z);
                } 
                else {
                    mesh->array_buffer_.insert(mesh->array_buffer_.end(), {0.f, 0.f, 0.f});
                }

                if(ai_mesh->HasNormals()) {
                    mesh->array_buffer_.push_back(ai_mesh->mNormals[i].x);
                    mesh->array_buffer_.push_back(ai_mesh->mNormals[i].y);
                    mesh->array_buffer_.push_back(ai_mesh->mNormals[i].z);
                } 
                else {
                    mesh->array_buffer_.insert(mesh->array_buffer_.end(), {0.f, 0.f, 0.f});
                }

                if(ai_mesh->GetNumColorChannels() == 4) {
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->r);
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->g);
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->b);
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->a);
                } 
                else if(ai_mesh->GetNumColorChannels() == 3) {
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->r);
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->g);
                    mesh->array_buffer_.push_back(ai_mesh->mColors[i]->b);
                    mesh->array_buffer_.push_back(1.f);
                }
                else {
                    mesh->array_buffer_.insert(mesh->array_buffer_.end(), {1.f, 1.f, 1.f, 1.f});
                }

                if (ai_mesh->GetNumUVChannels() > 0) {
                    mesh->array_buffer_.push_back(ai_mesh->mTextureCoords[0][i].x);
                    mesh->array_buffer_.push_back(ai_mesh->mTextureCoords[0][i].y);
                } 
                else {
                    mesh->array_buffer_.insert(mesh->array_buffer_.end(), {0.f, 0.f});
                }

            }

            // Reserve enough space in the element buffer to hold all the face indices (flattened)
            mesh->element_buffer_.reserve(ai_mesh->mNumFaces * 3);

            // Flatten face index data into a single element buffer
            for (unsigned int f = 0; f < ai_mesh->mNumFaces; f++) {
                mesh->element_buffer_.push_back(ai_mesh->mFaces[f].mIndices[0]);
                mesh->element_buffer_.push_back(ai_mesh->mFaces[f].mIndices[1]);
                mesh->element_buffer_.push_back(ai_mesh->mFaces[f].mIndices[2]);
            }

            if(on_success)
                on_success(mesh);
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

    std::shared_ptr<Mesh> ResourceManager::get_mesh(const Guid& guid) const
    {
        auto it = mesh_registry_.find(guid);
        if (it != mesh_registry_.end())
            return it->second;
        return nullptr;
    }

    bool ResourceManager::has_texture(const Guid& guid) const
    {
        return texture_registry_.find(guid) != texture_registry_.end();
    }

    bool ResourceManager::has_mesh(const Guid& guid) const
    {
        return mesh_registry_.find(guid) != mesh_registry_.end();
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

    std::shared_ptr<Mesh> ResourceManager::register_mesh(const std::shared_ptr<Mesh>& mesh)
    {
        if(!mesh)
            return nullptr;

        auto it = mesh_registry_.find(mesh->guid_);
        if (it != mesh_registry_.end())
            return it->second;

        mesh_registry_[mesh->guid_] = mesh;
        return mesh;
    }

    void ResourceManager::remove_all_meshes()
    {
        mesh_registry_.clear();
    }

    void ResourceManager::remove_all_textures()
    {
        texture_registry_.clear();
    }

    void ResourceManager::remove_all()
    {
        remove_all_meshes();
        remove_all_textures();
    }

}