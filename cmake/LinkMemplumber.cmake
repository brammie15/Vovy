include(FetchContent)

macro(LinkMemPlumber TARGET ACCESS)
    FetchContent_Declare(
            memplumber
            GIT_REPOSITORY https://github.com/seladb/MemPlumber.git
    )

    FetchContent_MakeAvailable(memplumber)

    #COLLECT_STATIC_VAR_DATA

    option(COLLECT_STATIC_VAR_DATA
            "Collect data also on static variable memory allocation" ON)

    target_link_libraries(${TARGET} ${ACCESS} memplumber)
endmacro()