#ifndef VDESCRIPTORS_H
#define VDESCRIPTORS_H

#include <memory>
#include <unordered_map>
#include "Core/Device.h"
namespace vov {
    class DescriptorSetLayout {
    public:
        class Builder {
        public:
            explicit Builder(Device& deviceRef): m_device{deviceRef} {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType type,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1
            );

            std::unique_ptr<DescriptorSetLayout> build();

        private:
            Device& m_device;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
        };

        DescriptorSetLayout(Device& deviceRef, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
        ~DescriptorSetLayout();
        DescriptorSetLayout(const DescriptorSetLayout &) = delete;
        DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

        [[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

    private:
        Device& m_device;
        VkDescriptorSetLayout m_descriptorSetLayout{};
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

        friend class DescriptorWriter;
    };
}

#endif //VDESCRIPTORS_H
