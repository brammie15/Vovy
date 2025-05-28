#ifndef WINDOW_H
#define WINDOW_H
#include <cstdint>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <unordered_map>
#include <glm/vec2.hpp>

#include "GLFW/glfw3.h"

namespace vov {
    class Window {
    public:
        Window(uint32_t width, uint32_t height, const std::string& windowName);
        ~Window();

        [[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose(gWindow); }

        void CreateSurface(VkInstance instance, VkSurfaceKHR* surface);

        [[nodiscard]] bool wasWindowResized() const { return m_resized; }
        void resetWindowResized() { m_resized = false; }

        [[nodiscard]] VkExtent2D getExtent() const { return {static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)}; }

        void LockCursor();
        void UnlockCursor();
        [[nodiscard]] bool isCursorLocked() const { return m_cursorLocked; }

        static GLFWwindow* gWindow;

        [[nodiscard]] glm::vec2 getMousePosition() const;

        glm::vec2 getMouseDelta();

        void PollInput();
        [[nodiscard]] bool isKeyDown(int key) const;
        [[nodiscard]] bool isKeyUp(int key) const;
        [[nodiscard]] bool isKeyPressed(int key);

        [[nodiscard]] bool isMouseButtonDown(int button) const;
        [[nodiscard]] bool isMouseButtonUp(int button) const;
        [[nodiscard]] bool isMouseButtonPressed(int button);

        [[nodiscard]] uint32_t getWidth() const { return m_width; }
        [[nodiscard]] uint32_t getHeight() const { return m_height; }

        void StopSplash();

    private:
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        void LoadGamepadMappings(const std::string& filename);

        void ShowSplash();

        uint32_t m_width;
        uint32_t m_height;

        glm::vec2 m_lastMousePos{};
        glm::vec2 m_currentMousePos{};

        bool m_resized = false;
        std::string m_windowName;

        bool m_cursorLocked = false;
        bool m_shouldToggleCursor = false;

        std::unordered_map<int, bool> m_currentKeys;
        std::unordered_map<int, bool> m_previousKeys;

        std::unordered_map<int, bool> m_currentMouseButtons;
        std::unordered_map<int, bool> m_previousMouseButtons;

        GLFWwindow* m_splashWindow = nullptr;
        GLuint m_splashTexture = 0;
        bool m_showingSplash = false;
    };
}


#endif //WINDOW_H
