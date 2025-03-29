#include "VCamera.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>

#include "VWindow.h"
#include "GLFW/glfw3.h"


VCamera::VCamera(const glm::vec3& position, const glm::vec3& up): m_position{position}, m_up{up} {
}

void VCamera::Update(float deltaTime) {
    float MovementSpeed = 1.0f;
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
    if (glfwGetKey(VWindow::gWindow,GLFW_KEY_Q) == GLFW_PRESS) {
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

    // Clamp pitch to prevent camera flip
    totalPitch = glm::clamp(totalPitch, -glm::half_pi<float>(), glm::half_pi<float>());

    // Create rotation matrix based on yaw and pitch
    glm::mat4 yawMatrix   = glm::rotate(glm::mat4(1.0f), -totalYaw, glm::vec3(0, 1, 0));
    glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), totalPitch, glm::vec3(1, 0, 0));

    glm::mat4 rotationMatrix = yawMatrix * pitchMatrix;

    // Update camera vectors
    m_forward = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0));
    m_right   = glm::normalize(glm::cross(m_forward, glm::vec3(0, 1, 0)));
    m_up      = glm::normalize(glm::cross(m_right, m_forward));

    CalculateProjectionMatrix();
    CalculateViewMatrix();
}

void VCamera::CalculateViewMatrix() {
    auto test = glm::lookAt(glm::vec3(2.f, 0, 0), glm::vec3(1.0f, 0.f, 0.f), glm::vec3(0.0f, 0.0f, 1.0f));
    // std::cout << glm::to_string(test) << std::endl;

    m_viewMatrix = glm::lookAt(m_position, m_position + m_forward, m_up);
    // m_viewMatrix = glm::lookAt(m_position, glm::vec3(1.0f, 0.f, 0.f), glm::vec3{0.f, 0.f, 1.f});

    // std::cout << std::endl;
    //
    // std::cout << "m_viewMatrix: " << glm::to_string(m_viewMatrix) << std::endl;
    //
    // std::cout << std::endl;

    // __debugbreak();

    // m_invMatrix = glm::inverse(m_viewMatrix);

    // m_right = glm::vec3(m_viewMatrix[0]);
    // m_up = glm::vec3(m_viewMatrix[1]);
    // m_forward = glm::vec3(m_viewMatrix[2]);
}

void VCamera::CalculateProjectionMatrix() {
    m_projectionMatrix = glm::perspective(glm::radians(fovAngle), m_aspectRatio, m_zNear, m_zFar);
}
