#ifndef FWINDOW_H
#define FWINDOW_H
#include <cstdint>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <glm/vec2.hpp>

#include "GLFW/glfw3.h"

class VWindow {
public:
    VWindow(uint32_t width, uint32_t height, const std::string& windowName);
    ~VWindow();

    [[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose(gWindow); }

    void CreateSurface(VkInstance instance, VkSurfaceKHR* surface);

    [[nodiscard]] bool wasWindowResized() const { return m_resized; }
    void resetWindowResized(){ m_resized = false; }

    [[nodiscard]] VkExtent2D getExtent() const{ return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }

    void LockCursor();
    void UnlockCursor();
    [[nodiscard]] bool isCursorLocked() const { return m_cursorLocked; }

    static GLFWwindow* gWindow;

    auto getMousePosition() const -> glm::vec2;

    glm::vec2 getMouseDelta();

    [[nodiscard]] uint32_t getWidth() const { return m_width; }
    [[nodiscard]] uint32_t getHeight() const { return m_height; }
private:
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void LoadGamepadMappins(const std::string& filename);

    uint32_t m_width;
    uint32_t m_height;

    glm::vec2 m_lastMousePos;
    glm::vec2 m_currentMousePos;

    bool m_resized = false;
    std::string m_windowName;

    bool m_cursorLocked = false;
    bool m_shouldToggleCursor = false;
};

#endif //FWINDOW_H
