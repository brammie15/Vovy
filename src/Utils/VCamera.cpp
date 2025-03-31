#include "VCamera.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Core/VWindow.h"
#include "GLFW/glfw3.h"


VCamera::VCamera(const glm::vec3& position, const glm::vec3& up): m_position{position}, m_up{up} {
}

void VCamera::Update(float deltaTime) {
    const auto MyWindow = static_cast<VWindow*>(glfwGetWindowUserPointer(VWindow::gWindow));
    glm::vec2 deltaMousePos = MyWindow->getMouseDelta();

    float sensitivity = 0.002f;
    // totalYaw -= deltaMousePos.x * sensitivity;
    // totalPitch += deltaMousePos.y * sensitivity;

    totalPitch = glm::clamp(totalPitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);

    float MovementSpeed = 2.0f;
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        MovementSpeed *= 2.0f;
    }

    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_W) == GLFW_PRESS) {
        m_position += m_forward * deltaTime * MovementSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_A) == GLFW_PRESS) {
        m_position -= m_right * deltaTime * MovementSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_S) == GLFW_PRESS) {
        m_position -= m_forward * deltaTime * MovementSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_D) == GLFW_PRESS) {
        m_position += m_right * deltaTime * MovementSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_E) == GLFW_PRESS) {
        m_position += m_up * deltaTime * MovementSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_Q) == GLFW_PRESS) {
        m_position -= m_up * deltaTime * MovementSpeed;
    }

    // Looking around with arrow keys
    float lookSpeed = 2.0f * glm::radians(1.f);
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_UP) == GLFW_PRESS) {
        totalPitch += lookSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_DOWN) == GLFW_PRESS) {
        totalPitch -= lookSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_LEFT) == GLFW_PRESS) {
        totalYaw -= lookSpeed;
    }
    if (glfwGetKey(VWindow::gWindow, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        totalYaw += lookSpeed;
    }
    totalPitch = glm::clamp(totalPitch, -glm::half_pi<float>(), glm::half_pi<float>());

    glm::mat4 yawMatrix   = glm::rotate(glm::mat4(1.0f), -totalYaw, glm::vec3(0, 1, 0));
    glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), totalPitch, glm::vec3(0, 0, 1));

    glm::mat4 rotationMatrix = yawMatrix * pitchMatrix;

    m_forward = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0));
    m_right   = glm::normalize(glm::cross(m_forward, glm::vec3(0, 1, 0)));
    m_up      = glm::normalize(glm::cross(m_right, m_forward));

    CalculateProjectionMatrix();
    CalculateViewMatrix();
}

void VCamera::CalculateViewMatrix() {
    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
    m_invMatrix = glm::inverse(m_viewMatrix);


    // m_right = glm::vec3(m_viewMatrix[0]);
    // m_up = glm::vec3(m_viewMatrix[1]);
    // m_forward = glm::vec3(m_viewMatrix[2]);
}

void VCamera::CalculateProjectionMatrix() {
    m_projectionMatrix = glm::perspective(glm::radians(fovAngle), m_aspectRatio, m_zNear, m_zFar);
}
