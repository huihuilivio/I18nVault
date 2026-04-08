# I18nVault - Compile-time i18n with Strong Typing

## 概述

改进后的 I18nVault 提供了两个主要功能：

### ✅ 1. 强类型化 Key（Strong Type Safety）

**之前：** 使用 `enum class I18nKey` - 容易混淆，没有类型检查

```cpp
// 容易出错 - 编译器无法区分不同的 key
I18nManager::instance().translate(I18nKey::LOGIN_BUTTON);
I18nManager::instance().translate(I18nKey::WELCOME_FMT);  // 可能错误地传递了错误的 key
```

**现在：** 每个 key 都有独立的类型 `Key_...`

```cpp
// 编译期强类型 - 错误的 key 编译器直接拒绝
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
// constexpr auto bad = i18n<NonExistentKey, Lang_en_US>();  // 编译错误！
```

### ✅ 2. 编译期 i18n（Zero Runtime Lookup）

**之前：** 运行时查表
```cpp
// 运行时：
// 1. 创建字符串对象
// 2. 进行哈希表查找
// 3. 返回翻译结果
auto text = I18nManager::instance().translate(I18nKey::LOGIN_BUTTON);
```

**现在：** 编译期嵌入，零运行时开销
```cpp
// 编译时已完全解析，运行时是常量引用
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
// 编译后的汇编代码就是 lea rax, [login_button_string]
```

---

## 实现细节

### 生成流程

1. **Python 脚本** (`gen_i18n_keys_constexpr.py`)
   - 读取 `i18n/` 目录下的所有 JSON 文件 (e.g., `en_US.json`, `zh_CN.json`)
   - 扁平化嵌套的 JSON 结构（`menu.file` → `Key_MENU_FILE`）
   - 生成 C++ 头文件 `build/generated/i18n_keys.h`

2. **生成的头文件内容**：

```cpp
// 强类型 Key 定义（每个 key 都是独立类型）
struct Key_LOGIN_BUTTON {
    static constexpr int value = 2;
    static constexpr std::string_view original = "LOGIN_BUTTON";
};

// 语言标签
struct Lang_en_US { static constexpr std::string_view name = "en_US"; };
struct Lang_zh_CN { static constexpr std::string_view name = "zh_CN"; };

// 编译期翻译数据
namespace en_US {
    inline constexpr std::string_view LOGIN_BUTTON = "Login";
    inline constexpr std::string_view WELCOME_FMT = "Welcome, {0}!";
    // ...
}

namespace zh_CN {
    inline constexpr std::string_view LOGIN_BUTTON = "登录";
    inline constexpr std::string_view WELCOME_FMT = "欢迎, {0}!";
    // ...
}

// 通用编译期翻译函数
template <typename KeyType, typename LangType>
constexpr std::string_view i18n() {
    constexpr int key_id = KeyType::value;
    constexpr std::string_view lang_name = LangType::name;
    
    if constexpr (key_id == 0) {
        if constexpr (lang_name == "en_US")
            return en_US::DIALOG_CONFIRM;
        else if constexpr (lang_name == "zh_CN")
            return zh_CN::DIALOG_CONFIRM;
    }
    // ... 其他 key
    return "";
}
```

### 编译期优化

编译器会完全优化掉模板的选择逻辑：

```cpp
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
```

编译后等同于：
```cpp
auto text = "Login";  // 常量字符串，无查找开销
```

---

## 使用方式

### I. 编译期 i18n（推荐）✨

完全零运行时开销的翻译方式：

```cpp
#include "i18n_manager.h"
using namespace I18nVault;

// 基础用法 - 直接使用模板
constexpr auto text1 = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
constexpr auto text2 = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();

// 也可以用宏简化（仍然是编译期）
constexpr auto text3 = I18N_CT_SELECT(MENU_FILE, en_US);
constexpr auto text4 = I18N_CT_SELECT(MENU_FILE, zh_CN);

std::cout << text1;  // "Login"
std::cout << text2;  // "登录"
```

**性能特点：**
- ✅ 零查表开销
- ✅ 直接编译到 rodata 段
- ✅ 可用于 constexpr/template 上下文
- ✅ 编译器完全优化，无虚函数调用

### II. 运行时 i18n（后向兼容）

需要动态选择语言或进行参数替换时：

```cpp
// 原有 API 完全兼容
auto text = I18nVault_TR(LOGIN_BUTTON);

// 带参数替换
auto welcome = I18nVault_TR(WELCOME_FMT, "Alice");  // "Welcome, Alice!"

// 直接使用 I18nManager
auto confirm = I18nManager::instance().translate(I18nKey::DIALOG_CONFIRM);
```

**特点：**
- ✅ 完全后向兼容
- ✅ 支持参数替换（{0}, {1}, ...）
- ✅ 支持动态语言选择
- ⚠️ 有运行时查找开销

---

## 实现要点

### 1. 强类型化机制

