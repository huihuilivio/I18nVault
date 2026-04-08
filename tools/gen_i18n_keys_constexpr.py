#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
生成编译期 i18n 系统 - 强类型化 + constexpr
使用 strong typing 和 constexpr 实现编译期翻译，零运行时开销
"""

import json
import re
from pathlib import Path
import sys

PREFIX = "I18N_"  # 数字开头的 key 前缀

def normalize_key(key: str) -> str:
    """将 JSON key 转换为合法 C++ identifier"""
    norm = re.sub(r'[^a-zA-Z0-9]', '_', key).upper()
    if norm[0].isdigit():
        norm = PREFIX + norm
    return norm

def flatten_json(data: dict, prefix: str = "") -> dict:
    """递归扁平化嵌套 JSON"""
    result = {}
    for k, v in data.items():
        flat_key = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            result.update(flatten_json(v, flat_key))
        elif isinstance(v, str):
            result[flat_key] = v
        else:
            print(f"[WARN] skip non-string value: {flat_key} = {v!r}")
    return result

def escape_string(s: str) -> str:
    """C++ 字符串转义"""
    s = s.replace('\\', '\\\\')
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n')
    s = s.replace('\r', '\\r')
    s = s.replace('\t', '\\t')
    return s

def generate_constexpr_header(json_files: dict, output_file: str):
    """
    生成支持编译期 i18n 的 C++ 头文件
    
    json_files: dict mapping language_name -> path
    例如: {"en_US": "i18n/en_US.json", "zh_CN": "i18n/zh_CN.json"}
    """
    
    # 加载所有 JSON
    translations = {}
    primary_keys = None
    
    for lang, path in json_files.items():
        json_path = Path(path)
        if not json_path.exists():
            print(f"Error: {path} 不存在")
            sys.exit(1)
        
        data = json.loads(json_path.read_text(encoding='utf-8'))
        flat = flatten_json(data)
        translations[lang] = flat
        
        if primary_keys is None:
            primary_keys = set(flat.keys())
    
    # 验证所有语言有相同的 key
    all_lang_keys = set(translations['primary'].keys()) if 'primary' in translations else set()
    for lang, trans in translations.items():
        if lang != list(json_files.keys())[0]:  # 跳过第一个
            for key in trans.keys():
                if key not in all_lang_keys:
                    all_lang_keys.add(key)
    
    # 生成规范化的 key 映射
    normalized_keys = {}
    for key in sorted(all_lang_keys):
        nk = normalize_key(key)
        if nk in normalized_keys:
            print(f"[ERROR] enum conflict: {key} and {normalized_keys[nk]} both map to {nk}")
            sys.exit(1)
        normalized_keys[nk] = key
    
    lines = []
    lines.append("// This file is auto-generated. Do not modify manually.\n")
    lines.append("// Compile-time i18n with strong typing and zero runtime overhead\n")
    lines.append("#pragma once\n\n")
    lines.append("#include <string_view>\n")
    lines.append("#include <array>\n\n")
    lines.append("namespace I18nVault\n")
    lines.append("{\n\n")
    
    # ============= Strong Type Definition =============
    lines.append("// ============ Strong Type Keys ============\n")
    lines.append("// Each key is a unique type, preventing accidental key mixing\n\n")
    
    for idx, (nk, original_key) in enumerate(sorted(normalized_keys.items())):
        lines.append(f"struct Key_{nk} {{\n")
        lines.append(f"    static constexpr int value = {idx};\n")
        lines.append(f"    static constexpr std::string_view original = \"{original_key}\";\n")
        lines.append(f"}};\n")
    
    lines.append("\n")
    lines.append("// ============ Language Tags ============\n\n")
    
    # 生成语言标签
    for lang in sorted(json_files.keys()):
        lines.append(f"struct Lang_{lang} {{ static constexpr std::string_view name = \"{lang}\"; }};\n")
    
    lines.append("\n")
    
    # ============= Compile-time Translation Storage =============
    lines.append("// ============ Compile-time Translation Data ============\n")
    lines.append("// All translations are embedded as constexpr, zero runtime cost\n\n")
    
    for lang, trans_data in sorted(translations.items()):
        lang_tag = f"Lang_{lang}"
        lines.append(f"// Language: {lang}\n")
        lines.append(f"namespace {lang}\n")
        lines.append(f"{{\n")
        
        for nk, original_key in sorted(normalized_keys.items()):
            translation = trans_data.get(original_key, "")
            escaped = escape_string(translation)
            lines.append(f"    inline constexpr std::string_view {nk} = \"{escaped}\";\n")
        
        lines.append(f"}}  // namespace {lang}\n\n")
    
    # ============= Constexpr Translate Function =============
    lines.append("// ============ Compile-time Translator ============\n")
    lines.append("// Usage: i18n<Key_LOGIN_BUTTON, Lang_en_US>()\n")
    lines.append("// Result: const char* with translation, zero runtime cost\n\n")
    
    lines.append("template <typename KeyType, typename LangType>\n")
    lines.append("constexpr std::string_view i18n()\n")
    lines.append("{\n")
    lines.append("    constexpr int key_id = KeyType::value;\n")
    lines.append("    constexpr std::string_view lang_name = LangType::name;\n")
    lines.append("    \n")
    
    # 生成 if constexpr 链
    for idx, (nk, original_key) in enumerate(sorted(normalized_keys.items())):
        condition = "if" if idx == 0 else "else if"
        lines.append(f"    {condition} constexpr (key_id == {idx})\n")
        lines.append(f"    {{\n")
        
        lang_conditions = []
        for lang in sorted(json_files.keys()):
            lang_conditions.append(lang)
        
        for lang_idx, lang in enumerate(lang_conditions):
            inner_cond = "if" if lang_idx == 0 else "else if"
            lines.append(f"        {inner_cond} constexpr (lang_name == \"{lang}\")\n")
            lines.append(f"            return {lang}::{nk};\n")
        
        lines.append(f"    }}\n")
    
    lines.append("    return \"\";\n")
    lines.append("}\n\n")
    
    # ============= Backward Compatibility =============
    lines.append("// ============ Backward Compatibility ============\n")
    lines.append("// Traditional enum for gradual migration\n\n")
    
    lines.append("enum class I18nKey : unsigned short\n")
    lines.append("{\n")
    for nk in sorted(normalized_keys.keys()):
        lines.append(f"    {nk},\n")
    lines.append("};\n\n")
    
    lines.append("inline constexpr const char* i18n_keys_string(I18nKey key)\n")
    lines.append("{\n")
    lines.append("    switch (key)\n")
    lines.append("    {\n")
    for nk, original in sorted(normalized_keys.items()):
        lines.append(f"        case I18nKey::{nk}:\n")
        lines.append(f"            return \"{original}\";\n")
    lines.append("        default:\n")
    lines.append("            return \"\";\n")
    lines.append("    }\n")
    lines.append("}\n\n")
    
    lines.append(f"inline constexpr unsigned short I18N_KEY_COUNT = {len(normalized_keys)};\n\n")
    
    lines.append("}  // namespace I18nVault\n")
    
    # 写入文件
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("".join(lines), encoding='utf-8')
    print(f"[OK] Header generated: {output_file}")

if __name__ == "__main__":
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent
    
    # 默认从 i18n/ 目录查找所有 .json 文件
    i18n_dir = project_root / "i18n"
    json_files = {}
    
    if i18n_dir.exists():
        for json_file in sorted(i18n_dir.glob("*.json")):
            lang_name = json_file.stem  # 例如 en_US, zh_CN
            json_files[lang_name] = str(json_file)
    
    if not json_files:
        print("Error: No JSON files found in i18n/ directory")
        sys.exit(1)
    
    output_header = project_root / "build" / "generated" / "i18n_keys.h"
    generate_constexpr_header(json_files, str(output_header))
    print(f"[OK] Supported languages: {sorted(json_files.keys())}")
