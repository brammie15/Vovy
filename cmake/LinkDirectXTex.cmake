include(FetchContent)

macro(LinkDirectXTex TARGET ACCESS)
    FetchContent_Declare(
            DirectXTex
            GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git
            GIT_TAG mar2025
    )

    FetchContent_GetProperties(DirectXTex)

    if (NOT DirectXTex_POPULATED)
        FetchContent_MakeAvailable(DirectXTex)
    endif()

    # DirectXTex defines its own target named "DirectXTex"
    target_link_libraries(${TARGET} ${ACCESS} DirectXTex)

#    target_include_directories(${TARGET} ${ACCESS}
#        ${DirectXTex_SOURCE_DIR}/DirectXTex
#    )
endmacro()
