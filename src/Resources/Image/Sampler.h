#ifndef SAMPLER_H
#define SAMPLER_H
#include "Core/Device.h"

namespace vov {
    class Sampler {
    public:
        Sampler(Device& device, VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, uint32_t mipLevels = 1);
        ~Sampler();

        [[nodiscard]] VkSampler getHandle() const { return m_sampler; }

    private:
        Device& m_device;
        VkSampler m_sampler{};
    };
}
#endif //SAMPLER_H
