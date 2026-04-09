#include "i18n_manager.h"

#include "crypto/hex_utils.h"
#include "crypto/sm4_gcm.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

using I18nVault::I18N_KEY_COUNT;
using I18nVault::i18n_keys_string;
using I18nVault::I18nKey;

namespace
{
constexpr const char* kTrsMagic    = "TRS1";
constexpr uint8_t     kTrsVersion  = 1;
constexpr uint8_t     kTrsReserved = 0;

bool ends_with(const std::string& value, const std::string& suffix)
{
    if (value.size() < suffix.size())
        return false;
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool read_file_binary(const std::string& path, std::vector<uint8_t>& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        return false;

    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    if (size < 0)
        return false;

    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    if (!out.empty())
        f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));

    return f.good() || f.eof();
}

uint32_t read_u32_le(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool decrypt_trs_to_json_text(const std::string& path, const uint8_t key[SM4_GCM_KEY_SIZE], const std::string& aad,
                              std::string& json_text)
{
    std::vector<uint8_t> blob;
    if (!read_file_binary(path, blob))
    {
        std::cerr << "Failed to read trs file: " << path << std::endl;
        return false;
    }

    constexpr size_t kHeaderMin = 4 + 1 + 1 + 1 + 1 + 4;
    if (blob.size() < kHeaderMin)
    {
        std::cerr << "Invalid trs file, too short: " << path << std::endl;
        return false;
    }

    size_t off = 0;
    if (std::memcmp(blob.data(), kTrsMagic, 4) != 0)
    {
        std::cerr << "Invalid trs magic: " << path << std::endl;
        return false;
    }
    off += 4;

    uint8_t  version  = blob[off++];
    uint8_t  iv_len   = blob[off++];
    uint8_t  tag_len  = blob[off++];
    uint8_t  reserved = blob[off++];
    uint32_t ct_len   = read_u32_le(blob.data() + off);
    off += 4;

    if (version != kTrsVersion || reserved != kTrsReserved || iv_len != SM4_GCM_IV_SIZE || tag_len != SM4_GCM_TAG_SIZE)
    {
        std::cerr << "Unsupported trs header: " << path << std::endl;
        return false;
    }

    if (static_cast<uint64_t>(off) + iv_len + tag_len + ct_len != blob.size())
    {
        std::cerr << "Invalid trs length fields: " << path << std::endl;
        return false;
    }

    const uint8_t* iv = blob.data() + off;
    off += iv_len;
    const uint8_t* tag = blob.data() + off;
    off += tag_len;
    const uint8_t* ct = blob.data() + off;

    std::vector<uint8_t> pt(ct_len);
    if (sm4_gcm_decrypt(key, iv, iv_len, reinterpret_cast<const uint8_t*>(aad.data()), aad.size(), ct, ct_len,
                        pt.data(), tag) != 0)
    {
        std::cerr << "Failed to decrypt trs file (auth verify failed): " << path << std::endl;
        return false;
    }

    json_text.assign(reinterpret_cast<const char*>(pt.data()), pt.size());
    return true;
}

std::vector<std::string> collect_required_keys()
{
    std::vector<std::string> keys;
    for (uint16_t i = 0; i < I18N_KEY_COUNT; ++i)
    {
        const char* k = i18n_keys_string(static_cast<I18nKey>(i));
        if (k && k[0] != '\0')
            keys.emplace_back(k);
    }
    return keys;
}

const std::vector<std::string>& get_required_keys()
{
    static const std::vector<std::string> keys = collect_required_keys();
    return keys;
}

void flatten(const nlohmann::json& j, const std::string& prefix, std::unordered_map<std::string, std::string>& out)
{
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
        if (it->is_object())
        {
            flatten(*it, key, out);
        }
        else if (it->is_string())
        {
            out[key] = it->get<std::string>();
        }
        else
        {
            std::cerr << "Skip non-string key: " << key << std::endl;
        }
    }
}

