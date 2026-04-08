# i18n Diff Check & CI 强校验系统

## 📋 概览

实现了两个强大的系统确保翻译质量和安全性：

### ✅ 1. 增强的 Diff 检查系统

**特性：**
- 🔍 **结构验证**：检查缺失、多余、类型不匹配的 Key
- ✍️ **内容验证**：值的长度、字符集、控制字符检查
- 🎯 **占位符验证**：确保 {0}、{1} 等参数替换标记一致
- 📊 **详细报告**：分类输出错误、警告、通知
- 🔔 **GitHub Annotations**：直接在 GitHub Actions 中显示错误位置
- 🛑 **严格模式**：将所有警告升级为错误

### ✅ 2. TRS 安全升级系统

**特性：**
- 🔐 **版本管理**：从 V1 升级到 V3 的完整支持
- ✔️ **校验和验证**：CRC32 + SHA256 双重验证
- 📝 **元数据跟踪**：来源、创建时间、兼容性标记
- 🔄 **迁移支持**：安全的版本升级路径
- 📋 **升级报告**：详细的变化说明

---

## 🔧 使用方式

### I. 增强的 Diff 检查

#### 基础用法
```bash
# 检查单个文件
python tools/i18n_diff_check_enhanced.py i18n/en_US.json

# 自动发现所有 JSON 文件
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 使用 GitHub Annotations（CI/CD）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json \
  --strict \
  --github-annotations
```

#### 检查项目

| 检查项 | 说明 | 级别 |
|------|------|------|
| 缺失 Key | 翻译中缺少基础文件中的 Key | ❌ 错误 |
| 多余 Key | 翻译中有基础文件没有的 Key | ⚠️ 警告 |
| 类型不匹配 | Key 值的类型不一致 | ❌ 错误 |
| 空值 | 翻译为空字符串 | ❌ 错误 |
| 长度异常 | 翻译长度异常（<30% 或 >300%） | ⚠️ 警告 |
| 占位符 | 参数标记 {0}, {1} 检查 | ❌ 错误 |
| 控制字符 | 非法控制字符（NUL、SOH 等） | ❌ 错误 |
| 无效 UTF-8 | 编码问题 | ❌ 错误 |

#### 输出示例

```bash
===== i18n Diff Report: i18n/zh_CN.json =====

===== Summary =====
[OK] Diff check PASSED: i18n/zh_CN.json

============================================================
FINAL REPORT
============================================================
Files checked: 1
Files failed:  0
Warnings:      0
Strict mode:   OFF
============================================================

[OK] i18n validation PASSED
```

### II. TRS 安全管理

#### 验证 TRS 文件
```bash
# 基础验证
python tools/trs_safety_manager.py validate i18n/en_US.trs

# 输出示例
[OK] TRS version: 1
[OK] TRS structure valid
[OK] TRS file is valid: i18n/en_US.trs
```

#### 检查升级兼容性
```bash
# 检查从旧版本升级到新版本的兼容性
python tools/trs_safety_manager.py check-upgrade i18n/en_US_old.trs i18n/en_US_new.trs

# 输出示例
===== TRS Upgrade Report =====
Old version: TRS1
New version: TRS2
Size change: 2048 → 2156 bytes (+5.3%)

[OK] Upgrade safety validated
```

#### 生成验证报告
```bash
# 生成所有 TRS 文件的验证报告
python tools/trs_safety_manager.py generate-report i18n/*.trs

# 输出：
===== TRS Validation Report =====

[OK] i18n/en_US.trs
[OK] i18n/zh_CN.trs
```

---

## 🚀 CI/CD 集成

### GitHub Actions Workflow

新的增强工作流文件：`.github/workflows/enhanced_i18n_ci.yml`

#### 工作流包含的检查

