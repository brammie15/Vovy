function(DownloadResource TARGET_DIR URL ZIP_DEST EXTRACT_DIR INNER_FOLDER_NAME)
    cmake_minimum_required(VERSION 3.14)
    include(FetchContent)

    set(SPONZA_DIR "${CMAKE_SOURCE_DIR}/${TARGET_DIR}")

    if(NOT EXISTS "${SPONZA_DIR}")
        file(MAKE_DIRECTORY "${SPONZA_DIR}")
        if(NOT EXISTS "${ZIP_DEST}")
            file(DOWNLOAD ${URL} ${ZIP_DEST} SHOW_PROGRESS)
        else()
            message(STATUS "Sponza zip file already exists, skipping download.")
        endif()
        file(REMOVE_RECURSE "${EXTRACT_DIR}")
        file(MAKE_DIRECTORY "${EXTRACT_DIR}")
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzf ${ZIP_DEST}
                WORKING_DIRECTORY "${EXTRACT_DIR}"
        )

        if(EXISTS "${INNER_FOLDER_NAME}")
            file(GLOB_RECURSE EXTRACTED_FILES RELATIVE "${INNER_FOLDER_NAME}" "${INNER_FOLDER_NAME}/*")
            foreach(FILE_PATH IN LISTS EXTRACTED_FILES)
                set(SOURCE_PATH "${INNER_FOLDER_NAME}/${FILE_PATH}")
                set(DEST_PATH "${SPONZA_DIR}/${FILE_PATH}")
                get_filename_component(DEST_FOLDER "${DEST_PATH}" DIRECTORY)
                file(MAKE_DIRECTORY "${DEST_FOLDER}")
                file(COPY "${SOURCE_PATH}" DESTINATION "${DEST_FOLDER}")
            endforeach()
        else()
            message(WARNING "Expected folder ${INNER_FOLDER_NAME} was not found after extraction!")
        endif()

        file(REMOVE "${ZIP_DEST}")
        file(REMOVE_RECURSE "${EXTRACT_DIR}")

        message(STATUS "Sponza model downloaded and extracted to ${SPONZA_DIR}")
    else()
        message(STATUS "Sponza model already exists at ${SPONZA_DIR}")
    endif()
endfunction()

