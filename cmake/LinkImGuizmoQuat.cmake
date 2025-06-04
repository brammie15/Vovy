include(FetchContent)

macro(LinkImQuat TARGET ACCESS)
    FetchContent_Declare(
            imquat
            GIT_REPOSITORY https://github.com/BrutPitt/imGuIZMO.quat.git

    )

    FetchContent_MakeAvailable(imquat)

    add_library(imquat STATIC
            ${imquat_SOURCE_DIR}/imguizmo_quat/imGuIZMOquat.cpp
#            ${imquat_SOURCE_DIR}/imguizmo_quat/imguizmo_quat.cpp
#            ${imquat_SOURCE_DIR}/imguizmo_quat/imguizmo_quat.h
#
#            ${imquat_SOURCE_DIR}/imguizmo_quat/imguizmo_config.h
#
#            ${imquat_SOURCE_DIR}/imguizmo_quat/imGuIZMOquat.h
#            ${imquat_SOURCE_DIR}/imguizmo_quat/vGizmo3D.h
#            ${imquat_SOURCE_DIR}/imguizmo_quat/vGizmo3D_config.h
#            ${imquat_SOURCE_DIR}/imguizmo_quat/vgMath.h
#            ${imquat_SOURCE_DIR}/imguizmo_quat/vgMath_config.h
    )
#    # Link ImGui to your target
    target_link_libraries(${TARGET} ${ACCESS} imquat)
#
#    # Include ImGui headers
    target_include_directories(imquat PUBLIC ${imquat_SOURCE_DIR}/imguizmo_quat)

    target_compile_definitions(imquat
            PUBLIC
            -DIMGUIZMO_IMGUI_FOLDER=
            -DIMGUIZMO_USE_IMGUI_NAMESPACE
    )
endmacro()