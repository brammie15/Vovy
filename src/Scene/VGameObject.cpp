#include "VGameObject.h"

#include <filesystem>
#include <iostream>

std::unique_ptr<VGameObject> VGameObject::LoadModelFromDisk(VDevice& device, const std::string& filepath) {
    if (std::filesystem::exists(filepath)) {
        std::cout << "Loading model from disk: " << filepath << std::endl;
    } else {
        std::cerr << "Model file not found: " << filepath << std::endl;
        return nullptr;
    }

    auto gameObject = VGameObject::createGameObject();
    auto model = std::make_shared<VModel>(device, filepath, gameObject.get());
    gameObject->model = std::move(model);
    return gameObject;
}
