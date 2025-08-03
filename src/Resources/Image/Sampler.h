#ifndef SAMPLER_H
#define SAMPLER_H
#include "Core/Device.h"

namespace vov {
    class Sampler {
    public:
        explicit Sampler(Device& device, VkFilter filter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, uint32_t mipLevels = 1);
        ~Sampler();

        [[nodiscard]] VkSampler getHandle() const { return m_sampler; }
        void SetName(const std::string& name) const;

    private:
        Device& m_device;
        VkSampler m_sampler{};
    };
}
#endif //SAMPLER_H
