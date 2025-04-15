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

#pragma region Keyboard Movement
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
    float lookSpeed = 1.0f * glm::radians(1.f);
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

    const glm::mat4 yawMatrix = glm::rotate(glm::mat4(1.0f), -totalYaw, glm::vec3(0, 1, 0));
    const glm::mat4 pitchMatrix = glm::rotate(glm::mat4(1.0f), totalPitch, glm::vec3(0, 0, 1));

    const glm::mat4 rotationMatrix = yawMatrix * pitchMatrix;

    m_forward = glm::vec3(rotationMatrix * glm::vec4(1, 0, 0, 0));
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0, 1, 0)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
#pragma endregion

#pragma region Mouse Looking
    static double lastMouseX = 0.0;
    static double lastMouseY = 0.0;
    static bool firstMouse = true;

    if (MyWindow->isCursorLocked()) {
        double mouseX, mouseY;
        glfwGetCursorPos(VWindow::gWindow, &mouseX, &mouseY);

        if (firstMouse) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            firstMouse = false;
        }

        float xOffset = static_cast<float>(mouseX - lastMouseX);
        float yOffset = static_cast<float>(lastMouseY - mouseY); // Reversed: y goes from bottom to top

        lastMouseX = mouseX;
        lastMouseY = mouseY;

        float sensitivity = 0.002f; // Adjust to your liking
        xOffset *= sensitivity;
        yOffset *= sensitivity;

        totalYaw += xOffset;
        totalPitch += yOffset;

        // Clamp pitch to avoid flipping
        totalPitch = glm::clamp(totalPitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);
    }
#pragma endregion

#pragma region Controller Movement

    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
        GLFWgamepadstate state;
        if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1) && glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
            float moveSpeed = MovementSpeed * deltaTime;
            const float rotSpeed = 1.5f * deltaTime;


            //TODO: god knows what this is
            auto Deadzone = [] (float value, float threshold = 0.1f) {
                if (fabs(value) < threshold)
                    return 0.0f;
                const float sign = (value > 0) ? 1.0f : -1.0f;
                return sign * (fabs(value) - threshold) / (1.0f - threshold);
            };

            //LT is x2 speed
            bool isLTPressed = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
            if (isLTPressed) {
                moveSpeed *= 3.0f;
            }

            const float lx = Deadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]);
            const float ly = Deadzone(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]);
            const float rx = Deadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
            const float ry = Deadzone(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);

            m_position += m_forward * (-ly * moveSpeed);
            m_position += m_right * (lx * moveSpeed);

            const float lTrigger = (state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] + 1.0f) / 2.0f;
            const float rTrigger = (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] + 1.0f) / 2.0f;
            const float vertical = (rTrigger - lTrigger);
            m_position += m_up * vertical * moveSpeed;

            totalYaw += rx * rotSpeed;
            totalPitch -= ry * rotSpeed;
        }
    }
#pragma endregion

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
