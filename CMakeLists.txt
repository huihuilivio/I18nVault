
cmake_minimum_required(VERSION 3.15)
project(I18nVault VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Enable conformant preprocessor on MSVC for __VA_OPT__ support
if(MSVC)
    string(APPEND CMAKE_C_FLAGS   " /Zc:preprocessor")
    string(APPEND CMAKE_CXX_FLAGS " /Zc:preprocessor")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# Put all binaries (exe + dll) in the same directory so tests find shared libs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Auto-generate i18n_keys.h (with compile-time i18n support) ----
file(GLOB I18N_JSON_FILES "${CMAKE_SOURCE_DIR}/i18n/*.json")
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    DEPENDS ${I18N_JSON_FILES}
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating i18n_keys.h with compile-time i18n support..."
    VERBATIM
)
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/i18n_keys.h)

message(STATUS "i18n_keys.h generation: using gen_i18n_keys_constexpr.py (compile-time i18n)")

# ---- Subdirectories ----
add_subdirectory(tools)
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)
