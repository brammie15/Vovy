cmake_minimum_required(VERSION 3.30)
project(Vovy)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME}
        src/VDevice.cpp
        src/VDevice.h
        src/VWindow.cpp
        src/VWindow.h
        src/VApp.cpp
        src/VApp.h
        src/main.cpp
        src/VSwapchain.cpp
        src/VSwapchain.h
        src/VPipeline.cpp
        src/VPipeline.h
        src/VBuffer.cpp
        src/VBuffer.h
        src/VModel.cpp
        src/VModel.h
        src/VGameObject.cpp
        src/VGameObject.h
        src/Descriptors/VDescriptorSetLayout.cpp
        src/Descriptors/VDescriptorSetLayout.h
        src/Descriptors/VDescriptorPool.cpp
        src/Descriptors/VDescriptorPool.h
        src/Descriptors/VDescriptorWriter.cpp
        src/Descriptors/VDescriptorWriter.h
        src/VRenderPass.cpp
        src/VRenderPass.h
        src/VRenderSystem.cpp
        src/VRenderSystem.h
        src/FrameContext.h
)
#
#add_executable(${PROJECT_NAME}
#        old_main.cpp
#)


find_package(Vulkan REQUIRED)

include(cmake/LinkGLFW.cmake)
LinkGLFW(${PROJECT_NAME} PRIVATE)

include(cmake/LinkGLM.cmake)
LinkGLM(${PROJECT_NAME} PUBLIC)

include(cmake/LinkVMA.cmake)
LinkVma(${PROJECT_NAME} PUBLIC)

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)


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

# Create a target for compiling shaders
add_custom_target(CompileShaders DEPENDS ${SPIRV_BINARY_FILES})

# Ensure the output shader directory exists
file(MAKE_DIRECTORY ${OUTPUT_SHADER_DIR})

# Copy compiled shaders to the output directory
foreach (SPV ${SPIRV_BINARY_FILES})
    get_filename_component(FILE_NAME ${SPV} NAME)
    add_custom_command(
            TARGET CompileShaders POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPV} ${OUTPUT_SHADER_DIR}/${FILE_NAME}
            COMMENT "Copying ${FILE_NAME} to output shaders directory"
    )
endforeach ()

add_dependencies(${PROJECT_NAME} CompileShaders)

