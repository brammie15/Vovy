#ifndef VDESCRIPTORWRITER_H
#define VDESCRIPTORWRITER_H

#include "VDescriptorPool.h"
#include "VDescriptorSetLayout.h"
#include "Core/VDevice.h"

class VDescriptorWriter {
public:
    VDescriptorWriter(VDescriptorSetLayout &setLayout, VDescriptorPool &pool);

    VDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
    VDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

    bool build(VkDescriptorSet &set);
    void overwrite(VkDescriptorSet &set);

private:
    VDescriptorSetLayout& m_setLayout;
    VDescriptorPool& m_pool;
    std::vector<VkWriteDescriptorSet> m_writes;
};

#endif //VDESCRIPTORWRITER_H
