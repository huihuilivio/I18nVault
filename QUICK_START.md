# 快速开始 - I18nVault 编译期 i18n

## 🚀 5 分钟快速上手

### 第 1 步：理解两种用法

**编译期方式（推荐，零开销）**
```cpp
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
std::cout << text;  // "Login"
```

**运行时方式（兼容，支持参数）**
```cpp
auto text = I18nVault_TR(LOGIN_BUTTON);
auto msg = I18nVault_TR(WELCOME_FMT, "Alice");  // "Welcome, Alice!"
```

### 第 2 步：在代码中使用

```cpp
#include "i18n_manager.h"
using namespace I18nVault;

int main()
{
    // ✨ 编译期 - 零开销
    constexpr auto en_text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
    constexpr auto zh_text = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();
    
    std::cout << "EN: " << en_text << std::endl;  // EN: Login
    std::cout << "ZH: " << zh_text << std::endl;  // ZH: 登录
    
    // 🔧 运行时 - 支持参数替换
    auto welcome = I18nVault_TR(WELCOME_FMT, "Alice");
    std::cout << welcome << std::endl;  // Welcome, Alice!
    
    return 0;
}
```

### 第 3 步：添加新翻译

编辑 `i18n/en_US.json`：
```json
{
  "LOGIN_BUTTON": "Login",
  "NEW_STRING": "My new translation"
}
```

同样编辑 `i18n/zh_CN.json`：
```json
{
  "LOGIN_BUTTON": "登录",
  "NEW_STRING": "我的新翻译"
}
```

重新生成：
```bash
python tools/gen_i18n_keys_constexpr.py
```

自动生成新的 Key 类型 `Key_NEW_STRING`，可立即使用！

---

## 📋 完整示例

### 示例 1：简单翻译

```cpp
#include "i18n_manager.h"
using namespace I18nVault;

// 编译期翻译（零成本）
void print_menu()
{
    std::cout << i18n<Key_MENU_FILE, Lang_en_US>() << std::endl;    // File
    std::cout << i18n<Key_MENU_EDIT, Lang_en_US>() << std::endl;    // Edit
    std::cout << i18n<Key_MENU_HELP, Lang_en_US>() << std::endl;    // Help
}
```

### 示例 2：多语言支持

```cpp
template<typename LangType>
void print_menu()
{
    std::cout << i18n<Key_MENU_FILE, LangType>() << std::endl;
    std::cout << i18n<Key_MENU_EDIT, LangType>() << std::endl;
}

// 使用
print_menu<Lang_en_US>();  // 英文菜单
print_menu<Lang_zh_CN>();  // 中文菜单
```

### 示例 3：参数替换

```cpp
// 获取编译期翻译模板
constexpr auto welcome_template = 
    i18n<Key_WELCOME_FMT, Lang_en_US>();

std::cout << welcome_template << std::endl;  // Welcome, {0}!

// 使用运行时方式替换参数
std::string personalized = 
    I18n Vault_TR(WELCOME_FMT, "Alice");

std::cout << personalized << std::endl;  // Welcome, Alice!
```

### 示例 4：constexpr 上下文

```cpp
// 所有翻译都是编译期常量
constexpr std::array menu = {
    i18n<Key_MENU_FILE, Lang_en_US>(),
    i18n<Key_MENU_EDIT, Lang_en_US>(),
    i18n<Key_MENU_HELP, Lang_en_US>(),
};

// 编译后这是静态常量，零运行时成本
for (const auto& item : menu)
    std::cout << item << std::endl;
```

---

## 🎯 何时使用哪种方式？

### 使用编译期 `i18n<Key_X, Lang_Y>()`：
✅ UI 字符串（按钮、菜单、标签）
✅ 性能关键代码
✅ 固定文本、常量字符串
✅ 模板和 constexpr 上下文

### 使用运行时 `I18nVault_TR()`：
✅ 需要参数替换（{0}, {1}, ...）
✅ 动态语言切换（基于用户选择）
✅ 条件文本
✅ 向后兼容现有代码

---

## 🔧 故障排除

### 编译错误：找不到 Key_XXX

**原因：** 翻译文件中还没有这个 Key

**解决：**
1. 在 `i18n/en_US.json` 和 `i18n/zh_CN.json` 中添加键
2. 运行 `python tools/gen_i18n_keys_constexpr.py`
3. 重新编译

### 错误：cannot use non-constant-expression

**原因：** 尝试在需要常量的上下文中使用非 constexpr

**解决：** 确保使用 `i18n<Key_X, Lang_Y>()` 而不是 `I18nVault_TR()`
```cpp
// ❌ 错误
constexpr auto text = I18nVault_TR(LOGIN_BUTTON);

// ✅ 正确
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
```

