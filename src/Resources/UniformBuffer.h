#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H
#include <memory>
#include <stdexcept>

#include "Buffer.h"
#include "Core/Device.h"
#include "Rendering/Swapchain.h"

namespace vov {
    template <typename T>
    class UniformBuffer {
    public:
        explicit UniformBuffer(Device& deviceRef);

        void update(int frameIndex, const T& data);

        [[nodiscard]] Buffer* getBuffer(int frameIndex) const;
        [[nodiscard]] VkDescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const;


        [[nodiscard]] const std::vector<std::unique_ptr<Buffer>>& getBuffers() const {
            return m_buffers;
        }

        [[nodiscard]] static size_t getSize() {
            return sizeof(T);
        }

        Buffer* operator[](int frameIndex) const {
            if (frameIndex < 0 || frameIndex >= Swapchain::MAX_FRAMES_IN_FLIGHT) {
                throw std::out_of_range("Frame index out of range");
            }
            return m_buffers[frameIndex].get();
        }

        void SetName(const std::string& name) const {
            for (int i{0}; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
                m_buffers[i]->SetName(name + "_frame_" + std::to_string(i));
            }
        }

    private:

        void createBuffers();

        std::vector<std::unique_ptr<Buffer>> m_buffers;

        Device& m_device;
    };

    template <typename T>
    UniformBuffer<T>::UniformBuffer(Device& deviceRef): m_device(deviceRef) {
        createBuffers();
    }

    template <typename T>
    void UniformBuffer<T>::update(int frameIndex, const T& data) {
        if (frameIndex < 0 || frameIndex >= Swapchain::MAX_FRAMES_IN_FLIGHT) {
            throw std::out_of_range("Frame index out of range");
        }
        m_buffers[frameIndex]->copyTo(&data, sizeof(T));
        m_buffers[frameIndex]->flush();
    }

    template <typename T>
    Buffer* UniformBuffer<T>::getBuffer(int frameIndex) const {
        return m_buffers[frameIndex].get();
    }

    template <typename T>
    VkDescriptorBufferInfo UniformBuffer<T>::getDescriptorBufferInfo(uint32_t frameIndex) const {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_buffers[frameIndex]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(T);
        return bufferInfo;
    }

    template <typename T>
    void UniformBuffer<T>::createBuffers() {
        for (size_t i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++) {
            m_buffers.emplace_back(std::make_unique<Buffer>(
                m_device,
                sizeof(T),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, true
            ));
        }
    }
}

#endif //UNIFORMBUFFER_H