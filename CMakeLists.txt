cmake_minimum_required(VERSION 3.30)
project(Vovy)

set(CMAKE_CXX_STANDARD 20)

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)

set(SRC_ROOT ${CMAKE_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME}
        ${SRC_ROOT}/main.cpp
        ${SRC_ROOT}/VApp.h  ${SRC_ROOT}/VApp.cpp
        ${SRC_ROOT}/Core/VDevice.h ${SRC_ROOT}/Core/VDevice.cpp
        ${SRC_ROOT}/Core/VWindow.h ${SRC_ROOT}/Core/VWindow.cpp

        ${SRC_ROOT}/Descriptors/VDescriptorPool.h ${SRC_ROOT}/Descriptors/VDescriptorPool.cpp
        ${SRC_ROOT}/Descriptors/VDescriptorWriter.h ${SRC_ROOT}/Descriptors/VDescriptorWriter.cpp
        ${SRC_ROOT}/Descriptors/VDescriptorSetLayout.h ${SRC_ROOT}/Descriptors/VDescriptorSetLayout.cpp

        ${SRC_ROOT}/Rendering/VPipeline.h ${SRC_ROOT}/Rendering/VPipeline.cpp
        ${SRC_ROOT}/Rendering/VRenderPass.h ${SRC_ROOT}/Rendering/VRenderPass.cpp
        ${SRC_ROOT}/Rendering/VSwapchain.h ${SRC_ROOT}/Rendering/VSwapchain.cpp

        ${SRC_ROOT}/Rendering/RenderSystems/GameObjectRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/GameObjectRenderSystem.cpp
        ${SRC_ROOT}/Rendering/RenderSystems/ImguiRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/ImGuiRenderSystem.cpp
        ${SRC_ROOT}/Rendering/RenderSystems/LineRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/LineRenderSystem.cpp

        ${SRC_ROOT}/Resources/VBuffer.h ${SRC_ROOT}/Resources/VBuffer.cpp
        ${SRC_ROOT}/Resources/VImage.h ${SRC_ROOT}/Resources/VImage.cpp

        ${SRC_ROOT}/Scene/VMesh.h ${SRC_ROOT}/Scene/VMesh.cpp
        ${SRC_ROOT}/Scene/VModel.h ${SRC_ROOT}/Scene/VModel.cpp
        ${SRC_ROOT}/Scene/VGameObject.h ${SRC_ROOT}/Scene/VGameObject.cpp
        ${SRC_ROOT}/Scene/Transform.h ${SRC_ROOT}/Scene/Transform.cpp
        ${SRC_ROOT}/Scene/VScene.h ${SRC_ROOT}/Scene/VScene.cpp

        ${SRC_ROOT}/Utils/FrameContext.h
        ${SRC_ROOT}/Utils/stb_image.h
        ${SRC_ROOT}/Utils/VCamera.h ${SRC_ROOT}/Utils/VCamera.cpp
        ${SRC_ROOT}/Utils/ResourceManager.h ${SRC_ROOT}/Utils/ResourceManager.cpp
        ${SRC_ROOT}/Utils/Singleton.h
        ${SRC_ROOT}/Utils/Timer.h ${SRC_ROOT}/Utils/Timer.cpp
        ${SRC_ROOT}/Utils/DeltaTime.h ${SRC_ROOT}/Utils/DeltaTime.cpp
        ${SRC_ROOT}/Utils/BezierCurves.h ${SRC_ROOT}/Utils/BezierCurves.cpp
)
#
#add_executable(${PROJECT_NAME}
#        old_main.cpp
#)


if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer for MSVC Debug build")

#    target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
#    target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
endif()

find_package(Vulkan REQUIRED)

include(cmake/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)

include(cmake/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PUBLIC)

include(cmake/LinkVMA.cmake)
LinkVma(${PROJECT_NAME} PUBLIC)

include(cmake/LinkASSIMP.cmake)
LinkASSIMP(${PROJECT_NAME} PUBLIC)

include(cmake/LinkImgui.cmake)
LinkImGui(${PROJECT_NAME} PRIVATE)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan glfw)

include(cmake/LinkImGuizmo.cmake)
LinkImGuizmo(${PROJECT_NAME} PRIVATE)
target_link_libraries(imguizmo PRIVATE imgui)

find_package(VLD CONFIG)

if (NOT VLD_FOUND)
    message("VLD NOT FOUND!")
endif ()




#include(cmake/LinkTinyObjLoader.cmake)
#LinkTinyObjLoader(${PROJECT_NAME} PUBLIC)

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan ${VLD_LIBRARY})
target_include_directories(${PROJECT_NAME} PUBLIC ${VLD_INCLUDE_DIR})



find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

# Define shader source and destination directories
set(SHADER_SRC_DIR "${PROJECT_SOURCE_DIR}/shaders")
set(COMPILED_SHADER_DIR "${PROJECT_SOURCE_DIR}/compiled_shaders")
set(OUTPUT_SHADER_DIR "${CMAKE_BINARY_DIR}/shaders")

# Ensure the compiled shader directory exists
file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})

file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${SHADER_SRC_DIR}/*.frag"
        "${SHADER_SRC_DIR}/*.vert"
        "${SHADER_SRC_DIR}/*.comp"
)

set(SPIRV_BINARY_FILES "")

foreach (GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${COMPILED_SHADER_DIR}/${FILE_NAME}.spv")

    message(STATUS "Compiling shader: ${GLSL} -> ${SPIRV}")

    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL}
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach ()

add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})

file(MAKE_DIRECTORY ${OUTPUT_SHADER_DIR})

foreach (SPV ${SPIRV_BINARY_FILES})
    get_filename_component(FILE_NAME ${SPV} NAME)
    add_custom_command(
            TARGET CompileShaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPV} ${OUTPUT_SHADER_DIR}/${FILE_NAME}
            COMMENT "Copying ${FILE_NAME} to output shaders directory"
    )
endforeach ()

add_dependencies(${PROJECT_NAME} CompileShaders)


set(RESOURCE_SRC_DIR "${PROJECT_SOURCE_DIR}/resources")
set(RESOURCE_DST_DIR "${CMAKE_BINARY_DIR}/resources")

add_custom_target(CopyResources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different  ${RESOURCE_SRC_DIR} ${RESOURCE_DST_DIR}
        COMMENT "Copying resources folder to build directory"
)
add_dependencies(${PROJECT_NAME} CopyResources)
