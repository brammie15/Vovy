#include "VWindow.h"

#include <stdexcept>

GLFWwindow* VWindow::gWindow = nullptr;

VWindow::VWindow(uint32_t width, uint32_t height, const std::string& windowName): m_width(width), m_height(height),
                                                                                       m_windowName(windowName) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    gWindow = glfwCreateWindow(m_width, m_height,m_windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(gWindow, this);
    glfwSetFramebufferSizeCallback(gWindow, framebufferResizeCallback);

    if (!gWindow) {
        throw std::runtime_error("Cannot make window!");
    }
}

VWindow::~VWindow() {
    glfwDestroyWindow(gWindow);
    glfwTerminate();
}

void VWindow::CreateSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if(instance == VK_NULL_HANDLE){
        throw std::runtime_error("Shit's fucked");
    }

    if (glfwCreateWindowSurface(instance, gWindow, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void VWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto MyWindow = reinterpret_cast<VWindow*>(glfwGetWindowUserPointer(window));
    MyWindow->m_resized = true;
    MyWindow->m_width = width;
    MyWindow->m_height = height;
}
