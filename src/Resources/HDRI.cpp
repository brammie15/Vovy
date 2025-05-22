#include "HDRI.h"
#include "utils/stb_image.h"
#include <stdexcept>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Buffer.h"
#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Descriptors/DescriptorWriter.h"
#include "Rendering/Pipeline.h"
#include "Rendering/Renderer.h"
#include "Utils/DebugLabel.h"

vov::HDRI::HDRI(Device& deviceRef): m_device(deviceRef) {
}

vov::HDRI::~HDRI() {
    vmaDestroyImage(m_device.allocator(), m_hdrImage, m_allocation);
    vmaDestroyImage(m_device.allocator(), m_cubeMap, m_cubeMapAllocation);
    vkDestroyImageView(m_device.device(), m_skyboxView, nullptr);
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

    m_hdrView = std::make_unique<ImageView>(m_device, m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    m_device.TransitionImageLayout(m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    m_device.copyBufferToImage(stagingBuffer.getBuffer(), m_hdrImage, width, height);

    m_hdrSampler = std::make_unique<Sampler>(m_device, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1);

    DebugLabel::NameImage(m_hdrImage, filename);

    m_device.TransitionImageLayout(m_hdrImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
}

void vov::HDRI::RenderToCubemap(VkImage vk_image, std::array<VkImageView, 6> faceViews, uint32_t size, const std::string& vertPath, const std::string& fragPath) {
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
    barrier.image = vk_image;
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

    std::array <VkDescriptorSetLayout, 1> descriptorSetLayouts = {
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

    std::unique_ptr<Pipeline> pipeline = std::make_unique<Pipeline>(
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
    imageInfo.imageView = m_hdrView->getHandle();
    imageInfo.sampler = m_hdrSampler->getHandle();

    VkDescriptorSet descriptorSet;

    DescriptorWriter writer(*descriptorSetLayout, *descriptorPool);
    writer.writeImage(0, &imageInfo);
    writer.build(descriptorSet);


    for (uint32_t face{ 0 }; face < 6; ++face) {
        PushConstants pc{};
        pc.view = viewMatrices[face];
        pc.projection = projection;

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

        // Bind pipeline and descriptor sets
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

        // Draw unit cube
        vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

        vov::vkCmdEndRenderingKHR(cmdBuffer);
    }

    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Submit command buffer
    m_device.endSingleTimeCommands(cmdBuffer);

    for (auto& view : faceViews) {
        vkDestroyImageView(m_device.device(), view, nullptr);
    }
    vkDestroyPipelineLayout(m_device.device(), pipelineLayout, nullptr);

}

void vov::HDRI::CreateCubeMap() {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.height = m_cubeMapSize;
    imageInfo.extent.width = m_cubeMapSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6; // 6 faces for cubemap
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

    RenderToCubemap(m_cubeMap, faceViews, m_cubeMapSize, "shaders/cubemap.vert.spv", "shaders/cubemap.frag.spv");
    DebugLabel::NameImage(m_cubeMap, "Cubemap");

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
    RenderToCubemap(m_diffuseIrradianceMap, faceViews, m_diffuseIrradianceMapSize, "shaders/cubemap.vert.spv", "shaders/diffuseIrradiance.frag.spv");

    DebugLabel::NameImage(m_diffuseIrradianceMap, "Diffuse Irradiance Map");

    // Clean up temporary face views
    for (auto& view : faceViews) {
        vkDestroyImageView(m_device.device(), view, nullptr);
    }
}

void vov::HDRI::RenderToCubemap(VkImage inputImage, VkImageView inputView, VkSampler sampler, VkImage outputImage, std::array<VkImageView, 6> faceViews, VkShaderModule fragShader) {
}
