#ifndef VDESCRIPTORPOOL_H
#define VDESCRIPTORPOOL_H
#include <memory>

#include "Core/Device.h"

namespace vov {
    class DescriptorPool {
    public:
        class Builder {
        public:
            explicit Builder(Device& deviceRef) : m_device{deviceRef} {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            [[nodiscard]] std::unique_ptr<DescriptorPool> build() const;

        private:
            Device& m_device;
            std::vector<VkDescriptorPoolSize> m_poolSizes{};
            uint32_t m_maxSets = 1000;
            VkDescriptorPoolCreateFlags m_poolFlags = 0;
        };

        DescriptorPool(
            Device& deviceRef,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize> &poolSizes);
        ~DescriptorPool();
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool &operator=(const DescriptorPool&) = delete;

        [[nodiscard]] VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
        [[nodiscard]] Device& getDevice() const { return m_device; }

        bool allocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
        void freeDescriptors(const std::vector<VkDescriptorSet> &descriptors) const;
        void resetPool() const;

    private:
        Device& m_device;
        VkDescriptorPool m_descriptorPool{};

        friend class DescriptorWriter;
    };
}

#endif //VDESCRIPTORPOOL_H
