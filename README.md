**English** | [中文](docs/README.zh_CN.md)

# I18nVault

A lightweight C++ internationalization library supporting both plain JSON and SM4-GCM encrypted TRS file loading.

## Features

- Fast runtime lookup (enum key → localized string)
- Formatted translations with `{0}` `{1}` placeholder substitution
- Dual format: plain JSON and encrypted TRS
- SM4-GCM authenticated encryption, tamper-proof
- Build-time i18n key consistency validation across locales
- Build-time auto-generation of TRS files and enum header
- pImpl idiom keeps public headers clean
- CMake install + CPack packaging, supports `find_package(I18nVault)`
- Cross-platform: Windows / macOS / Linux

## Quick Start

### Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Build as shared library (default is static):

```bash
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release
```

### Run Tests

```bash
ctest --test-dir build -C Release --output-on-failure
```

### Install

```bash
cmake --install build --config Release --prefix /usr/local
```

### Package

```bash
cd build
cpack -C Release
# Output: I18nVault-1.0.0-<OS>-<Arch>.zip / .tar.gz / .deb
```

## Integration

### Option 1: CMake find_package

After installing, add to your `CMakeLists.txt`:

```cmake
find_package(I18nVault REQUIRED)
target_link_libraries(your_target PRIVATE I18nVault::I18nVaultCore)
```

### Option 2: add_subdirectory

Add I18nVault as a subdirectory:

```cmake
add_subdirectory(third_party/I18nVault)
target_link_libraries(your_target PRIVATE I18nVaultCore)
```

## Usage

### Basic Translation

```cpp
#include "i18n_manager.h"

// Get the singleton
auto& mgr = I18nVault::I18nManager::instance();

// Load a JSON locale file
mgr.reload("i18n/en_US.json");

// Translate using the macro
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);
// => "Login"

// Or call the method directly
std::string text2 = mgr.translate(I18nVault::I18nKey::MENU_FILE);
// => "File"
```

### Formatted Translation

Use `{0}` `{1}` placeholders in your JSON:

```json
{
  "WELCOME_FMT": "Welcome, {0}!",
  "dialog": {
    "delete_fmt": "Delete {0}? This action cannot be undone."
  }
}
```

```cpp
// Using the macro (recommended)
std::string msg = I18nVault_TR(I18nVault::I18nKey::WELCOME_FMT, "Alice");
// => "Welcome, Alice!"

std::string msg2 = I18nVault_TR(I18nVault::I18nKey::DIALOG_DELETE_FMT, "photo.jpg");
// => "Delete photo.jpg? This action cannot be undone."

// Or call the method directly
std::string msg3 = mgr.translate(I18nVault::I18nKey::WELCOME_FMT, {"Alice"});
```

### Loading Encrypted TRS Files

```cpp
auto& mgr = I18nVault::I18nManager::instance();

// Configure decryption
mgr.setTrsCryptoConfig({
    .key_hex = "00112233445566778899AABBCCDDEEFF",  // ℹ️ Demo only — replace with a secure key in production
    .aad     = "i18n:v1"                             // Additional authenticated data
});

// Load TRS (auto-decrypts)
mgr.reload("i18n/zh_CN.trs");

// Translation works as usual
std::string text = I18nVault_TR(I18nVault::I18nKey::LOGIN_BUTTON);

// Clear key material when no longer needed
mgr.clearTrsCryptoConfig();
```

## Project Structure

```
CMakeLists.txt                  # Top-level: project settings + install + CPack
cmake/
  I18nVaultConfig.cmake.in      # find_package() template
src/
  CMakeLists.txt                # I18nVaultCore library (static/shared)
  i18n_manager.h                # Public header (pImpl)
  i18n_manager.cpp              # Implementation
  crypto/
    CMakeLists.txt              # SM4-GCM crypto sources (compiled into Core)
    sm4.c / sm4.h               # SM4 block cipher
    gcm.c / gcm.h               # GCM mode
    sm4_gcm.c / sm4_gcm.h       # SM4-GCM unified API
    hex_utils.c / hex_utils.h   # Hex parsing utilities
generated/
  i18n_keys.h                   # Auto-generated enum & mapping functions
i18n/
  en_US.json                    # English locale (baseline)
  zh_CN.json                    # Chinese locale
  *.trs                         # Encrypted files (build-time generated)
test/
  CMakeLists.txt                # Tests
  main.cpp                      # Regression tests
tools/
  CMakeLists.txt                # CLI tools + build-time tasks
  gen_i18n_keys.py              # Generates i18n_keys.h
  i18n_diff_check.py            # Multi-locale key consistency checker
  gen_trs_files.py              # Batch TRS generation
  encrypt_i18n.c                # Encryption/decryption CLI
```

