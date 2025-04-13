include("${CMAKE_SOURCE_DIR}/cmake/DownloadResource.cmake")

# 4GB model
#function(DownloadSponza TARGET_DIR)
#DownloadResource(
#        TARGET_DIR "${TARGET_DIR}"
#        URL "https://cdrdv2.intel.com/v1/dl/getContent/830833"
#        ZIP_DEST "${CMAKE_BINARY_DIR}/main1_sponza.zip"
#        EXTRACT_DIR "${CMAKE_BINARY_DIR}/sponza_extracted"
#        INNER_FOLDER_NAME "${CMAKE_BINARY_DIR}/sponza_extracted/main1_sponza"
#)
#
#endfunction()
#
#function(DownloadSponza TARGET_DIR)
#    DownloadResource(
#            TARGET_DIR "${TARGET_DIR}"
#            URL "https://casual-effects.com/g3d/data10/common/model/crytek_sponza/sponza.zip"
#            ZIP_DEST "${CMAKE_BINARY_DIR}/main1_sponza.zip"
#            EXTRACT_DIR "${CMAKE_BINARY_DIR}/sponza_extracted"
#            INNER_FOLDER_NAME "${CMAKE_BINARY_DIR}/sponza_extracted/"
#    )
#
#endfunction()