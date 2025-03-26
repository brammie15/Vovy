#include "VDescriptorWriter.h"

#include <cassert>

VDescriptorWriter::VDescriptorWriter(VDescriptorSetLayout& setLayout, VDescriptorPool& pool): m_setLayout{setLayout}, m_pool{pool} {}

VDescriptorWriter& VDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
    assert(m_setLayout.m_bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = m_setLayout.m_bindings[binding];

    assert(
        bindingDescription.descriptorCount == 1 &&
        "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = bufferInfo;
    write.descriptorCount = 1;

    m_writes.push_back(write);
    return *this;
}

VDescriptorWriter& VDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
    assert(m_setLayout.m_bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = m_setLayout.m_bindings[binding];

    assert(
        bindingDescription.descriptorCount == 1 &&
        "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = imageInfo;
    write.descriptorCount = 1;

    m_writes.push_back(write);
    return *this;
}

bool VDescriptorWriter::build(VkDescriptorSet& set) {
    bool succes = m_pool.allocateDescriptor(m_setLayout.getDescriptorSetLayout(), set);
    if (!succes) {
        return false;
    }
    overwrite(set);
    return true;
}

void VDescriptorWriter::overwrite(VkDescriptorSet& set) {
    for (auto& write: m_writes) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(m_pool.m_device.device(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
}



