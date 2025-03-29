function(DownloadSponza TARGET_DIR)
    cmake_minimum_required(VERSION 3.14)
    include(FetchContent)

    set(SPONZA_URL "https://cdrdv2.intel.com/v1/dl/getContent/830833")
    set(SPONZA_ZIP "${CMAKE_BINARY_DIR}/main1_sponza.zip")
    set(SPONZA_DIR "${CMAKE_SOURCE_DIR}/${TARGET_DIR}")
    set(EXTRACT_DIR "${CMAKE_BINARY_DIR}/sponza_extracted")
    set(INNER_FOLDER "${EXTRACT_DIR}/main1_sponza")

    if(NOT EXISTS "${SPONZA_DIR}")
        file(MAKE_DIRECTORY "${SPONZA_DIR}")
        if(NOT EXISTS "${SPONZA_ZIP}")
            file(DOWNLOAD ${SPONZA_URL} ${SPONZA_ZIP} SHOW_PROGRESS)
        else()
            message(STATUS "Sponza zip file already exists, skipping download.")
        endif()
        file(REMOVE_RECURSE "${EXTRACT_DIR}")
        file(MAKE_DIRECTORY "${EXTRACT_DIR}")
        execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xzf ${SPONZA_ZIP}
                WORKING_DIRECTORY "${EXTRACT_DIR}"
        )

        if(EXISTS "${INNER_FOLDER}")
            file(GLOB_RECURSE EXTRACTED_FILES RELATIVE "${INNER_FOLDER}" "${INNER_FOLDER}/*")
            foreach(FILE_PATH IN LISTS EXTRACTED_FILES)
                set(SOURCE_PATH "${INNER_FOLDER}/${FILE_PATH}")
                set(DEST_PATH "${SPONZA_DIR}/${FILE_PATH}")
                get_filename_component(DEST_FOLDER "${DEST_PATH}" DIRECTORY)
                file(MAKE_DIRECTORY "${DEST_FOLDER}")
                file(COPY "${SOURCE_PATH}" DESTINATION "${DEST_FOLDER}")
            endforeach()
        else()
            message(WARNING "Expected folder ${INNER_FOLDER} was not found after extraction!")
        endif()

        message(STATUS "Sponza model downloaded and extracted to ${SPONZA_DIR}")
    else()
        message(STATUS "Sponza model already exists at ${SPONZA_DIR}")
    endif()
endfunction()

