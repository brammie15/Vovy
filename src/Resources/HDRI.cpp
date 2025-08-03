#include "HDRI.h"
#include <array>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/stb_image.h"

#include "Buffer.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Rendering/Pipeline.h"
#include "Rendering/Renderer.h"
#include "Utils/DebugLabel.h"

vov::HDRI::HDRI(Device& deviceRef): m_device(deviceRef), m_cubeMap{nullptr}, m_cubeMapAllocation{nullptr}, m_skyboxView{nullptr} {
    m_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);;
    // m_projection[1][1] *= -1; // Flip Y coordinate for OpenGL compatibility
}

vov::HDRI::~HDRI() {
    vmaDestroyImage(m_device.allocator(), m_hdrImage, m_allocation);
    vmaDestroyImage(m_device.allocator(), m_cubeMap, m_cubeMapAllocation);
    vkDestroyImageView(m_device.device(), m_skyboxView, nullptr);
    vkDestroyImageView(m_device.device(), m_diffuseIrradianceView, nullptr);
    vmaDestroyImage(m_device.allocator(), m_diffuseIrradianceMap, m_diffuseIrradianceAllocation);
}

void vov::HDRI::LoadHDR(const std::string& filename) {
    int width, height, channels;
    float* pixels = stbi_loadf(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load HDR image!");
    }

    const auto imageSize = static_cast<VkDeviceSize>(width * height * 4 * sizeof(float));

    const Buffer stagingBuffer(m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    stagingBuffer.copyTo(pixels, imageSize);

    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = height;
    imageInfo.extent.width = width;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_device.allocator(), &imageInfo, &allocInfo, &m_hdrImage, &m_allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image with VMA!");
    }
    DebugLabel::NameImage(m_hdrImage, filename);

    m_hdrView = std::make_unique<ImageView>(m_device, m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    m_hdrView->SetName("HDRI Image View");

    m_device.TransitionImageLayout(m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    m_device.copyBufferToImage(stagingBuffer.getBuffer(), m_hdrImage, width, height);

    m_hdrSampler = std::make_unique<Sampler>(m_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);
    m_hdrSampler->SetName("HDRI Sampler");

    m_device.TransitionImageLayout(m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
}

void vov::HDRI::RenderToCubemap(VkImage inputImage, VkImageView inputView, VkSampler sampler,
                       VkImage outputImage, std::array<VkImageView, 6> faceViews, uint32_t size,
                       const std::string& vertPath, const std::string& fragPath) {
    VkCommandBuffer cmdBuffer = m_device.beginSingleTimeCommands();

    struct PushConstants {
        glm::mat4 view;
        glm::mat4 projection;
    };

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = outputImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    std::unique_ptr<DescriptorPool> descriptorPool = DescriptorPool::Builder(m_device)
        .SetName("HDRI Descriptor Pool")
        .setMaxSets(1)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        .build();

    std::unique_ptr<DescriptorSetLayout> descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts = {
        descriptorSetLayout->getDescriptorSetLayout()
    };

    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    PipelineConfigInfo pipelineConfig{};
    Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.vertexAttributeDescriptions = {};
    pipelineConfig.vertexBindingDescriptions = {};

    pipelineConfig.pipelineLayout = pipelineLayout;

    pipelineConfig.colorAttachments = {VK_FORMAT_R32G32B32A32_SFLOAT};

    auto pipeline = std::make_unique<Pipeline>(
        m_device,
        vertPath,
        fragPath,
        pipelineConfig
    );

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(size);
    viewport.height = static_cast<float>(size);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { size, size };

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = inputView;
    imageInfo.sampler = sampler;

    VkDescriptorSet descriptorSet;

    DescriptorWriter writer(*descriptorSetLayout, *descriptorPool);
    writer.writeImage(0, &imageInfo);
    writer.build(descriptorSet);

    for (uint32_t face{ 0 }; face < 6; ++face) {
        PushConstants pc{};
        pc.view = viewMatrices[face];
        pc.projection = m_projection;

        VkRenderingAttachmentInfoKHR colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachment.imageView = faceViews[face];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

        VkRenderingInfoKHR renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea.offset = {0, 0};
        renderingInfo.renderArea.extent = {size, size};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vov::vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);

        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

        pipeline->bind(cmdBuffer);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                              0, 1, &descriptorSet, 0, nullptr);

        vkCmdPushConstants(
             cmdBuffer,
             pipelineLayout,
             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
             0,
             sizeof(PushConstants),
             &pc
         );

        vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

        vov::vkCmdEndRenderingKHR(cmdBuffer);
    }

    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.image = outputImage;

    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    m_device.endSingleTimeCommands(cmdBuffer);

    vkDestroyPipelineLayout(m_device.device(), pipelineLayout, nullptr);
}

void vov::HDRI::CreateCubeMap() {
    const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(m_cubeMapSize))) + 1;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = m_cubeMapSize;
    imageInfo.extent.width = m_cubeMapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 6; // 6 faces for cubemap
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Create a cubemap

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_device.allocator(), &imageInfo, &allocInfo, &m_cubeMap, &m_cubeMapAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image with VMA!");
    }


    std::array<VkImageView, 6> faceViews{};
    for (uint32_t face = 0; face < 6; ++face) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_cubeMap;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &faceViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view!");
        }
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_cubeMap;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;


    if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_skyboxView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    RenderToCubemap(m_hdrImage, m_hdrView->getHandle(), m_hdrSampler->getHandle(),
                    m_cubeMap, faceViews, m_cubeMapSize,
                    "shaders/cubemap.vert.spv", "shaders/cubemap.frag.spv");
    GenerateMipmaps(m_cubeMap, VK_FORMAT_R32G32B32A32_SFLOAT, m_cubeMapSize, m_cubeMapSize, mipLevels, 6);
    DebugLabel::NameImage(m_cubeMap, "Cubemap");

    for (const auto& view : faceViews) {
        vkDestroyImageView(m_device.device(), view, nullptr);
    }

}

