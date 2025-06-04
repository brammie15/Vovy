include(FetchContent)

macro(LinkASSIMP TARGET ACCESS)

        # Prevent FetchContent from re-including if already done
        if (NOT assimp_POPULATED)
#                set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
#                set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
#                set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
#                set(ASSIMP_BUILD_USD_IMPORTER ON CACHE BOOL "" FORCE)
#
#                set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
                # You may want to control zlib too if needed:
                # set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

                include(FetchContent)
                FetchContent_Declare(
                        assimp
                        URL https://github.com/assimp/assimp/archive/refs/tags/v5.4.3.zip
                )
                FetchContent_MakeAvailable(assimp)
        endif()

        target_link_libraries(${TARGET} ${ACCESS} assimp::assimp)

endmacro()
