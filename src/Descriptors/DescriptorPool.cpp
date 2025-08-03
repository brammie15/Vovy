#include "DescriptorPool.h"

#include <stdexcept>

#include "Utils/DebugLabel.h"

namespace vov {
    DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
        m_poolSizes.push_back({descriptorType, count});
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        m_poolFlags = flags;
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count) {
        m_maxSets = count;
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::SetName(const std::string& name) {
        m_name = name;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
        return std::make_unique<DescriptorPool>(m_device, m_name, m_maxSets, m_poolFlags, m_poolSizes);
    }

    DescriptorPool::DescriptorPool(Device& deviceRef, const std::string& name, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
                                   const std::vector<VkDescriptorPoolSize>& poolSizes): m_device{deviceRef} {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(m_device.device(), &descriptorPoolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        DebugLabel::SetObjectName(
            reinterpret_cast<uint64_t>(m_descriptorPool),
            VK_OBJECT_TYPE_DESCRIPTOR_POOL,
            name.c_str()
        );
    }

    DescriptorPool::~DescriptorPool() {
        vkDestroyDescriptorPool(m_device.device(), m_descriptorPool, nullptr);
    }

    bool DescriptorPool::allocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;


        //Could overflow actually, well fix it in prod
        const auto result = vkAllocateDescriptorSets(m_device.device(), &allocInfo, &descriptor);
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY) {
            throw std::runtime_error("Descriptor pool out of memory");
        }
        return result == VK_SUCCESS;
    }

    void DescriptorPool::freeDescriptors(const std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(
            m_device.device(),
            m_descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data()
        );
    }

    void DescriptorPool::resetPool() const {
        vkResetDescriptorPool(m_device.device(), m_descriptorPool, 0);
    }
}