void vov::HDRI::CreateDiffuseIrradianceMap() {
    // Create the irradiance cubemap (lower resolution)
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = m_diffuseIrradianceMapSize;
    imageInfo.extent.width = m_diffuseIrradianceMapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6; // 6 faces for cubemap
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(m_device.allocator(), &imageInfo, &allocInfo, &m_diffuseIrradianceMap, &m_diffuseIrradianceAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create diffuse irradiance image with VMA!");
    }

    // Create image views for each face
    std::array<VkImageView, 6> faceViews{};
    for (uint32_t face = 0; face < 6; ++face) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_diffuseIrradianceMap;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &faceViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create diffuse irradiance image view!");
        }
    }

    // Create the cubemap view for the irradiance map
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_diffuseIrradianceMap;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(m_device.device(), &viewInfo, nullptr, &m_diffuseIrradianceView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create diffuse irradiance cubemap view!");
    }

    // Render to the cubemap using the special diffuse irradiance fragment shader
    // RenderToCubemap(m_diffuseIrradianceMap, faceViews, m_diffuseIrradianceMapSize, "shaders/cubemap.vert.spv", "shaders/diffuseIrradiance.frag.spv");
    RenderToCubemap(m_cubeMap, m_skyboxView, m_hdrSampler->getHandle(),
                    m_diffuseIrradianceMap, faceViews, m_diffuseIrradianceMapSize,
                    "shaders/cubemap.vert.spv", "shaders/diffuseIrradiance.frag.spv");
    DebugLabel::NameImage(m_diffuseIrradianceMap, "Diffuse Irradiance Map");

    //Mipmaps
    const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(m_diffuseIrradianceMapSize))) + 1;
    //GenerateMipmaps(m_diffuseIrradianceMap, VK_FORMAT_R32G32B32A32_SFLOAT, m_diffuseIrradianceMapSize, m_diffuseIrradianceMapSize, mipLevels, 6);

    // Clean up temporary face views
    for (const auto& view : faceViews) {
        vkDestroyImageView(m_device.device(), view, nullptr);
    }
}

void vov::HDRI::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_device.getPhysicalDevice(), imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = m_device.beginSingleTimeCommands();

    // First, transition the entire image to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier initialBarrier{};
    initialBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    initialBarrier.image = image;
    initialBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    initialBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    initialBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    initialBarrier.subresourceRange.baseArrayLayer = 0;
    initialBarrier.subresourceRange.layerCount = layerCount;
    initialBarrier.subresourceRange.baseMipLevel = 0;
    initialBarrier.subresourceRange.levelCount = mipLevels;
    initialBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    initialBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    initialBarrier.srcAccessMask = 0;
    initialBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &initialBarrier);

    for (uint32_t face = 0; face < layerCount; ++face) {
        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        // The base mip level is already in TRANSFER_DST_OPTIMAL from the initial barrier
        for (uint32_t i = 1; i < mipLevels; ++i) {
            // Transition mip level i-1 to TRANSFER_SRC_OPTIMAL
            VkImageMemoryBarrier barrierBeforeBlit{};
            barrierBeforeBlit.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierBeforeBlit.image = image;
            barrierBeforeBlit.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierBeforeBlit.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierBeforeBlit.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierBeforeBlit.subresourceRange.baseArrayLayer = face;
            barrierBeforeBlit.subresourceRange.layerCount = 1;
            barrierBeforeBlit.subresourceRange.baseMipLevel = i - 1;
            barrierBeforeBlit.subresourceRange.levelCount = 1;
            barrierBeforeBlit.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrierBeforeBlit.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrierBeforeBlit.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrierBeforeBlit.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrierBeforeBlit);

            // Blit from mip level i-1 to i
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = face;
            blit.srcSubresource.layerCount = 1;

            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1
            };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = face;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            // Transition mip level i-1 to SHADER_READ_ONLY_OPTIMAL
            VkImageMemoryBarrier barrierAfterBlit{};
            barrierAfterBlit.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrierAfterBlit.image = image;
            barrierAfterBlit.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierAfterBlit.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrierAfterBlit.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrierAfterBlit.subresourceRange.baseArrayLayer = face;
            barrierAfterBlit.subresourceRange.layerCount = 1;
            barrierAfterBlit.subresourceRange.baseMipLevel = i - 1;
            barrierAfterBlit.subresourceRange.levelCount = 1;
            barrierAfterBlit.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrierAfterBlit.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrierAfterBlit.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrierAfterBlit.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrierAfterBlit);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition the last mip level to SHADER_READ_ONLY_OPTIMAL
        VkImageMemoryBarrier finalMipBarrier{};
        finalMipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        finalMipBarrier.image = image;
        finalMipBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        finalMipBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        finalMipBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        finalMipBarrier.subresourceRange.baseArrayLayer = face;
        finalMipBarrier.subresourceRange.layerCount = 1;
        finalMipBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
        finalMipBarrier.subresourceRange.levelCount = 1;
        finalMipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        finalMipBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        finalMipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        finalMipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &finalMipBarrier);
    }

    m_device.endSingleTimeCommands(commandBuffer);
}

void vov::HDRI::TransitionImageLayout(VkCommandBuffer cmd,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           uint32_t mipLevels,
                           uint32_t arrayLayers) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = arrayLayers;

    // Adjust access masks depending on transition
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    const VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    const VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(
        cmd,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}
