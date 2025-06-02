function(fetch_resources_if_missing)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

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
