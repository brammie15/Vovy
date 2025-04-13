#ifndef VDESCRIPTORPOOL_H
#define VDESCRIPTORPOOL_H
#include <memory>

#include "Core/VDevice.h"

class VDescriptorPool {
public:
    class Builder {
    public:
        Builder(VDevice& deviceRef) : m_device{deviceRef} {}

        Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder& setMaxSets(uint32_t count);
        [[nodiscard]] std::unique_ptr<VDescriptorPool> build() const;

    private:
        VDevice& m_device;
        std::vector<VkDescriptorPoolSize> m_poolSizes{};
        uint32_t m_maxSets = 1000;
        VkDescriptorPoolCreateFlags m_poolFlags = 0;
    };

    VDescriptorPool(
        VDevice& deviceRef,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize> &poolSizes);
    ~VDescriptorPool();
    VDescriptorPool(const VDescriptorPool&) = delete;
    VDescriptorPool &operator=(const VDescriptorPool&) = delete;

    [[nodiscard]] VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
    [[nodiscard]] VDevice& getDevice() const { return m_device; }

    bool allocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
    void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;
    void resetPool();

private:
    VDevice& m_device;
    VkDescriptorPool m_descriptorPool;

    friend class VDescriptorWriter;
};


#endif //VDESCRIPTORPOOL_H
