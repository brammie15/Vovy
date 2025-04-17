#ifndef VDESCRIPTORWRITER_H
#define VDESCRIPTORWRITER_H

#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "Core/Device.h"

namespace vov {
    class DescriptorWriter {
    public:
        DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool);

        DescriptorWriter& writeBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);
        DescriptorWriter& writeImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet &set);
        void overwrite(const VkDescriptorSet &set);

    private:
        DescriptorSetLayout& m_setLayout;
        DescriptorPool& m_pool;
        std::vector<VkWriteDescriptorSet> m_writes;
    };
}
#endif //VDESCRIPTORWRITER_H
