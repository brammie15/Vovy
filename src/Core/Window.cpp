#include "Window.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "Utils/stb_image.h"

namespace vov {
    GLFWwindow* Window::gWindow = nullptr;

    Window::Window(uint32_t width, uint32_t height, const std::string& windowName): m_width(width), m_height(height),
                                                                                    m_windowName(windowName) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        gWindow = glfwCreateWindow(static_cast<int>(m_width), static_cast<int>(m_height), m_windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(gWindow, this);
        glfwSetFramebufferSizeCallback(gWindow, framebufferResizeCallback);
        // glfwSetKeyCallback(gWindow, keyCallback);
        glfwSetCursorPosCallback(gWindow, [] (GLFWwindow* window, double x, double y) {
            const auto MyWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
            MyWindow->m_currentMousePos.x = static_cast<float>(x);
            MyWindow->m_currentMousePos.y = static_cast<float>(y);
        });

        glfwMaximizeWindow(gWindow);
        // ShowSplash();

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
        StopSplash();
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

        const bool wasPressed = m_currentMouseButtons[button];

        if (state == GLFW_PRESS && !wasPressed) {
            m_currentMouseButtons[button] = true;
            return true;
        } else if (state == GLFW_RELEASE) {
            m_currentMouseButtons[button] = false;
        }

        return false;
    }

    void Window::CreateSurface(VkInstance instance, VkSurfaceKHR* surface) {
        if (instance == VK_NULL_HANDLE) {
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

        const bool wasPressed = m_currentKeys[key];

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

    void Window::ShowSplash() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        m_splashWindow = glfwCreateWindow(640, 360, "Loading...", nullptr, nullptr);
        if (!m_splashWindow) {
            std::cerr << "Failed to create splash screen window.\n";
            return;
        }

        glfwMakeContextCurrent(m_splashWindow);

        int width, height, channels;
        unsigned char* pixels = stbi_load("resources/splash.jpg", &width, &height, &channels, 4);
        if (!pixels) {
            std::cerr << "Failed to load splash.jpg\n";
            glfwDestroyWindow(m_splashWindow);
            m_splashWindow = nullptr;
            return;
        }

        // glfwMaximizeWindow(m_splashWindow);
        // glfwFocusWindow(m_splashWindow);

        glGenTextures(1, &m_splashTexture);
        glBindTexture(GL_TEXTURE_2D, m_splashTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(pixels);

        m_showingSplash = true;

        // Start a simple render loop in a separate thread (optional)
        std::thread([this]() {
            while (m_showingSplash && !glfwWindowShouldClose(m_splashWindow)) {
                glfwMakeContextCurrent(m_splashWindow);
                int winW, winH;
                glfwGetFramebufferSize(m_splashWindow, &winW, &winH);
                glViewport(0, 0, winW, winH);
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, m_splashTexture);

                glBegin(GL_QUADS);
                glTexCoord2f(0.f, 1.f);
                glVertex2f(-1.f, -1.f);
                glTexCoord2f(1.f, 1.f);
                glVertex2f(1.f, -1.f);
                glTexCoord2f(1.f, 0.f);
                glVertex2f(1.f, 1.f);
                glTexCoord2f(0.f, 0.f);
                glVertex2f(-1.f, 1.f);
                glEnd();

                glfwSwapBuffers(m_splashWindow);
                glfwPollEvents();

                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
            }
        }).detach();
    }

    void Window::StopSplash() {
        m_showingSplash = false;

        if (m_splashTexture != 0) {
            glDeleteTextures(1, &m_splashTexture);
            m_splashTexture = 0;
        }

        if (m_splashWindow != nullptr) {
            glfwDestroyWindow(m_splashWindow);
            m_splashWindow = nullptr;
        }
    }

}
