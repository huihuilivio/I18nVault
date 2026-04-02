#include "i18n_keys.h"
#include "i18n_manager.h"

#include <iostream>

int main()
{
    I18nManager::instance().reload("i18n/en_US.json");
    std::cout << I18nManager::instance().translate(I18nKey::LOGIN_BUTTON) << std::endl;
    return 0;
}