#ifndef FWINDOW_H
#define FWINDOW_H
#include <cstdint>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class VWindow {
public:
    VWindow(uint32_t width, uint32_t height, const std::string& windowName);

    ~VWindow();

    [[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose(m_window); }

    void CreateSurface(VkInstance instance, VkSurfaceKHR* surface);

    [[nodiscard]] bool wasWindowResized() const { return m_resized; }
    void resetWindowResized(){ m_resized = false; }

    [[nodiscard]] VkExtent2D getExtent() const{ return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }

private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    GLFWwindow* m_window{nullptr};

    uint32_t m_width;
    uint32_t m_height;

    bool m_resized = false;
    std::string m_windowName;
};

#endif //FWINDOW_H
