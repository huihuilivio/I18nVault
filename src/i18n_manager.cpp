#include "i18n_manager.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

I18nManager &I18nManager::instance() {
  static I18nManager inst;
  return inst;
}

std::string I18nManager::translate(I18nKey key) {
  std::shared_lock lock(mu_);
  auto it = data_.find(i18n_keys_string(key));
  if (it != data_.end())
    return it->second;
  return "";
}

static void flatten(const nlohmann::json &j, const std::string &prefix,
                    std::unordered_map<std::string, std::string> &out) {
  for (auto it = j.begin(); it != j.end(); ++it) {
    std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
    if (it->is_object()) {
      flatten(*it, key, out);
    } else if (it->is_string()) {
      out[key] = it->get<std::string>();
    }
  }
}

bool I18nManager::reload(const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    return false;
  nlohmann::json j;
  f >> j;

  std::unordered_map<std::string, std::string> new_data;
  flatten(j, "", new_data);

  {
    std::unique_lock lock(mu_);
    for (auto &[k, _] : data_) {
      if (new_data.find(k) == new_data.end()) {
        std::cerr << "Missing key: " << k << std::endl;
        return false;
      }
    }
    data_.swap(new_data);
    version_++;
  }
  return true;
}