# CMake Integration for Compile-time i18n

## 📋 概述

更新后的 CMakeLists.txt 现在自动执行 `tools/gen_i18n_keys_constexpr.py` 脚本，在构建时生成支持编译期 i18n 的头文件。

## ✨ 特性

### 自动代码生成
```cmake
# 当以下任一文件变化时自动重新生成：
# - i18n/en_US.json
# - i18n/zh_CN.json
# - tools/gen_i18n_keys_constexpr.py

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    DEPENDS ...
    COMMENT "Generating i18n_keys.h with compile-time i18n support..."
)
```

### 完整依赖跟踪
- 依赖于所有 i18n JSON 文件（en_US.json, zh_CN.json）
- 依赖于脚本本身（gen_i18n_keys_constexpr.py）
- 工作目录设为项目根目录
- 详细的进度消息

## 🔄 工作流程

### 1. 初始配置
```bash
cmake -S . -B build
```

**输出：**
```
-- i18n_keys.h generation: using gen_i18n_keys_constexpr.py (compile-time i18n)
-- Configuring done (0.4s)
```

### 2. 编译时自动生成
```bash
cmake --build build --config Release
```

**输出：**
```
Generating i18n_keys.h with compile-time i18n support...
[OK] Header generated: D:\livio\I18nVault\build\generated\i18n_keys.h
[OK] Supported languages: ['en_US', 'zh_CN']
```

### 3. 增量构建
修改 JSON 文件后，下次编译会自动重新生成：

```bash
# 编辑翻译文件
vim i18n/en_US.json

# 自动重新生成 i18n_keys.h
cmake --build build --config Release
```

## 📊 生成的文件

### 位置
```
build/generated/i18n_keys.h
```

### 内容
```cpp
// 强类型 Key
struct Key_LOGIN_BUTTON { ... };
struct Key_MENU_FILE { ... };

// 语言标签
struct Lang_en_US { ... };
struct Lang_zh_CN { ... };

// 编译期翻译数据
namespace en_US { ... }
namespace zh_CN { ... }

// 编译期翻译模板
template <typename KeyType, typename LangType>
constexpr std::string_view i18n() { ... }

// 向后兼容的 enum
enum class I18nKey { ... };
```

## 🛠️ CMakeLists.txt 关键部分

```cmake
# 1. Python 解释器配置
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# 2. 自动生成目标
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    DEPENDS ${CMAKE_SOURCE_DIR}/i18n/en_US.json
            ${CMAKE_SOURCE_DIR}/i18n/zh_CN.json
            ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating i18n_keys.h with compile-time i18n support..."
    VERBATIM
)

# 3. 创建 target 以跟踪生成
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/i18n_keys.h)

# 4. 处理顺序：i18n_keys 首先生成，然后其他目标可以使用
add_subdirectory(tools)  # gen_trs_files.py 使用生成的 i18n_keys.h
add_subdirectory(src)    # 链接时包含生成的 header
add_subdirectory(test)
```

## 🔍 依赖链

```
CMakeLists.txt (configure)
    ↓
    └─→ Python3_EXECUTABLE + gen_i18n_keys_constexpr.py
         ↓
         ├─ 输入：i18n/en_US.json, i18n/zh_CN.json
         ├─ 处理：扁平化，生成强类型和编译期函数
         └─ 输出：build/generated/i18n_keys.h
    
build (compile phase)
    ↓
    ├─→ src/ (includes generated/i18n_keys.h)
    ├─→ tools/ (reads from generated/i18n_keys.h)
    └─→ test/ (uses generated types and functions)
```

## ⚙️ 自定义配置

### 修改生成脚本
如果需要使用不同的脚本，编辑 CMakeLists.txt：

```cmake
# 改为使用旧脚本（仅向后兼容）
COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys.py
        ${CMAKE_SOURCE_DIR}/i18n/en_US.json
        ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
```

### 添加新的翻译文件

如需支持更多语言（如 fr_FR.json），更新依赖：

