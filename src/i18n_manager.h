#pragma once
#include "i18n_keys.h"
#include "i18n_vault_export.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace I18nVault
{

// ============================================================================
// ============ PART 1: Compile-time i18n (Zero Runtime Cost) ============
// ============================================================================

/// Compile-time translation with strong typing
/// Usage: constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
/// 
/// The i18n<> template function is auto-generated in i18n_keys.h
/// Each key-language pair is a separate template instantiation with 
/// compile-time selection via if constexpr - zero runtime cost!
///
/// Result: Completely inlined at compile time, zero runtime lookup.

/// Shorter alias: I18N_CT_SELECT(key, lang)
/// All resolved at compile time, zero runtime cost
/// Example: I18N_CT_SELECT(LOGIN_BUTTON, en_US)
#define I18N_CT_SELECT(key, lang) \
    (::I18nVault::i18n<::I18nVault::Key_##key, ::I18nVault::Lang_##lang>())

/// Default language version (defaults to en_US)
/// Example: I18N_CT_DEFAULT(LOGIN_BUTTON) → i18n<Key_LOGIN_BUTTON, Lang_en_US>()
#define I18N_CT_DEFAULT(key) \
    (::I18nVault::i18n<::I18nVault::Key_##key, ::I18nVault::Lang_en_US>())

// ============================================================================
// ======= PART 1.5: Compile-time Key + Runtime FMT (Recommended) =======
// ============================================================================

/// Replace {0}, {1}, ... in a format string with runtime arguments (variadic).
/// The format string itself is resolved at compile time — only substitution is runtime.
/// Usage:
///   std::string msg = i18n_fmt(i18n<Key_WELCOME_FMT, Lang_en_US>(), "Alice");
///   std::string msg = i18n_fmt(i18n<Key_DIALOG_DELETE_FMT, Lang_zh_CN>(), name, path);

namespace detail
{
inline void i18n_fmt_replace(std::string& /*result*/, size_t /*idx*/) {}

template <typename T, typename... Rest>
inline void i18n_fmt_replace(std::string& result, size_t idx, T&& arg, Rest&&... rest)
{
    static_assert(std::is_constructible_v<std::string_view, T>,
                  "i18n_fmt arguments must be convertible to std::string_view");
    std::string_view sv(arg);
    std::string      placeholder = "{" + std::to_string(idx) + "}";
    size_t           pos         = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos)
    {
        result.replace(pos, placeholder.size(), sv);
        pos += sv.size();
    }
    i18n_fmt_replace(result, idx + 1, std::forward<Rest>(rest)...);
}
}  // namespace detail

template <typename... Args>
inline std::string i18n_fmt(std::string_view fmt, Args&&... args)
{
    std::string result(fmt);
    detail::i18n_fmt_replace(result, 0, std::forward<Args>(args)...);
    return result;
}

/// Compile-time key lookup + runtime FMT (variadic, requires at least 1 arg)
/// Usage: I18N_CT_FMT(WELCOME_FMT, en_US, "Alice")
///        I18N_CT_FMT(DIALOG_DELETE_FMT, zh_CN, name, path)
/// Note: For keys without placeholders, use I18N_CT_SELECT instead.
#define I18N_CT_FMT(key, lang, ...) \
    (::I18nVault::i18n_fmt( \
        ::I18nVault::i18n<::I18nVault::Key_##key, ::I18nVault::Lang_##lang>() \
        __VA_OPT__(,) __VA_ARGS__))

/// Default language (en_US) version
/// Usage: I18N_CT_FMT_DEFAULT(WELCOME_FMT, "Alice")
#define I18N_CT_FMT_DEFAULT(key, ...) \
    (::I18nVault::i18n_fmt( \
        ::I18nVault::i18n<::I18nVault::Key_##key, ::I18nVault::Lang_en_US>() \
        __VA_OPT__(,) __VA_ARGS__))

// ============================================================================
// ============ PART 2: Runtime i18n (Backward Compatibility) ============
// ============================================================================

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

    /// Translate an enum key, replacing {0}, {1}, ... with given arguments (Runtime).
    std::string translate(I18nKey key, std::initializer_list<std::string> args = {});

    /// Runtime translation from strong-typed key
    template <typename KeyType>
    std::string translate(std::initializer_list<std::string> args = {})
    {
        // Convert strong type back to enum
        return translate(static_cast<I18nKey>(KeyType::value), args);
    }

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

// ============================================================================
// ============ PART 3: Convenience Macros ============
// ============================================================================

/// ===== RUNTIME i18n (For dynamic translations, backward compatible) =====
/// Usage: I18nVault_TR(I18nKey::LOGIN_BUTTON)
///        I18nVault_TR(I18nKey::WELCOME_FMT, "Alice")
/// Performs runtime lookup and parameter substitution
#define I18nVault_TR(key, ...) \
    (::I18nVault::I18nManager::instance().translate(key, {__VA_ARGS__}))
