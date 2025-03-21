include(FetchContent)

macro(linkTinyObjLoader TARGET ACCESS)
    FetchContent_Declare(
            tinyobjloader
            GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
            GIT_TAG release
    )

    FetchContent_MakeAvailable(tinyobjloader)

    target_link_libraries(${TARGET} ${ACCESS} tinyobjloader)
    target_include_directories(${TARGET} PUBLIC ${tinyobjloader_SOURCE_DIR})

endmacro()