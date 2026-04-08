# CMake 集成快速参考

## 🚀 简单步骤

### 第一次编译
```bash
cmake -S . -B build
cmake --build build --config Release
# ✅ 自动生成 build/generated/i18n_keys.h
```

### 修改翻译后
```bash
# 编辑 i18n/en_US.json 或 i18n/zh_CN.json
vim i18n/en_US.json

# 重新编译（自动重新生成 i18n_keys.h）
cmake --build build --config Release
```

### 使用编译期 i18n
```cpp
#include "i18n_keys.h"

// 编译期翻译（零开销）
constexpr auto text = i18n<Key_LOGIN_BUTTON, Lang_en_US>();
```

---

## 📊 完整流程

```
1️⃣ 编辑翻译
   └─ i18n/en_US.json
   └─ i18n/zh_CN.json

2️⃣ 运行 CMake
   └─ cmake -S . -B build
   └─ 检测 Python 和依赖

3️⃣ 编译项目
   └─ cmake --build build
   └─ CMake 自动运行 Python 脚本
   └─ 生成 build/generated/i18n_keys.h

4️⃣ Key 和语言类型可用
   └─ Key_LOGIN_BUTTON
   └─ Lang_en_US
   └─ i18n<...>() 模板函数

5️⃣ 代码开发
   └─ 使用生成的强类型 Key
```

---

## ✨ 自动化特性

### 依赖检测
```cmake
DEPENDS
  i18n/en_US.json
  i18n/zh_CN.json
  tools/gen_i18n_keys_constexpr.py
```

当这些文件变化时，自动重新运行脚本。

### 输出处理
```cmake
OUTPUT ${CMAKE_BINARY_DIR}/generated/i18n_keys.h
```

生成的文件自动被后续目标使用。

### 工作目录
```cmake
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
```

脚本在项目根目录运行，保证路径正确。

---

## 🔧 常见操作

### 查看编译输出
```bash
cmake --build build --config Release --verbose
```

### 看到脚本运行
```
Generating i18n_keys.h with compile-time i18n support...
[OK] Header generated: build/generated/i18n_keys.h
[OK] Supported languages: ['en_US', 'zh_CN']
```

### 强制重新生成
```bash
rm -rf build
cmake -S . -B build
cmake --build build --config Release
```

### 验证生成的文件
```cpp
// build/generated/i18n_keys.h 包含

// ✅ 强类型 Key
struct Key_LOGIN_BUTTON { ... };

// ✅ 语言标签
struct Lang_en_US { ... };

// ✅ 编译期翻译
template <typename KeyType, typename LangType>
constexpr std::string_view i18n() { ... }
```

---

## 📋 CMakeLists.txt 改动

### 新增部分
```cmake
# 自动生成编译期 i18n header
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
add_custom_target(i18n_keys ALL DEPENDS ${CMAKE_BINARY_DIR}/generated/i18n_keys.h)
```

### 前提条件
```cmake
cmake_minimum_required(VERSION 3.15)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
```

---

## 🎯 效果

### Before（旧方式）
```bash
# 需要手动运行
python tools/gen_i18n_keys.py i18n/en_US.json build/generated/i18n_keys.h

# 然后再编译
cmake --build build
```

### After（新方式）
```bash
# 直接编译，脚本自动运行
cmake --build build

# ✅ 脚本自动执行
# ✅ 依赖自动追踪
# ✅ 增量构建支持
```

---

## ⚠️ 注意事项

### JSON 文件必须存在
```bash
# ✅ 需要这些文件
i18n/en_US.json
i18n/zh_CN.json

# ❌ 否则脚本失败
```

### Python 3 必须可用
```bash
# 验证 Python
python --version  # 或 python3

# CMake 会检测并报告
find_package(Python3 REQUIRED COMPONENTS Interpreter)
```

### 增量重新生成
```bash
# ✅ 修改 JSON 后自动重新生成
vim i18n/en_US.json
cmake --build build

# ❌ 不需要手动删除
rm build/generated/i18n_keys.h  # 不必要
```

---

## 🔗 相关命令

### 重新配置
```bash
cmake -S . -B build --fresh
```

### 详细输出
```bash
cmake --build build --config Release --verbose
```

### 查看依赖
```bash
cmake --build build --target help
# 会列出 i18n_keys target
```

### 仅生成 i18n_keys
```bash
cmake --build build --target i18n_keys
```

---

## 📊 编译时间影响

- **Python 脚本运行**：~100-200ms
- **首次完整构建**：+200ms
- **增量构建（无改动）**：+0ms（跳过脚本）
- **增量构建（JSON 改动）**：+200ms（重新运行脚本）

---

## ✅ 验证一切工作

```bash
# 1. 编译
cmake --build build --config Release

# 2. 查看输出
# 应该看到：
# Generating i18n_keys.h with compile-time i18n support...
# [OK] Header generated: build/generated/i18n_keys.h

# 3. 运行测试
ctest --test-dir build -C Release

# 4. 应该看到
# Test #1: i18n_manager_smoke ... Passed
# 100% tests passed
```

---

## 🎓 总结

CMake 现在：

✅ 自动执行 Python 脚本
✅ 追踪所有依赖
✅ 支持增量构建
✅ 生成编译期 i18n header
✅ 一切透明且高效

开发者只需 `cmake --build`，其余的都自动处理！ 🚀
