#include "VDescriptorSetLayout.h"

#include <cassert>
#include <stdexcept>

VDescriptorSetLayout::Builder& VDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType type,
                                                                         VkShaderStageFlags stageFlags,
                                                                         uint32_t count) {
    assert(m_bindings.count(binding) == 0 && "Binding already in use");
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    m_bindings[binding] = layoutBinding;
    return *this;
}

std::unique_ptr<VDescriptorSetLayout> VDescriptorSetLayout::Builder::build() {
    return std::make_unique<VDescriptorSetLayout>(m_device, m_bindings);
}

VDescriptorSetLayout::VDescriptorSetLayout(VDevice& deviceRef,
                                           std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings): m_device{deviceRef}, m_bindings{bindings} {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
    for (auto kv: bindings) {
        setLayoutBindings.push_back(kv.second);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(
            m_device.device(),
            &descriptorSetLayoutInfo,
            nullptr,
            &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

VDescriptorSetLayout::~VDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(m_device.device(), m_descriptorSetLayout, nullptr);
}
