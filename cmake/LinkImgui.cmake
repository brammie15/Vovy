include(FetchContent)

macro(LinkImGui TARGET ACCESS)
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking # Use the 'docking' branch
    )

    # Fetch and make ImGui available
    FetchContent_MakeAvailable(imgui)

    # Add the ImGui sources to your project
    add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )

    target_sources(imgui PRIVATE
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )
    # Link ImGui to your target
    target_link_libraries(${TARGET} ${ACCESS} imgui)

    # Include ImGui headers
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
endmacro()