bool parse_json_file(const std::string& path, nlohmann::json& j, const std::optional<std::vector<uint8_t>>& trs_key,
                     const std::string& trs_aad)
{
    if (ends_with(path, ".trs"))
    {
        std::vector<uint8_t> key;
        std::string          aad;

        if (trs_key.has_value())
        {
            key = *trs_key;
            aad = trs_aad;
        }

        if (key.empty())
        {
            const char* key_hex = std::getenv("I18N_TRS_KEY_HEX");
            if (!key_hex)
                key_hex = std::getenv("I18N_SM4_KEY_HEX");
            if (!key_hex)
            {
                std::cerr << "TRS key is not configured. Call setTrsCryptoConfig() or set I18N_TRS_KEY_HEX."
                          << std::endl;
                return false;
            }

            uint8_t env_key[SM4_GCM_KEY_SIZE] = {};
            if (hex_parse(key_hex, env_key, SM4_GCM_KEY_SIZE) != 0)
            {
                std::cerr << "TRS key hex is invalid (must be 32 hex chars)." << std::endl;
                secure_wipe(env_key, sizeof(env_key));
                return false;
            }

            key.assign(env_key, env_key + SM4_GCM_KEY_SIZE);
            secure_wipe(env_key, sizeof(env_key));
            const char* aad_env = std::getenv("I18N_TRS_AAD");
            aad                 = aad_env ? aad_env : "";
        }

        std::string json_text;
        if (!decrypt_trs_to_json_text(path, key.data(), aad, json_text))
        {
            secure_wipe(key.data(), key.size());
            return false;
        }
        secure_wipe(key.data(), key.size());
        j = nlohmann::json::parse(json_text);
    }
    else
    {
        std::ifstream f(path);
        if (!f.is_open())
            return false;
        f >> j;
    }
    return true;
}

bool validate_required_keys(const std::unordered_map<std::string, std::string>& data)
{
    for (const auto& key : get_required_keys())
    {
        if (data.find(key) == data.end())
        {
            std::cerr << "Missing required key: " << key << std::endl;
            return false;
        }
    }
    return true;
}

}  // anonymous namespace

namespace I18nVault
{

struct I18nManager::Impl
{
    using LangData = std::unordered_map<std::string, std::string>;

    mutable std::shared_mutex mu;

    // Multi-language storage: locale -> translations
    std::unordered_map<std::string, LangData> languages;
    std::string                               currentLocale;
    std::string                               fallbackLocale;

    // Language change callbacks
    std::vector<std::pair<size_t, I18nManager::LanguageChangedCallback>> callbacks;
    size_t                                                               nextCallbackId = 1;

    std::optional<std::vector<uint8_t>> trs_key;
    std::string                         trs_aad;

    // Lookup a key in a specific locale's data. Caller must hold at least shared lock.
    const LangData* findLang(const std::string& locale) const
    {
        auto it = languages.find(locale);
        return it != languages.end() ? &it->second : nullptr;
    }
};

I18nManager::I18nManager() : pImpl_(std::make_unique<Impl>()) {}
I18nManager::~I18nManager() = default;

bool I18nManager::setTrsCryptoConfig(const TrsCryptoConfig& config)
{
    if (config.key_hex.size() != SM4_GCM_KEY_SIZE * 2)
        return false;

    std::vector<uint8_t> key(SM4_GCM_KEY_SIZE);
    if (hex_parse(config.key_hex.c_str(), key.data(), SM4_GCM_KEY_SIZE) != 0)
        return false;

    {
        std::unique_lock lock(pImpl_->mu);
        pImpl_->trs_key = std::move(key);
        pImpl_->trs_aad = config.aad;
    }
    return true;
}

void I18nManager::clearTrsCryptoConfig()
{
    std::unique_lock lock(pImpl_->mu);
    if (pImpl_->trs_key.has_value())
    {
        secure_wipe(pImpl_->trs_key->data(), pImpl_->trs_key->size());
        pImpl_->trs_key.reset();
    }
    pImpl_->trs_aad.clear();
}

I18nManager& I18nManager::instance()
{
    static I18nManager inst;
    return inst;
}

std::string I18nManager::tr(I18nKey key)
{
    std::shared_lock  lock(pImpl_->mu);
    const std::string keyStr = i18n_keys_string(key);

    // Try current language
    if (const auto* lang = pImpl_->findLang(pImpl_->currentLocale))
    {
        auto it = lang->find(keyStr);
        if (it != lang->end())
            return it->second;
    }

    // Try fallback language
    if (!pImpl_->fallbackLocale.empty() && pImpl_->fallbackLocale != pImpl_->currentLocale)
    {
        if (const auto* lang = pImpl_->findLang(pImpl_->fallbackLocale))
        {
            auto it = lang->find(keyStr);
            if (it != lang->end())
                return it->second;
        }
    }

    return "";
}

std::string I18nManager::translate(I18nKey key, std::initializer_list<std::string> args)
{
    std::string result = tr(key);
    size_t      idx    = 0;
    for (const auto& arg : args)
    {
        std::string placeholder = "{" + std::to_string(idx) + "}";
        size_t      pos         = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos)
        {
            result.replace(pos, placeholder.size(), arg);
            pos += arg.size();
        }
        ++idx;
    }
    return result;
}