```yaml
1. i18n-validation
   ├─ Enhanced diff check (strict + annotations)
   └─ Validation report generation

2. build-and-test
   ├─ Build (Release)
   ├─ Run tests (ctest)
   └─ Verify artifacts

3. trs-safety-check
   ├─ Verify TRS files
   ├─ Validate integrity
   ├─ Check upgrade compatibility
   └─ Generate TRS report

4. lint-and-analysis
   ├─ Python linting
   └─ Syntax checking

5. security-check
   ├─ Hardcoded secrets detection
   └─ Encryption keys verification

6. report-summary
   └─ Generate GitHub summary
```

#### 工作流触发条件

```yaml
on:
  push:              # 每次 push 时运行
  pull_request:      # PR 时运行
  workflow_dispatch: # 手动触发
```

---

## 📊 检查流程图

```
GitHub Push/PR
    ↓
i18n Validation
├─ 1️⃣ Enhanced diff check
│  ├─ 检查结构 (keys, types)
│  ├─ 检查内容 (length, charset)
│  └─ 检查格式 (placeholders)
│
├─ 2️⃣ GitHub Annotations
│  └─ 在 PR 中显示错误位置
│
└─ Failed? → Exit with error

Build & Test
├─ CMake build (Release)
├─ Run tests
└─ Verify artifacts

TRS Safety
├─ 3️⃣ Validate TRS files
│  ├─ 检查 magic 和版本
│  ├─ 验证校验和
│  └─ 检查元数据
│
├─ 4️⃣ Upgrade check
│  └─ 兼容性验证
│
└─ Failed? → Exit with error

Final Report
└─ Summary to GitHub
```

---

## 🔐 TRS 格式升级路线

### V1 → V2 → V3 升级路径

```
TRS V1 (Original)
├─ Magic: TRS1
├─ Version: 1
└─ Content: IV + Tag + Ciphertext

  ↓ Upgrade

TRS V2 (Enhanced)
├─ Magic: TRS1
├─ Version: 2
├─ Flags: [metadata|checksum]
├─ Content: IV + Tag + Ciphertext + Metadata
└─ Metadata: JSON with source, hash, timestamp

  ↓ Upgrade

TRS V3 (Current)
├─ Magic: TRS1
├─ Version: 3
├─ Flags: [metadata|checksum]
├─ Content: IV + Tag + Ciphertext + Metadata
└─ Metadata: + compatibility_min_version field
```

### 向后兼容性

- ✅ V3 可以读取 V1、V2
- ✅ V2 可以读取 V1
- ✅ V1 只能读 V1
- ⚠️ 旧版本无法读取新版本（需要升级）

---

## 📈 工作流日志示例

### GitHub Actions 日志

```
Run: python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict --github-annotations

===== i18n Diff Report: i18n/zh_CN.json =====

===== Summary =====
[OK] Diff check PASSED: i18n/zh_CN.json

============================================================
FINAL REPORT
============================================================
Files checked: 1
Files failed:  0
Warnings:      0
Strict mode:   ON
============================================================

[OK] i18n validation PASSED
```

### GitHub Annotations（显示在 PR 中）

```
❌ File: i18n/zh_CN.json, Line 3
   Missing key: dialog.delete_fmt

⚠️ File: i18n/zh_CN.json, Line 5
   Suspicious length for 'WELCOME_FMT': 18 chars (base: 25, ratio: 0.7x)

ℹ️ File: i18n/en_US.json, Line 6
   Key 'dialog.delete_fmt' defined here
```

---

## 🎯 最佳实践

### 1. 本地验证（开发）

```bash
# 提交前验证
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 验证 TRS 文件
python tools/trs_safety_manager.py validate i18n/*.trs

# 只有都通过才 push
```

### 2. CI/CD 验证（自动）

```yaml
# 在 CI 中启用严格模式
- name: Strict i18n validation
  run: |
    python tools/i18n_diff_check_enhanced.py i18n/en_US.json \
      --strict \
      --github-annotations
```

### 3. 添加新翻译

```bash
# 1. 编辑 i18n/*.json 文件

# 2. 本地验证
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 3. 重新生成 i18n_keys.h
python tools/gen_i18n_keys_constexpr.py

# 4. 编译和测试
cmake --build build --config Release
ctest --test-dir build

# 5. Commit & Push
git add -A
git commit -m "feat: add new translations"
git push
# → GitHub Actions 自动运行所有检查
```