```cmake
DEPENDS ${CMAKE_SOURCE_DIR}/i18n/en_US.json
        ${CMAKE_SOURCE_DIR}/i18n/zh_CN.json
        ${CMAKE_SOURCE_DIR}/i18n/fr_FR.json    # ← 新增
        ${CMAKE_SOURCE_DIR}/tools/gen_i18n_keys_constexpr.py
```

### 自定义输出位置（不推荐）

```cmake
set(I18N_KEYS_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/my_custom_location/i18n_keys.h)

add_custom_command(
    OUTPUT ${I18N_KEYS_OUTPUT}
    ...
)
```

## 🐛 调试

### 查看生成的文件
```bash
# Windows
type build/generated/i18n_keys.h | head -100

# Linux/macOS
head -100 build/generated/i18n_keys.h
```

### 强制重新生成
```bash
# 删除构建目录重新配置
rm -rf build
cmake -S . -B build
cmake --build build --config Release
```

### 查看生成过程
```bash
# 详细 CMake 输出
cmake --build build --config Release --verbose

# 或者直接运行脚本
cd build/generated
python ../../tools/gen_i18n_keys_constexpr.py
```

## 📋 使用生成的 Header

### C++ 代码使用

```cpp
#include "i18n_keys.h"
using namespace I18nVault;

// 编译期使用
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();

// 运行时使用（向后兼容）
auto key = I18nKey::LOGIN_BUTTON;
```

### 包含路径

生成的文件位置：
- **编译时**：`${CMAKE_BINARY_DIR}/generated/i18n_keys.h`
- **安装后**：（不直接安装，用户需要运行脚本）

## 🚀 最佳实践

### 1. 始终依赖版本控制中的 JSON 文件
```bash
# ✅ 版本控制
git add i18n/*.json

# ❌ 不版本控制生成的文件
git add -f build/generated/  # 不要这样做
```

### 2. CI/CD 中自动生成
```yaml
# GitHub Actions
- name: Build
  run: |
    cmake -S . -B build
    cmake --build build --config Release
    # gen_i18n_keys_constexpr.py 自动运行
```

### 3. 本地开发中修改 JSON 后重新编译
```bash
# 1. 编辑翻译
vim i18n/en_US.json

# 2. 重新编译（自动重新生成）
cmake --build build --config Release

# 3. 使用新的 Key 类型编写代码
```

## 🔧 故障排除

### 问题：Python 脚本找不到

**解决：** 确保 Python3 被正确检测
```bash
# 查看 CMake 配置
cmake -S . -B build --fresh
# 应该看到 "Python3_EXECUTABLE" 信息
```

### 问题：生成的文件不看起来对

**解决：** 检查 JSON 文件格式
```bash
# 验证 JSON 语法
python -m json.tool i18n/en_US.json
```

### 问题：增量编译没有重新生成

**解决：** 修改 CMakeLists.txt 时需要重新配置
```bash
# 不仅仅是重新编译
cmake -S . -B build --fresh
cmake --build build --config Release
```

## 📊 性能特性

- **脚本执行时间**：~100-200ms（生成单个 header）
- **增量：**~10-20ms（依赖检查）
- **首次构建**：触发生成（包含脚本运行）
- **后续构建**：仅在依赖变化时重新生成

## 📚 相关文档

- [CMakeLists.txt](../../CMakeLists.txt) - 完整配置
- [gen_i18n_keys_constexpr.py](../../tools/gen_i18n_keys_constexpr.py) - 生成脚本
- [COMPILE_TIME_I18N.md](./COMPILE_TIME_I18N.md) - 编译期 i18n 指南
- [QUICK_START.md](../QUICK_START.md) - 快速开始

---

## ✅ 总结

CMake 现在自动化了编译期 i18n header 的生成，确保：

✅ **自动更新** - JSON 文件变化时自动重新生成
✅ **依赖追踪** - CMake 理解所有依赖关系
✅ **增量构建** - 仅在需要时重新生成
✅ **清晰的输出** - 详细的编译消息
✅ **无缝集成** - 与 CI/CD 完全兼容

开发者现在只需编辑翻译文件，编译时一切都会自动处理！ 🚀