// ---- Language Management Implementation ----

bool I18nManager::addLanguage(const std::string& locale, const std::string& path)
{
    nlohmann::json j;
    try
    {
        std::optional<std::vector<uint8_t>> key;
        std::string                         aad;
        {
            std::shared_lock lock(pImpl_->mu);
            key = pImpl_->trs_key;
            aad = pImpl_->trs_aad;
        }

        if (!parse_json_file(path, j, key, aad))
            return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse i18n file " << path << ": " << e.what() << std::endl;
        return false;
    }

    std::unordered_map<std::string, std::string> new_data;
    flatten(j, "", new_data);

    if (!validate_required_keys(new_data))
        return false;

    {
        std::unique_lock lock(pImpl_->mu);
        pImpl_->languages[locale] = std::move(new_data);

        // If this is the first language loaded, make it active automatically
        if (pImpl_->currentLocale.empty())
        {
            pImpl_->currentLocale = locale;
        }
    }
    return true;
}

bool I18nManager::removeLanguage(const std::string& locale)
{
    std::unique_lock lock(pImpl_->mu);
    if (locale == pImpl_->currentLocale)
    {
        std::cerr << "Cannot remove the current active language: " << locale << std::endl;
        return false;
    }
    if (locale == pImpl_->fallbackLocale)
        pImpl_->fallbackLocale.clear();

    return pImpl_->languages.erase(locale) > 0;
}

bool I18nManager::setLanguage(const std::string& locale)
{
    std::vector<std::pair<size_t, LanguageChangedCallback>> cbs;
    {
        std::unique_lock lock(pImpl_->mu);
        auto             it = pImpl_->languages.find(locale);
        if (it == pImpl_->languages.end())
        {
            std::cerr << "Language not loaded: " << locale << std::endl;
            return false;
        }

        if (pImpl_->currentLocale == locale)
            return true;  // already active

        pImpl_->currentLocale = locale;
        cbs                   = pImpl_->callbacks;  // copy for invoking outside lock
    }

    // Notify listeners outside the lock to avoid deadlocks
    for (const auto& [id, cb] : cbs)
    {
        if (cb)
        {
            try
            {
                cb(locale);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Language change callback (id=" << id << ") threw: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Language change callback (id=" << id << ") threw unknown exception." << std::endl;
            }
        }
    }
    return true;
}

std::string I18nManager::currentLanguage() const
{
    std::shared_lock lock(pImpl_->mu);
    return pImpl_->currentLocale;
}

bool I18nManager::setFallbackLanguage(const std::string& locale)
{
    std::unique_lock lock(pImpl_->mu);
    if (pImpl_->languages.find(locale) == pImpl_->languages.end())
    {
        std::cerr << "Fallback language not loaded: " << locale << std::endl;
        return false;
    }
    pImpl_->fallbackLocale = locale;
    return true;
}

std::string I18nManager::fallbackLanguage() const
{
    std::shared_lock lock(pImpl_->mu);
    return pImpl_->fallbackLocale;
}

std::vector<std::string> I18nManager::availableLanguages() const
{
    std::shared_lock         lock(pImpl_->mu);
    std::vector<std::string> result;
    result.reserve(pImpl_->languages.size());
    for (const auto& [locale, _] : pImpl_->languages)
        result.push_back(locale);
    return result;
}

size_t I18nManager::onLanguageChanged(LanguageChangedCallback callback)
{
    std::unique_lock lock(pImpl_->mu);

    // Find a unique ID, skipping 0 and any existing IDs after wrap-around.
    size_t id    = pImpl_->nextCallbackId++;
    size_t tries = 0;
    while (id == 0 || std::any_of(pImpl_->callbacks.begin(), pImpl_->callbacks.end(),
                                  [id](const auto& p) { return p.first == id; }))
    {
        id = pImpl_->nextCallbackId++;
        if (++tries > pImpl_->callbacks.size() + 1)
        {
            std::cerr << "Too many language change callbacks registered, cannot allocate ID." << std::endl;
            return 0;
        }
    }

    pImpl_->callbacks.emplace_back(id, std::move(callback));
    return id;
}

void I18nManager::removeLanguageChangedCallback(size_t id)
{
    std::unique_lock lock(pImpl_->mu);
    auto&            cbs = pImpl_->callbacks;
    cbs.erase(std::remove_if(cbs.begin(), cbs.end(), [id](const auto& p) { return p.first == id; }), cbs.end());
}

}  // namespace I18nVault