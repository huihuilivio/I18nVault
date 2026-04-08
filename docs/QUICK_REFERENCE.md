# I18nVault Quick Reference

## 快速对比

### 👑 推荐：编译期 i18n（零运行时开销）

```cpp
// ✨ 编译期翻译 - 最性能、最安全
#include "i18n_manager.h"
using namespace I18nVault;

// 基础用法
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

// 多语言
auto text_en = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
auto text_zh = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();

// 在常量表达式中使用
constexpr std::array<std::string_view, 3> menu = {
    i18n<Key_MENU_FILE, Lang_en_US>(),
    i18n<Key_MENU_EDIT, Lang_en_US>(),
    i18n<Key_MENU_HELP, Lang_en_US>(),
};
```

### 🔧 后向兼容：运行时 i18n

```cpp
// 旧方式 - 仍然支持
auto text = I18nVault_TR(LOGIN_BUTTON);

// 带参数替换
auto msg = I18nVault_TR(WELCOME_FMT, "Alice");  // "Welcome, Alice!"

// 直接调用 Manager
I18nManager::instance().translate(I18nKey::DIALOG_CONFIRM);
```

---

## 完整对照表

| 功能 | 编译期方式 | 运行时方式 |
|------|---------|---------|
| **基础翻译** | `i18n<Key_X, Lang_en_US>()` | `I18nVault_TR(X)` |
| **多语言** | `i18n<Key_X, Lang_zh_CN>()` | 需要运行时选择 |
| **参数替换** | ❌ 不支持 | `I18nVault_TR(X, "arg")` |
| **性能** | ⚡ 零开销 | ⚠️ 哈希表查找 |
| **类型检查** | ✅ 编译时 | ❌ 运行时 |
| **常量用途** | ✅ 可用 | ❌ 不可用 |

---

## 使用模式

### 模式 1：UI 字符串（编译期最佳）

```cpp
// ✅ 推荐
void draw_ui() {
    draw_button(i18n<Key_LOGIN_BUTTON, Lang_en_US>());
    draw_button(i18n<Key_MENU_FILE, Lang_en_US>());
}

// ❌ 不推荐（有运行时开销）
void draw_ui() {
    draw_button(I18nVault_TR(LOGIN_BUTTON));
    draw_button(I18nVault_TR(MENU_FILE));
}
```

### 模式 2：动态参数替换

```cpp
// ✅ 编译期获取模板，运行时替换参数
void show_welcome(const std::string& name) {
    constexpr auto template_en = i18n<Key_WELCOME_FMT, Lang_en_US>();
    // template_en = "Welcome, {0}!"
    // 手动替换或使用 fmt 库
}

// ✅ 或使用运行时方式
void show_welcome(const std::string& name) {
    auto msg = I18nVault_TR(WELCOME_FMT, name);  // 一步完成
}
```

### 模式 3：多语言动态选择

```cpp
// ✅ 编译期方式需要编译时已知语言
void main_english() {
    str = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
}

void main_chinese() {
    str = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();
}

// ✅ 或运行时方式（更灵活）
void main() {
    std::string lang = get_user_language();  // 动态读取
    if (lang == "zh_CN") {
        str = I18nVault_TR(LOGIN_BUTTON);  // 运行时查表
    }
}
```

---

## 类型安全示例

### ✅ 编译期类型检查

```cpp
// ✅ 正确 - 编译通过
auto a = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

// ✅ 正确 - 编译通过  
auto b = i18n<Key_MENU_FILE, Lang_zh_CN>();

// ❌ 编译错误 - 不存在的 key
auto c = i18n<Key_NONEXISTENT, Lang_en_US>();

// ❌ 编译错误 - 错误的语言类型
auto d = i18n<Key_LOGIN_BUTTON, MyCustomLang>();

// 编译器在编译时直接拒绝错误！
```

### 运行时无类型检查

