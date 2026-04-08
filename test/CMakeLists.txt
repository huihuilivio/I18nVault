add_executable(i18n_test main.cpp)
target_link_libraries(i18n_test PRIVATE I18nVaultCore)
# Test directly calls crypto C APIs (hex_parse, sm4_gcm_encrypt) which are not
# part of the public DLL interface. Link the OBJECT library for those symbols.
target_sources(i18n_test PRIVATE $<TARGET_OBJECTS:I18nVaultCrypto>)
target_include_directories(i18n_test PRIVATE
    $<TARGET_PROPERTY:I18nVaultCrypto,INTERFACE_INCLUDE_DIRECTORIES>
)

add_test(NAME i18n_manager_smoke COMMAND $<TARGET_FILE:i18n_test>)
set_tests_properties(i18n_manager_smoke PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
