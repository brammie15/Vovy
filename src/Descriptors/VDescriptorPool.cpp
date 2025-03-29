#include "VDescriptorPool.h"

#include <stdexcept>

VDescriptorPool::Builder& VDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
    m_poolSizes.push_back({descriptorType, count});
    return *this;
}

VDescriptorPool::Builder& VDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
    m_poolFlags = flags;
    return *this;
}

VDescriptorPool::Builder& VDescriptorPool::Builder::setMaxSets(uint32_t count) {
    m_maxSets = count;
    return *this;
}

std::unique_ptr<VDescriptorPool> VDescriptorPool::Builder::build() const {
    return std::make_unique<VDescriptorPool>(m_device, m_maxSets, m_poolFlags, m_poolSizes);
}

VDescriptorPool::VDescriptorPool(VDevice& deviceRef, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
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
}

VDescriptorPool::~VDescriptorPool() {
    vkDestroyDescriptorPool(m_device.device(), m_descriptorPool, nullptr);
}

bool VDescriptorPool::allocateDescriptor(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;


    //Could overflow actually, well fix it in prod
    return vkAllocateDescriptorSets(m_device.device(), &allocInfo, &descriptor) == VK_SUCCESS;
}

void VDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
    vkFreeDescriptorSets(
        m_device.device(),
        m_descriptorPool,
        static_cast<uint32_t>(descriptors.size()),
        descriptors.data()
    );
}

void VDescriptorPool::resetPool() {
    vkResetDescriptorPool(m_device.device(), m_descriptorPool, 0);
}