```cpp
// 所有这些都在运行时执行
I18nVault_TR(LOGIN_BUTTON);       // ✅ 可能成功
I18nVault_TR(TYPO_BUTTON);        // ⚠️ 返回空字符串（运行时才发现）
I18nVault_TR(WELCOME_FMT, "a");   // ✅ 替换参数
I18nVault_TR(WELCOME_FMT);        // ✅ 返回模板 "Welcome, {0}!"
```

---

## 性能技巧

### 🚀 最快：编译期常量

```cpp
// 编译后就是常数引用，无任何运行时开销
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
draw_button(text);  // 直接引用，零查找
```

**汇编代码示例：**
```asm
; 编译期方式 - 直接引用常量
lea rax, [rip + login_button_string]

; 运行时方式 - 需要查表
call I18nManager::translate
```

### ⚡ 快速：使用 string_view

```cpp
// string_view 是轻量级引用，零开销
std::string_view text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
// 不会创建字符串对象副本
```

### ⚠️ 较慢：创建字符串对象

```cpp
// 会在堆上分配内存
std::string text = I18nVault_TR(LOGIN_BUTTON);
```

---

## 添加新的翻译

### 步骤 1：编辑 JSON 文件

编辑 `i18n/en_US.json` 和 `i18n/zh_CN.json`：

```json
{
  "LOGIN_BUTTON": "Login",
  "WELCOME_FMT": "Welcome, {0}!",
  "menu": {
    "file": "File",
    "edit": "Edit"
  }
}
```

### 步骤 2：重新生成 Header

```bash
python tools/gen_i18n_keys_constexpr.py
```

这会自动生成新的 key 类型：
```cpp
struct Key_NEW_STRING { ... };

namespace en_US {
    inline constexpr std::string_view NEW_STRING = "...";
}
namespace zh_CN {
    inline constexpr std::string_view NEW_STRING = "...";
}
```

### 步骤 3：使用新的 Key

```cpp
auto text = i18n<Key_NEW_STRING, Lang_en_US>();
```

自动获得类型检查和编译期优化！

---

## 常见问题

### Q: 如何选择编译期还是运行时？

```
需要参数替换? → 运行时 I18nVault_TR()
需要动态语言? → 运行时 I18nVault_TR()
性能关键代码? → 编译期 i18n<Key_X, Lang_Y>()
UI 常量字符串? → 编译期 i18n<Key_X, Lang_Y>()
```

### Q: 能混用吗？

```cpp
// ✅ 完全可以混用
constexpr auto button = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
auto message = I18nVault_TR(WELCOME_FMT, "Alice");
```

### Q: 能在 template 中用吗？

```cpp
// ✅ 可以
template <typename KeyType>
class LocalizedWidget {
    std::string_view text = i18n<KeyType, Lang_en_US>();
};

LocalizedWidget<Key_LOGIN_BUTTON> w1;
```

### Q: 编译时间会变长吗？

- 生成的 header 更大 → 编译时间略增
- 但运行时性能大幅提升
- 值得权衡！

### Q: 二进制会变大吗？

- 编译期方式：字符串直接嵌入
- 通常增加 1-5%
- 可以通过 LTO 优化

---

## 快速命令

```bash
# 生成新的 i18n_keys.h（添加翻译后运行）
python tools/gen_i18n_keys_constexpr.py

# 编译项目
cmake --build build --config Release

# 运行示例
./build/bin/Release/i18n_example.exe
```

---

## 工作流总结

1. **编辑翻译** → `i18n/en_US.json`, `i18n/zh_CN.json`
2. **生成代码** → `python gen_i18n_keys_constexpr.py`
3. **编译期用** → `i18n<Key_X, Lang_Y>()`
4. **运行时用** → `I18nVault_TR(X, args)`
5. **享受性能** → ⚡

---

**记住：新系统是零代价抽象！**  
- 更好的类型安全 ✅
- 更快的运行速度 ✅  
- 完全后向兼容 ✅