### 编译时间过长

**原因：** 模板实例化导致编译体积增大

**解决：** 
- 这是正常的（权衡性能获得）
- 使用 incremental build
- 考虑 LTO (Link Time Optimization)

---

## 📊 性能验证

编译后查看汇编，编译期翻译会直接引用常量：

```cpp
// 编译期方式
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

// 生成的汇编（伪代码）
// lea rax, [rip + const_login_button_string]  // 直接常量引用
```

对比运行时方式：
```cpp
// 运行时方式
auto text = I18nVault_TR(LOGIN_BUTTON);

// 生成的汇编（伪代码）
// call I18nManager::instance()          // 获取单例
// call I18nManager::translate()         // 哈希表查询
// ... 内存分配和字符串复制
```

性能差异：**编译期 = 0ns，运行时 = 100-1000ns**

---

## ✨ 进阶技巧

### 技巧 1：创建强类型的翻译函数

```cpp
template<typename LangType>
std::string_view get_menu_file()
{
    return i18n<Key_MENU_FILE, LangType>();
}

// 类型安全的调用
std::string_view en = get_menu_file<Lang_en_US>();
std::string_view zh = get_menu_file<Lang_zh_CN>();

// ❌ 编译错误：类型不对
// auto bad = get_menu_file<int>();
```

### 技巧 2：编译期字符串数组

```cpp
template<typename LangType>
constexpr auto get_all_menus()
{
    return std::array{
        i18n<Key_MENU_FILE, LangType>(),
        i18n<Key_MENU_EDIT, LangType>(),
        i18n<Key_MENU_HELP, LangType>(),
    };
}

// 完全编译期生成
constexpr auto en_menus = get_all_menus<Lang_en_US>();
constexpr auto zh_menus = get_all_menus<Lang_zh_CN>();
```

### 技巧 3：条件编译

```cpp
#ifdef ENGLISH_BUILD
    constexpr auto default_lang = Lang_en_US;
#else
    constexpr auto default_lang = Lang_zh_CN;
#endif

// 编译时选择语言
constexpr auto message = i18n<Key_LOGIN_BUTTON, decltype(default_lang)>();
```

---

## 📚 完整资源链接

| 资源 | 位置 | 用途 |
|------|------|------|
| 详细设计 | `docs/COMPILE_TIME_I18N.md` | 理解实现原理 |
| 快速参考 | `docs/QUICK_REFERENCE.md` | 常见用法查询 |
| 详细示例 | `examples/compile_time_i18n_example.cpp` | 7 个详细例子 |
| 演示程序 | `src/i18n_demo.cpp` | 运行查看演示 |
| 改进总结 | `IMPROVEMENTS_SUMMARY.md` | 完整改进列表 |

---

## 🎓 学习路径

**初级 - 基础使用（现在）**
1. 理解两种用法（编译期 vs 运行时）
2. 在项目中尝试编译期方式
3. 运行 `i18n_demo.cpp` 查看输出

**中级 - 进阶应用**
1. 阅读 `QUICK_REFERENCE.md` 学习常见模式
2. 查看 `examples/compile_time_i18n_example.cpp` 详细例子
3. 将项目代码逐步迁移到编译期方式

**高级 - 深入理解**
1. 阅读 `COMPILE_TIME_I18N.md` 设计文档
2. 查看生成的 `build/generated/i18n_keys.h` 源代码
3. 研究 Python 生成脚本 `tools/gen_i18n_keys_constexpr.py`
4. 根据需要定制生成器

---

## 💡 核心原则

```
┌─────────────────────────────────────┐
│ 零代价抽象的三要素                    │
├─────────────────────────────────────┤
│ 1. 强类型 ← 编译期检查，运行时零成本  │
│ 2. constexpr ← 编译期求值，无查询    │
│ 3. if constexpr ← 编译期条件，死代码删除 │
└─────────────────────────────────────┘

结果：
✨ 比肩手写硬代码的性能
🛡️ 比 C++ 模板更好的类型安全
🔄 完全兼容现有代码库
```

---

## 🚀 下一步行动

```bash
# 1. 尝试编译期翻译
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

# 2. 添加新翻译
# 编辑 i18n/*.json → python tools/gen_i18n_keys_constexpr.py

# 3. 查看更多例子
cat examples/compile_time_i18n_example.cpp

# 4. 阅读详细文档
cat docs/COMPILE_TIME_I18N.md
```

**祝贺！你现在掌握了现代 C++ 的国际化系统！** 🎉
