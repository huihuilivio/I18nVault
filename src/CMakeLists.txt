add_subdirectory(crypto)

add_library(I18nVaultCore
    i18n_manager.cpp
    $<TARGET_OBJECTS:I18nVaultCrypto>
)
target_include_directories(I18nVaultCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/exports>
        $<INSTALL_INTERFACE:include/I18nVault>
    PRIVATE
        $<TARGET_PROPERTY:I18nVaultCrypto,INTERFACE_INCLUDE_DIRECTORIES>
)
add_dependencies(I18nVaultCore i18n_keys i18n_diff_check i18n_trs)

include(GenerateExportHeader)
generate_export_header(I18nVaultCore
    EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/i18n_vault_export.h
    EXPORT_MACRO_NAME I18NVAULT_API
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(I18nVaultCore PRIVATE I18nVaultCore_EXPORTS)
endif()
