#include "GameObject.h"

#include <filesystem>
#include <iostream>

namespace vov {
    std::unique_ptr<GameObject> GameObject::LoadModelFromDisk(Device& device, const std::string& filepath) {
        if (std::filesystem::exists(filepath)) {
            std::cout << "Loading model from disk: " << filepath << std::endl;
        } else {
            std::cerr << "Model file not found: " << filepath << std::endl;
            return nullptr;
        }

        auto gameObject = GameObject::createGameObject();
        auto model = std::make_shared<Model>(device, filepath, gameObject.get());
        gameObject->model = std::move(model);
        return gameObject;
    }
}