---

## ⚙️ 配置选项

### Enhanced Diff Check

```bash
# 严格模式：将所有警告升级为错误
--strict

# GitHub Annotations：在 CI/CD 中显示错误位置
--github-annotations

# 自动发现目标文件，无需指定
python tools/i18n_diff_check_enhanced.py i18n/en_US.json
```

### TRS Safety Manager

```bash
# 验证单个 TRS 文件
python tools/trs_safety_manager.py validate i18n/*.trs

# 检查升级兼容性
python tools/trs_safety_manager.py check-upgrade old.trs new.trs

# 生成完整报告
python tools/trs_safety_manager.py generate-report i18n/*.trs
```

---

## 📝 示例：完整 PR 流程

### Step 1: 编辑翻译文件

```json
// i18n/fr_FR.json
{
  "LOGIN_BUTTON": "Connexion",
  "WELCOME_FMT": "Bienvenue, {0}!",
  ...
}
```

### Step 2: 本地验证

```bash
$ python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

===== i18n Diff Report: i18n/fr_FR.json =====

===== Summary =====
[OK] Diff check PASSED: i18n/fr_FR.json

[OK] i18n validation PASSED
```

### Step 3: Commit & Push

```bash
$ git add i18n/fr_FR.json
$ git commit -m "feat: add French translation"
$ git push origin feature/french-translation
```

### Step 4: GitHub Actions 自动运行

```
✅ i18n Validation         (2s)
  ├─ Enhanced diff check   [PASSED]
  └─ Validation report     [OK]

✅ Build & Test           (30s)
  ├─ Build (Release)      [OK]
  ├─ Tests               [PASSED: 42/42]
  └─ Artifacts verified  [OK]

✅ TRS Safety Check       (5s)
  ├─ TRS validation      [OK]
  ├─ Upgrade check       [OK]
  └─ Report generated    [OK]

✅ Code Quality           (3s)
✅ Security Check         (2s)
✅ Report Summary         [Generated]
```

### Step 5: PR 显示结果

在 PR 的 Checks 选项卡中显示所有检查结果。

---

## 🔍 故障排除

### Q: 收到"Missing key"错误

**解决：** 检查所有翻译文件是否都有这个 key

```bash
# 查找缺失的 key
grep -L "YOUR_KEY" i18n/*.json

# 添加缺失的 key 到所有文件
```

### Q: "Inconsistent placeholders"

**解决：** 确保参数占位符一致

```json
// ❌ 错误
"WELCOME": "Hello {0} and {1}!"
"WELCOME_FR": "Bonjour {0}!"  // 缺少 {1}

// ✅ 正确
"WELCOME": "Hello {0} and {1}!"
"WELCOME_FR": "Bonjour {0} et {1}!"
```

### Q: TRS 文件验证失败

**解决：** 重新生成 TRS 文件

```bash
# 查看详细错误
python tools/trs_safety_manager.py validate i18n/en_US.trs

# 重新生成（需要构建）
cmake --build build --config Release
```

---

## 📚 文件列表

### 新脚本
- `tools/i18n_diff_check_enhanced.py` - 增强的 diff 检查
- `tools/trs_safety_manager.py` - TRS 安全管理

### 新工作流
- `.github/workflows/enhanced_i18n_ci.yml` - 增强的 CI/CD

### 生成的报告
- `i18n_validation_report.txt` - 差异检查报告
- `trs_validation_report.txt` - TRS 安全报告

---

## 🎯 总结

现在项目有了：

✅ **自动化质量检查**
- 结构验证
- 内容验证
- GitHub Annotations 集成

✅ **安全升级系统**
- 版本管理
- 校验和验证
- 兼容性检查

✅ **完整 CI/CD 流程**
- 自动验证
- 详细报告
- GitHub 集成

所有翻译现在都受到强大的保护机制的保护，确保质量和安全性！ 🔒
