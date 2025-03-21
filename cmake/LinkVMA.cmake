include(FetchContent)

macro(LinkVma TARGET ACCESS)
    FetchContent_Declare(
            vma
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
            GIT_TAG v3.2.1
    )

    FetchContent_GetProperties(vma)

    if (NOT vma_POPULATED)
        FetchContent_MakeAvailable(vma)
    endif()

    target_include_directories(${TARGET} ${ACCESS} ${vma_SOURCE_DIR})
endmacro()