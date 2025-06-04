function(fetch_resources_if_missing)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    if(NOT Python3_FOUND)
        message(FATAL_ERROR "Python3 is required to fetch resources, but it was not found.")
    endif()

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/download_resources.py")
        message(FATAL_ERROR "The script download_resources.py is missing in the source directory.")
    endif()

    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/.resources_checked"
            COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/download_resources.py"
            COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/.resources_checked"
            COMMENT "Checking and downloading resources if necessary..."
    )

    add_custom_target(
            DownloadResources ALL
            DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/.resources_checked"
    )
endfunction()

#fetch_resources_if_missing()
