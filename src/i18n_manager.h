#pragma once
#include "i18n_keys.h"
#include "i18n_vault_export.h"

#include <initializer_list>
#include <memory>
#include <string>

namespace I18nVault
{

class I18NVAULT_API I18nManager
{
public:
    // ---- Public API ----

    struct TrsCryptoConfig
    {
        std::string key_hex;
        std::string aad;
    };

    static I18nManager& instance();

    /// Load or reload an i18n file (.json or .trs).
    bool reload(const std::string& path);

    /// Translate an enum key, replacing {0}, {1}, ... with given arguments.
    std::string translate(I18nKey key, std::initializer_list<std::string> args = {});

    /// Set runtime TRS decryption parameters.
    bool setTrsCryptoConfig(const TrsCryptoConfig& config);

    /// Clear runtime TRS decryption parameters.
    void clearTrsCryptoConfig();

    I18nManager(const I18nManager&)            = delete;
    I18nManager& operator=(const I18nManager&) = delete;

private:
    I18nManager();
    ~I18nManager();

    std::string tr(I18nKey key);

    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

}  // namespace I18nVault

/// Convenience macro: I18nVault_TR(key) or I18nVault_TR(key, "a", "b") for {0},{1},... formatting.
#define I18nVault_TR(key, ...) \
    (::I18nVault::I18nManager::instance().translate(key, {__VA_ARGS__}))
