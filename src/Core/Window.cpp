#include "Window.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "Utils/stb_image.h"

namespace vov {
    GLFWwindow* Window::gWindow = nullptr;

    Window::Window(uint32_t width, uint32_t height, const std::string& windowName): m_width(width), m_height(height),
                                                                                           m_windowName(windowName) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        gWindow = glfwCreateWindow(static_cast<int>(m_width), static_cast<int>(m_height),m_windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(gWindow, this);
        glfwSetFramebufferSizeCallback(gWindow, framebufferResizeCallback);
        // glfwSetKeyCallback(gWindow, keyCallback);
        glfwSetCursorPosCallback(gWindow, [](GLFWwindow* window, double x, double y) {
            const auto MyWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
            MyWindow->m_currentMousePos.x = static_cast<float>(x);
            MyWindow->m_currentMousePos.y = static_cast<float>(y);
        });

        glfwMaximizeWindow(gWindow);

        GLFWimage images[1];
        images[0].pixels = stbi_load("resources/vovy_logo.png", &images[0].width, &images[0].height, 0, 4); //rgba channels
        glfwSetWindowIcon(gWindow, 1, images);
        stbi_image_free(images[0].pixels);

        // glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (!gWindow) {
            throw std::runtime_error("Cannot make window!");
        }

        LoadGamepadMappings("resources/gamecontrollerdb.txt");

        if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
            std::cout << "Joystick detected: " << glfwGetJoystickName(GLFW_JOYSTICK_1) << std::endl;
        } else {
            std::cerr << "No joystick detected!" << std::endl;
        }

        for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++) {
            m_currentMouseButtons[i] = false;
            m_previousMouseButtons[i] = false;
        }
    }

    Window::~Window() {
        glfwDestroyWindow(gWindow);
        glfwTerminate();
    }

    bool Window::isMouseButtonDown(int button) const {
        const int state = glfwGetMouseButton(gWindow, button);
        return state == GLFW_PRESS;
    }

    bool Window::isMouseButtonUp(int button) const {
        const int state = glfwGetMouseButton(gWindow, button);
        return state == GLFW_RELEASE;
    }

    bool Window::isMouseButtonPressed(int button) {
        const int state = glfwGetMouseButton(gWindow, button);

        bool wasPressed = m_currentMouseButtons[button];

        if (state == GLFW_PRESS && !wasPressed) {
            m_currentMouseButtons[button] = true;
            return true;
        } else if (state == GLFW_RELEASE) {
            m_currentMouseButtons[button] = false;
        }

        return false;
    }

    void Window::CreateSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if(instance == VK_NULL_HANDLE){
            throw std::runtime_error("Shit's fucked");
        }

        if (glfwCreateWindowSurface(instance, gWindow, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void Window::LockCursor() {
        m_cursorLocked = true;
        glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Window::UnlockCursor() {
        m_cursorLocked = false;
        glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


    glm::vec2 Window::getMousePosition() const {
        double xpos, ypos;
        glfwGetCursorPos(gWindow, &xpos, &ypos);
        return {static_cast<float>(xpos), static_cast<float>(ypos)};
    }

    glm::vec2 Window::getMouseDelta() {
        m_currentMousePos = getMousePosition();
        const glm::vec2 delta = m_currentMousePos - m_lastMousePos;
        m_lastMousePos = m_currentMousePos;
        return delta;
    }

    void Window::PollInput() {
        glfwPollEvents();
        m_previousKeys = m_currentKeys;
        m_previousMouseButtons = m_currentMouseButtons;
    }

    bool Window::isKeyDown(int key) const {
        const int state = glfwGetKey(gWindow, key);
        return state == GLFW_PRESS;
    }

    bool Window::isKeyUp(int key) const {
        const int state = glfwGetKey(gWindow, key);
        return state == GLFW_RELEASE;
    }

    bool Window::isKeyPressed(int key) {
        const int state = glfwGetKey(gWindow, key);

        bool wasPressed = m_currentKeys[key];

        if (state == GLFW_PRESS && !wasPressed) {
            m_currentKeys[key] = true;
            return true;
        } else if (state == GLFW_RELEASE) {
            m_currentKeys[key] = false;
        }

        return false;
    }

    void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        const auto MyWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        MyWindow->m_resized = true;
        MyWindow->m_width = width;
        MyWindow->m_height = height;
    }

    void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        const auto MyWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            MyWindow->m_currentKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            MyWindow->m_currentKeys[key] = false;
        }
    }

    void Window::LoadGamepadMappings(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            if (glfwUpdateGamepadMappings(line.c_str()) == GLFW_FALSE) {
                std::cerr << "Failed to load gamepad mapping: " << line << std::endl;
            }
        }
    }
}