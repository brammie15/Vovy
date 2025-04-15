#ifndef VDESCRIPTORS_H
#define VDESCRIPTORS_H

#include <memory>
#include <unordered_map>
#include "Core/VDevice.h"

class VDescriptorSetLayout {
public:
    class Builder {
    public:
        explicit Builder(VDevice& deviceRef): m_device{deviceRef} {}

        Builder& addBinding(
            uint32_t binding,
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            uint32_t count = 1
        );

        std::unique_ptr<VDescriptorSetLayout> build();

    private:
        VDevice& m_device;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
    };

    VDescriptorSetLayout(VDevice& deviceRef, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
    ~VDescriptorSetLayout();
    VDescriptorSetLayout(const VDescriptorSetLayout &) = delete;
    VDescriptorSetLayout &operator=(const VDescriptorSetLayout &) = delete;

    [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    VDevice& m_device;
    VkDescriptorSetLayout m_descriptorSetLayout{};
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class VDescriptorWriter;
};

#endif //VDESCRIPTORS_H
