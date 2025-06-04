include(FetchContent)

macro(LinkGli TARGET ACCESS)
    # Commit 779b99a
    FetchContent_Declare(
            gli
            GIT_REPOSITORY https://github.com/g-truc/gli.git
            GIT_TAG     779b99a
    )

    FetchContent_GetProperties(gli)

    if (NOT gli_POPULATED)
        FetchContent_MakeAvailable(gli)
    endif()

    target_link_libraries(${TARGET} ${ACCESS} gli)

endmacro()
