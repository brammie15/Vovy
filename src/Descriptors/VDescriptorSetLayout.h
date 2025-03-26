#ifndef VDESCRIPTORS_H
#define VDESCRIPTORS_H

#include "../VDevice.h"
#include <memory>
#include <unordered_map>

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

    VDescriptorSetLayout(VDevice& deviceRef, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~VDescriptorSetLayout();
    VDescriptorSetLayout(const VDescriptorSetLayout &) = delete;
    VDescriptorSetLayout &operator=(const VDescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
    VDevice& m_device;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

    friend class VDescriptorWriter;
};

#endif //VDESCRIPTORS_H
