# Diff Check & TRS Safety 快速开始

## 🚀 30秒快速上手

### 检查翻译质量

```bash
# 验证所有翻译文件
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 输出：
# [OK] i18n validation PASSED
```

### 验证 TRS 文件安全

```bash
# 验证 TRS 文件
python tools/trs_safety_manager.py validate i18n/en_US.trs

# 输出：
# [OK] TRS file is valid: i18n/en_US.trs
```

---

## 📋 工作流

### 开发流程

```
1️⃣ 编辑翻译
   └─ 修改 i18n/*.json

2️⃣ 本地验证
   └─ python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

3️⃣ 如果通过
   └─ Commit & Push

4️⃣ GitHub 自动运行检查
   └─ ✅ 所有检查通过
```

### CI/CD 自动执行

打开 GitHub Actions，看到以下检查自动运行：

```
✅ i18n Validation
   • Enhanced diff check
   • Validation report

✅ Build & Test
   • Build (Release)
   • Tests passed

✅ TRS Safety Check
   • TRS validation
   • Upgrade compatibility

✅ Code Quality
   • Python linting

✅ Security Analysis
   • Secrets detection
```

---

## 🛠️ 常见命令

### 基础验证

```bash
# 严格验证（推荐）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 生成报告
python tools/i18n_diff_check_enhanced.py i18n/en_US.json > report.txt

# GitHub Annotations（CI）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict --github-annotations
```

### TRS 文件检查

```bash
# 验证 TRS 文件
python tools/trs_safety_manager.py validate i18n/en_US.trs

# 验证所有 TRS 文件
python tools/trs_safety_manager.py validate i18n/*.trs

# 生成完整报告
python tools/trs_safety_manager.py generate-report i18n/*.trs
```

---

## ✅ 检查项

| 项目 | 检查内容 | 失败等级 |
|------|--------|--------|
| 缺失 Key | 翻译缺少 Key | ❌ 错误 |
| 多余 Key | 多出的 Key | ⚠️ 警告 |
| 断开占位符 | {0} 不匹配 | ❌ 错误 |
| 类型错误 | Key 值类型不一致 | ❌ 错误 |
| 空值 | 空字符串翻译 | ❌ 错误 |
| 异常长度 | 太短或太长 | ⚠️ 警告 |
| 控制字符 | NUL、SOH 等 | ❌ 错误 |
| TRS 完整性 | 文件损坏 | ❌ 错误 |

---

## 🎯 添加新翻译

### 完整步骤

```bash
# 1. 编辑 i18n/en_US.json 和 i18n/zh_CN.json
# 新增：
# {
#   "NEW_KEY": "New value"
# }

# 2. 验证
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 3. 重新生成 i18n_keys.h
python tools/gen_i18n_keys_constexpr.py

# 4. 编译
cmake --build build --config Release

# 5. 测试
ctest --test-dir build

# 6. Commit
git add i18n/*.json build/generated/i18n_keys.h
git commit -m "feat: add new translations"
git push
```

---

## 🔍 查看 GitHub Actions 结果

### 在 PR 中看到的内容

1. **Checks 选项卡** → 看到所有检查的状态
2. **Annotations** → 鼠标悬停查看详细错误位置
3. **Artifacts** → 下载验证报告

### 错误示例

```
❌ i18n validation FAILED

Missing key: dialog.confirm
   in: i18n/zh_CN.json

Generate template? → download i18n/zh_CN.missing.template.json
```

---

## 📊 报告示例

### 验证成功

```
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

### TRS 验证成功

```
[OK] TRS version: 1
[OK] TRS structure valid

[OK] TRS file is valid: i18n/en_US.trs
```

---

## ⚙️ 配置

### 严格模式

```bash
# ❌ 不严格（警告不会失败）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json

# ✅ 严格模式（任何警告都是错误）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict
```

### GitHub Annotations

```bash
# 本地运行不显示 Annotations
python tools/i18n_diff_check_enhanced.py i18n/en_US.json

# CI 中显示 Annotations（直接在 PR 中看到错误）
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --github-annotations
```

---

## 💡 技巧

### 检查缺失的 key

```bash
# 找出 zh_CN 缺失的 key
python tools/i18n_diff_check_enhanced.py i18n/en_US.json 2>&1 | grep "Missing"

# 会看到缺失哪些 key
# [ERROR] Missing key: dialog.confirm
# [ERROR] Missing key: menu.file
# ...
```

### 自动生成缺失 key 模板

```bash
# 运行后会生成 i18n/zh_CN.missing.template.json
python tools/i18n_diff_check_enhanced.py i18n/en_US.json --strict

# 编辑模板文件中的值，然后合并到实际文件中
cat i18n/zh_CN.missing.template.json
```

### 查看详细的 TRS 信息

```bash
# 基础验证输出版本信息
python tools/trs_safety_manager.py validate i18n/en_US.trs

# [OK] TRS version: 1
# [OK] TRS structure valid
```

---

## 🚨 常见问题

### 问：CI 失败了怎么办？

**答：** 
1. 查看 GitHub Actions 的错误日志
2. 在本地运行相同命令重现问题
3. 修复后重新 push

### 问：如何跳过某次检查？

**答：** 不推荐，但可以临时禁用
```bash
# 不要这样做！
git push --force
```

### 问：验证报告在哪里？

**答：** GitHub Actions → Artifacts 中可以下载
```
i18n-validation-report/i18n_validation_report.txt
trs-validation-report/trs_validation_report.txt
```

---

## 📚 更多信息

详细文档：[DIFF_CI_TRS_GUIDE.md](DIFF_CI_TRS_GUIDE.md)

---

## ✨ 总结

现在你有了：

✅ **自动翻译质量检查**
- 本地验证
- CI/CD 集成
- GitHub Annotations

✅ **TRS 文件安全保障**
- 完整性验证
- 版本管理
- 升级保护

✅ **完整工作流**
- Push → GitHub Actions 自动运行
- 错误直接显示在 PR 中
- 完全自动化

开发翻译时，不需要担心质量问题！ 🎯
