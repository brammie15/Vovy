include(FetchContent)

macro(LinkImGuizmo TARGET ACCESS)
    FetchContent_Declare(
            imguizmo
            GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    )

    FetchContent_MakeAvailable(imguizmo)

    add_library(imguizmo STATIC
            ${imguizmo_SOURCE_DIR}/GraphEditor.cpp
            ${imguizmo_SOURCE_DIR}/ImCurveEdit.cpp
            ${imguizmo_SOURCE_DIR}/ImGradient.cpp
            ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
            ${imguizmo_SOURCE_DIR}/ImSequencer.cpp
    )
#
#    target_sources(imgui PRIVATE
#            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
#            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
#    )
#    # Link ImGui to your target
    target_link_libraries(${TARGET} ${ACCESS} imguizmo)
#
#    # Include ImGui headers
    target_include_directories(imguizmo PUBLIC ${imguizmo_SOURCE_DIR})
endmacro()