## Install Layout

```
<prefix>/
  include/I18nVault/
    i18n_manager.h              # Public header
    i18n_vault_export.h         # DLL export macros (auto-generated)
  share/I18nVault/tools/
    gen_i18n_keys.py            # Key enum generation script
  lib/
    I18nVaultCore.lib           # Core library (with crypto; static = full lib, shared = import lib)
    cmake/I18nVault/
      I18nVaultConfig.cmake
      I18nVaultConfigVersion.cmake
      I18nVaultTargets.cmake
  bin/
    i18n_crypto_cli             # Encryption/decryption CLI tool
    I18nVaultCore.dll           # Shared library (BUILD_SHARED_LIBS=ON only)
```

## CMake Targets

| Target | Type | Description |
|--------|------|-------------|
| `I18nVaultCore` | Static / Shared lib | Core i18n library (with SM4-GCM crypto, controlled by `BUILD_SHARED_LIBS`) |
| `i18n_crypto_cli` | Executable | JSON/TRS encryption/decryption CLI |
| `i18n_test` | Executable | Regression tests |
| `i18n_keys` | Custom | Generates enum header |
| `i18n_diff_check` | Custom | Key consistency validation |
| `i18n_trs` | Custom | Build-time TRS generation |

## Public API

| Method / Macro | Description |
|----------------|-------------|
| `I18nManager::instance()` | Get the singleton |
| `reload(path)` | Load a .json or .trs file |
| `translate(key)` | Basic translation (no arguments) |
| `translate(key, {args...})` | Translate with `{0}` `{1}` placeholder substitution |
| `setTrsCryptoConfig({key_hex, aad})` | Set TRS decryption parameters |
| `clearTrsCryptoConfig()` | Clear decryption parameters |
| `I18nVault_TR(key, ...)` | Convenience macro: plain translation without extra args, formatted translation with args |

## Configuration

### Build-time (CMake Cache)

| Variable | Default | Description |
|----------|---------|-------------|
| `BUILD_SHARED_LIBS` | `OFF` | Set to `ON` to build I18nVaultCore as a shared library |
| `I18N_TRS_KEY_HEX` | `00112233...FF` | 32-char hex SM4 key |
| `I18N_TRS_AAD` | `i18n:v1` | Additional authenticated data |

### Runtime

Prefer injecting via `setTrsCryptoConfig()`. Falls back to environment variables if not set:

| Environment Variable | Description |
|---------------------|-------------|
| `I18N_TRS_KEY_HEX` / `I18N_SM4_KEY_HEX` | SM4 key |
| `I18N_TRS_AAD` | AAD |

## Adding a New Locale

1. Copy `i18n/en_US.json` to `i18n/<locale>.json`
2. Translate all values (keep key structure unchanged)
3. `i18n_diff_check` automatically validates key completeness at build time
4. Corresponding `.trs` encrypted file is auto-generated during build

## Adding a New Key

1. Add a new key-value pair to `i18n/en_US.json`
2. Update all other locale files accordingly
3. `generated/i18n_keys.h` enum is regenerated automatically at build time
4. Use `I18nVault::I18nKey::NEW_KEY` in your code

## Build-time Data Flow

```
i18n/en_US.json ──→ gen_i18n_keys.py ──→ generated/i18n_keys.h
i18n/*.json ────→ i18n_diff_check.py ──→ Pass / Fail
i18n/*.json ────→ gen_trs_files.py ────→ i18n/*.trs
```

## CI

GitHub Actions cross-platform matrix (Ubuntu / macOS / Windows), automatically runs:

1. Strict key diff validation
2. CMake build
3. CTest tests
4. TRS artifact verification
5. SM4-GCM encrypt/decrypt roundtrip verification
6. cmake install artifact verification
7. CPack packaging
8. Upload packages as Actions artifacts

## License

MIT

## CI Design

CI workflow at `.github/workflows/ci.yml`, matrix execution across:

- ubuntu-latest
- macos-latest
- windows-latest

Pipeline includes:

1. Strict key diff validation
2. CMake build
3. CTest smoke tests
4. TRS artifact existence check
5. Decrypt roundtrip verification using build-produced TRS

## Related Documentation

- TRS encryption/decryption: docs/trs_crypto.md

## Design Trade-offs

- Testing: lightweight executable tests, no heavy test frameworks
- Configuration: explicit runtime configuration preferred, environment variable fallback for compatibility
- Artifacts: TRS files auto-generated at build time to avoid manual errors