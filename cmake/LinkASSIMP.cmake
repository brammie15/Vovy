include(FetchContent)

macro(LinkASSIMP TARGET ACCESS)
        include(FetchContent)
        FetchContent_Declare(
                assimp
                URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.zip
        )
        FetchContent_MakeAvailable(assimp)

#        OPTION(ASSIMP_BUILD_ZLIB "ON" "Build with zlib support" ON)

        target_link_libraries(${TARGET} ${ACCESS} assimp::assimp)
endmacro()