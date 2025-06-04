cmake_minimum_required(VERSION 3.30)
project(Vovy)

set(CMAKE_CXX_STANDARD 20)

include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src)

set(SRC_ROOT ${CMAKE_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME}
        ${SRC_ROOT}/main.cpp
        ${SRC_ROOT}/VApp.h  ${SRC_ROOT}/VApp.cpp
        ${SRC_ROOT}/Core/Device.h ${SRC_ROOT}/Core/Device.cpp
        ${SRC_ROOT}/Core/Window.h ${SRC_ROOT}/Core/Window.cpp

        ${SRC_ROOT}/Descriptors/DescriptorPool.h ${SRC_ROOT}/Descriptors/DescriptorPool.cpp
        ${SRC_ROOT}/Descriptors/DescriptorWriter.h ${SRC_ROOT}/Descriptors/DescriptorWriter.cpp
        ${SRC_ROOT}/Descriptors/DescriptorSetLayout.h ${SRC_ROOT}/Descriptors/DescriptorSetLayout.cpp

        ${SRC_ROOT}/Rendering/Pipeline.h ${SRC_ROOT}/Rendering/Pipeline.cpp
        ${SRC_ROOT}/Rendering/Renderer.h ${SRC_ROOT}/Rendering/Renderer.cpp
        ${SRC_ROOT}/Rendering/Swapchain.h ${SRC_ROOT}/Rendering/Swapchain.cpp
        ${SRC_ROOT}/Rendering/RenderTexture.h ${SRC_ROOT}/Rendering/RenderTexture.cpp

#        ${SRC_ROOT}/Rendering/RenderSystems/GameObjectRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/GameObjectRenderSystem.cpp
        ${SRC_ROOT}/Rendering/RenderSystems/ImguiRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/ImGuiRenderSystem.cpp
        ${SRC_ROOT}/Rendering/RenderSystems/LineRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/LineRenderSystem.cpp
#        ${SRC_ROOT}/Rendering/RenderSystems/ShadowRenderSystem.h ${SRC_ROOT}/Rendering/RenderSystems/ShadowRenderSystem.cpp

        ${SRC_ROOT}/Rendering/Passes/DepthPrePass.h ${SRC_ROOT}/Rendering/Passes/DepthPrePass.cpp
        ${SRC_ROOT}/Rendering/Passes/GeometryPass.h ${SRC_ROOT}/Rendering/Passes/GeometryPass.cpp
        ${SRC_ROOT}/Rendering/Passes/LightingPass.h ${SRC_ROOT}/Rendering/Passes/LightingPass.cpp
        ${SRC_ROOT}/Rendering/Passes/BlitPass.h ${SRC_ROOT}/Rendering/Passes/BlitPass.cpp
        ${SRC_ROOT}/Rendering/Passes/SelectPass.h ${SRC_ROOT}/Rendering/Passes/SelectPass.cpp
        ${SRC_ROOT}/Rendering/Passes/ShadowPass.h ${SRC_ROOT}/Rendering/Passes/ShadowPass.cpp
        ${SRC_ROOT}/Rendering/Passes/LinePass.h ${SRC_ROOT}/Rendering/Passes/LinePass.cpp


        ${SRC_ROOT}/Resources/Buffer.h ${SRC_ROOT}/Resources/Buffer.cpp
        ${SRC_ROOT}/Resources/Image.h ${SRC_ROOT}/Resources/Image.cpp
        ${SRC_ROOT}/Resources/GeoBuffer.h ${SRC_ROOT}/Resources/GeoBuffer.cpp
        ${SRC_ROOT}/Resources/HDRI.h ${SRC_ROOT}/Resources/HDRI.cpp

        ${SRC_ROOT}/Resources/Image/ImageView.h ${SRC_ROOT}/Resources/Image/ImageView.cpp
        ${SRC_ROOT}/Resources/Image/Sampler.h ${SRC_ROOT}/Resources/Image/Sampler.cpp

        ${SRC_ROOT}/Scene/Mesh.h ${SRC_ROOT}/Scene/Mesh.cpp
        ${SRC_ROOT}/Scene/Model.h ${SRC_ROOT}/Scene/Model.cpp
        ${SRC_ROOT}/Scene/GameObject.h ${SRC_ROOT}/Scene/GameObject.cpp
        ${SRC_ROOT}/Scene/Transform.h ${SRC_ROOT}/Scene/Transform.cpp
        ${SRC_ROOT}/Scene/Scene.h ${SRC_ROOT}/Scene/Scene.cpp

        ${SRC_ROOT}/Scene/Lights/DirectionalLight.h ${SRC_ROOT}/Scene/Lights/DirectionalLight.cpp
        ${SRC_ROOT}/Scene/Lights/PointLight.h ${SRC_ROOT}/Scene/Lights/PointLight.cpp

        ${SRC_ROOT}/Utils/FrameContext.h
        ${SRC_ROOT}/Utils/stb_image.h
        ${SRC_ROOT}/Utils/Camera.h ${SRC_ROOT}/Utils/Camera.cpp
        ${SRC_ROOT}/Utils/ResourceManager.h ${SRC_ROOT}/Utils/ResourceManager.cpp
        ${SRC_ROOT}/Utils/Singleton.h
        ${SRC_ROOT}/Utils/Timer.h ${SRC_ROOT}/Utils/Timer.cpp
        ${SRC_ROOT}/Utils/DeltaTime.h ${SRC_ROOT}/Utils/DeltaTime.cpp
        ${SRC_ROOT}/Utils/BezierCurves.h ${SRC_ROOT}/Utils/BezierCurves.cpp
        ${SRC_ROOT}/Utils/DebugLabel.h ${SRC_ROOT}/Utils/DebugLabel.cpp
        ${SRC_ROOT}/Utils/AABB.h ${SRC_ROOT}/Utils/AABB.cpp
        ${SRC_ROOT}/Utils/LineManager.h ${SRC_ROOT}/Utils/LineManager.cpp

        ${SRC_ROOT}/Utils/External/imguizmo_config.h
        ${SRC_ROOT}/Utils/External/imguizmo_quat.h ${SRC_ROOT}/Utils/External/imguizmo_quat.cpp
        ${SRC_ROOT}/Utils/External/imGuIZMOquat.h ${SRC_ROOT}/Utils/External/imGuIZMOquat.cpp
        ${SRC_ROOT}/Utils/External/vGizmo3D.h
        ${SRC_ROOT}/Utils/External/vGizmo3D_config.h
        ${SRC_ROOT}/Utils/External/vgMath.h
        ${SRC_ROOT}/Utils/External/vgMath_config.h

)

if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer for MSVC Debug build")

#    target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
#    target_link_options(${PROJECT_NAME} PRIVATE /fsanitize=address)
endif()

# Suppress deprecation warnings in external projects
if(POLICY CMP0120)
    cmake_policy(SET CMP0120 NEW)
endif()

# Ignore deprecation warnings during FetchContent
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "Suppress CMake developer warnings")

find_package(Vulkan REQUIRED)

include(cmake/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)

include(cmake/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PUBLIC)

include(cmake/LinkVMA.cmake)
LinkVma(${PROJECT_NAME} PUBLIC)

include(cmake/LinkASSIMP.cmake)
LinkASSIMP(${PROJECT_NAME} PUBLIC)

include(cmake/LinkGli.cmake)
LinkGli(${PROJECT_NAME} PUBLIC)

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




target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan ${VLD_LIBRARY} opengl32)
target_include_directories(${PROJECT_NAME} PUBLIC ${VLD_INCLUDE_DIR})


include(cmake/FetchResources.cmake)
fetch_resources_if_missing()



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
            COMMAND ${GLSL_VALIDATOR} -V -g ${GLSL} -o ${SPIRV}
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
