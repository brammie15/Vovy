#ifndef HDRI_H
#define HDRI_H
#include <array>

#include "Image.h"
#include "Core/Device.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Image/ImageView.h"
#include "Image/Sampler.h"

namespace vov {
    class HDRI {
    public:
        explicit HDRI(Device& deviceRef);
        ~HDRI();

        void LoadHDR(const std::string& filename);

        void RenderToCubemap(VkImage inputImage, VkImageView inputView, VkSampler sampler,
                       VkImage outputImage, std::array<VkImageView, 6> faceViews, uint32_t size,
                       const std::string& vertPath, const std::string& fragPath);
        void CreateCubeMap();

        void CreateDiffuseIrradianceMap();

        [[nodiscard]] VkImage GetCubeMap() const { return m_cubeMap; }
        [[nodiscard]] VkImageView GetCubeMapView() const { return m_skyboxView; }
        [[nodiscard]] VkSampler GetSampler() const { return m_sampler.getHandle(); }
        [[nodiscard]] VkImageView GetHDRView() const { return m_hdrView->getHandle(); }
        [[nodiscard]] VkSampler GetHDRSampler() const { return m_hdrSampler->getHandle(); }
        [[nodiscard]] VkImageView GetIrradianceView() const { return m_diffuseIrradianceView; }
        [[nodiscard]] VkImage GetIrradianceMap() const { return m_diffuseIrradianceMap; }

        [[nodiscard]] uint32_t GetCubeMapSize() const { return m_cubeMapSize; }
        [[nodiscard]] uint32_t GetDiffuseIrradianceMapSize() const { return m_diffuseIrradianceMapSize; }

    private:

        void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t arrayLayers);
        //Since i use this and not vov::image
        void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t arrayLayers);


        Device& m_device;

        // HDRI image
        VkImage m_hdrImage{VK_NULL_HANDLE};
        VmaAllocation m_allocation{VK_NULL_HANDLE};
        std::unique_ptr<ImageView> m_hdrView{nullptr};
        std::unique_ptr<Sampler> m_hdrSampler{nullptr};

        Sampler m_sampler{m_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1};

        // Cube map (skybox)
        VkImage m_cubeMap;
        VmaAllocation m_cubeMapAllocation;
        VkImageView m_skyboxView;
        uint32_t m_cubeMapSize = 1024; // Default size for cube map

        VkImage m_diffuseIrradianceMap = VK_NULL_HANDLE;
        VmaAllocation m_diffuseIrradianceAllocation = VK_NULL_HANDLE;
        VkImageView m_diffuseIrradianceView = VK_NULL_HANDLE;
        uint32_t m_diffuseIrradianceMapSize = 256; // Default size for diffuse irradiance map


        const std::array<glm::mat4, 6> viewMatrices = {
            // POSITIVE_X
            glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            // NEGATIVE_X
            glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            // POSITIVE_Y
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            // NEGATIVE_Y
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            // POSITIVE_Z
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            // NEGATIVE_Z
            glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    };
}

#endif //HDRI_H
