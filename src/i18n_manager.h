#pragma once
#include "i18n_keys.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class I18nManager
{
public:
    static I18nManager& instance();
    std::string         translate(I18nKey key);
    bool                reload(const std::string& path);

    I18nManager(const I18nManager&)            = delete;
    I18nManager& operator=(const I18nManager&) = delete;

private:
    I18nManager()  = default;
    ~I18nManager() = default;
    mutable std::shared_mutex                    mu_;
    std::unordered_map<std::string, std::string> data_;
    int                                          version_ = 0;
};