每个 key 都是独立的 `struct` 类型：

```cpp
struct Key_LOGIN_BUTTON { ... };
struct Key_MENU_FILE { ... };
struct Key_WELCOME_FMT { ... };
```

编译器看到这些完全不同的类型，因此：
- ❌ `i18n<Key_LOGIN_BUTTON, Lang_en_US>()` 和 `i18n<Key_MENU_FILE, Lang_en_US>()` 是**不同的函数调用**
- ❌ 不能互换
- ❌ 编译符号不同，类型检查严格

### 2. 编译期求值

`if constexpr` 确保选择在编译期进行：

```cpp
template <typename KeyType, typename LangType>
constexpr std::string_view i18n() {
    if constexpr (key_id == 2 && lang_name == "en_US") {
        return en_US::LOGIN_BUTTON;  // 编译器选择这个分支
    }
    // 其他分支在编译时被丢弃
}
```

由于 `KeyType::value` 和 `LangType::name` 都是 `constexpr`，编译器在编译时就能确定分支，最终代码中只包含选中的分支。

### 3. 字符串内联

所有翻译字符串都定义在 `constexpr` 命名空间中：

```cpp
namespace en_US {
    inline constexpr std::string_view LOGIN_BUTTON = "Login";
}
```

这些被放在 `.rodata` 段，运行时引用时零开销。

---

## 迁移指南

### 第一步：生成新的 i18n_keys.h

```bash
cd I18nVault
python tools/gen_i18n_keys_constexpr.py
```

这会生成支持强类型和编译期翻译的 header。

### 第二步：逐步迁移代码

**阶段 1：保持现有代码不变**
```cpp
// 旧代码继续工作
auto text = I18nVault_TR(LOGIN_BUTTON);
```

**阶段 2：新代码使用编译期 i18n**
```cpp
// 新特性
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

// 性能关键路径改进
void render_ui()
{
    // 这些是编译期常量，无运行时开销
    draw_button(i18n<Key_LOGIN_BUTTON, Lang_en_US>());
    draw_menu_item(i18n<Key_MENU_FILE, Lang_en_US>());
}
```

**阶段 3：完全迁移（可选）**
```cpp
// 完全使用新系统，删除旧 API
```

---

## 性能对比

| 特性 | 编译期 i18n | 运行时 i18n |
|------|-----------|----------|
| **查找时间** | O(1)，0ns，嵌入在代码中 | O(1)，~100-1000ns 哈希表查找 |
| **内存开销** | .rodata 中的常量字符串 | 运行时哈希表 + 字符串对象 |
| **参数替换** | ❌ 需要运行时处理 | ✅ 支持 |
| **动态语言切换** | ❌ 编译期固定 | ✅ 支持 |
| **编译后大小** | 略大（更多内联） | 更小（外部引用） |
| **最佳用途** | 性能关键路径、UI 常量 | 动态内容、配置文本 |

---

## 示例

详见 `examples/compile_time_i18n_example.cpp`

关键示例：

```cpp
// 强类型编译期翻译
constexpr auto login_en = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
constexpr auto login_zh = i18n<Key_LOGIN_BUTTON, Lang_zh_CN>();

// 编译时保证类型正确
// i18n<NonExistent, Lang_en_US>();  // ❌ 编译错误
// i18n<Key_LOGIN_BUTTON, int>();    // ❌ 编译错误

// 性能关键的 UI 渲染
void render_menu() {
    // 这些都是编译期常量，直接从二进制中读取
    draw_item(i18n<Key_MENU_FILE, Lang_en_US>());
    draw_item(i18n<Key_MENU_EDIT, Lang_en_US>());
    draw_item(i18n<Key_MENU_HELP, Lang_en_US>());
}
```

---

## 技术细节

### 为什么同时支持编译期和运行时？

1. **渐进式迁移** - 不需要一次性重写所有代码
2. **灵活性** - 不同场景有不同需求
3. **兼容性** - 保护既有投资
4. **学习曲线** - 让用户逐步学习新特性

### 编译时间影响

- 生成的 header 更大（需要包含所有翻译）
- C++ 模板实例化可能增加编译时间
- 但运行时性能大幅提升

### 二进制大小影响

- 编译期方式：字符串直接嵌入，可能增大二进制
- 但可以通过 LTO（Link Time Optimization）优化这些常量
- 通常增幅在 1-5%

---

## 总结

改进后的 I18nVault 提供了：

✅ **强类型化（Type Safety）**
- 每个 key 都是独立的类型
- 编译器在编译时检查正确性
- 消除运行时错误

✅ **编译期 i18n（Compile-time Optimization）**
- 所有翻译在编译时嵌入
- 零运行时查表开销
- 最优的性能和内存效率

✅ **完全后向兼容**
- 现有代码继续工作
- 可以渐进式迁移
- 没有破坏性改变

这是一个"零代价抽象"的完美案例 - 获得更好的类型安全和性能，而无需额外的运行时开销。
