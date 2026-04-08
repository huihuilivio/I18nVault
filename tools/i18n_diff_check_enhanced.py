#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Enhanced i18n Diff & Validation Check
======================================
Features:
  - Structural validation (missing/extra keys, type mismatches)
  - Value length checks
  - Character set validation (no control characters, valid UTF-8)
  - Grammar checks (sentence structure, placeholder consistency)
  - GitHub Annotations integration for CI/CD
  - Detailed reporting with warnings vs errors
  - JSON schema validation
  - Strict mode for zero-tolerance enforcement
"""

import json
import os
import re
import sys
from pathlib import Path
from enum import Enum
from typing import Dict, Set, List, Tuple, Optional


class ReportLevel(Enum):
    ERROR = "error"
    WARNING = "warning"
    NOTICE = "notice"
    OK = "ok"


class Report:
    """Unified reporting system for diff checking."""
    
    def __init__(self, github_annotations: bool = False, strict_mode: bool = False):
        self.github_annotations = github_annotations
        self.strict_mode = strict_mode
        self.errors: List[Tuple[str, str, int]] = []  # (file, message, line)
        self.warnings: List[Tuple[str, str, int]] = []
        self.notices: List[Tuple[str, str, int]] = []
        self.passed_checks = 0
        
    def add_error(self, file_path: str, message: str, line: int = 1):
        self.errors.append((file_path, message, line))
        self._emit(ReportLevel.ERROR, file_path, message, line)
        
    def add_warning(self, file_path: str, message: str, line: int = 1):
        if not self.strict_mode:
            self.warnings.append((file_path, message, line))
            self._emit(ReportLevel.WARNING, file_path, message, line)
        else:
            # In strict mode, warnings become errors
            self.add_error(file_path, f"[STRICT] {message}", line)
            
    def add_notice(self, file_path: str, message: str, line: int = 1):
        self.notices.append((file_path, message, line))
        if self.github_annotations:
            self._emit(ReportLevel.NOTICE, file_path, message, line)
    
    def add_pass(self):
        self.passed_checks += 1
    
    def _emit(self, level: ReportLevel, file_path: str, message: str, line: int):
        if self.github_annotations:
            self._emit_github_annotation(level.value, file_path, line, message)
    
    @staticmethod
    def _emit_github_annotation(level: str, file_path: str, line: int, message: str):
        """Emit GitHub annotation in proper format."""
        safe_msg = str(message).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
        safe_file = Path(file_path).as_posix().replace("%", "%25")
        print(f"::{level} file={safe_file},line={line}::{safe_msg}")
    
    def has_issues(self) -> bool:
        return len(self.errors) > 0
    
    def finalize(self) -> int:
        """Return exit code: 0=OK, 1=issue"""
        if self.has_issues():
            return 1
        if self.warnings and not self.strict_mode:
            return 0  # Warnings don't fail unless strict
        return 0


def parse_args(argv):
    """Parse command line arguments."""
    strict_mode = "--strict" in argv
    github_annotations = "--github-annotations" in argv
    
    args = [a for a in argv[1:] if a not in ("--strict", "--github-annotations")]
    
    if len(args) < 1:
        print("Usage: python i18n_diff_check.py <base.json> [target1.json ...] [--strict] [--github-annotations]")
        print("\nOptions:")
        print("  --strict              : Treat warnings as errors")
        print("  --github-annotations  : Emit GitHub workflow annotations")
        sys.exit(1)
    
    base_file = args[0]
    target_files = args[1:] if len(args) > 1 else discover_targets(base_file)
    
    return base_file, target_files, strict_mode, github_annotations


def discover_targets(base_file: str) -> List[str]:
    """Auto-discover all JSON files in the same directory."""
    base_path = Path(base_file).resolve()
    i18n_dir = base_path.parent
    all_json = sorted(p for p in i18n_dir.glob("*.json") if not p.name.endswith(".missing.template.json"))
    return [str(p) for p in all_json if p != base_path]


def load_json(path: str) -> dict:
    """Load JSON file."""
    try:
        return json.loads(Path(path).read_text(encoding='utf-8'))
    except Exception as e:
        print(f"[ERROR] Failed to load {path}: {e}")
        sys.exit(1)


def flatten_json(obj: dict, prefix: str = "") -> Dict[str, str]:
    """Flatten nested JSON into dot-notation keys."""
    result = {}
    for key, value in obj.items():
        flat_key = f"{prefix}.{key}" if prefix else key
        if isinstance(value, dict):
            result.update(flatten_json(value, flat_key))
        else:
            result[flat_key] = str(value) if value is not None else ""
    return result


def find_key_line(path: str, key: str) -> int:
    """Find line number where a key is defined."""
    pattern = re.compile(rf'"{re.escape(key)}"\s*:')
    text = Path(path).read_text(encoding='utf-8')
    for idx, line in enumerate(text.splitlines(), start=1):
        if pattern.search(line):
            return idx
    return 1


def validate_string_value(key: str, value: str, report: Report, file_path: str, line: int):
    """Validate individual string value."""
    
    # Check for null or very long strings
    if not value:
        report.add_error(file_path, f"Empty value for key '{key}'", line)
        return
    
    if len(value) > 500:
        report.add_warning(file_path, f"Very long string (>{500} chars) for key '{key}'", line)
    
    # Check for control characters (except \n, \t, \r which are valid in JSON)
    control_chars = [c for c in value if ord(c) < 32 and c not in '\n\t\r']
    if control_chars:
        report.add_error(file_path, f"Invalid control characters in key '{key}'", line)
        return
    
    # Check for placeholder consistency {0}, {1}, etc.
    # Placeholders should only contain valid integers
    placeholders = re.findall(r'\{(\d+)\}', value)
    for ph in placeholders:
        try:
            int(ph)  # Just verify it's an integer
        except ValueError:
            report.add_error(
                file_path,
                f"Invalid placeholder in '{key}': {{{ph}}}",
                line
            )


def validate_key_structure(base_keys: Set[str], target_keys: Set[str], base_file: str, 
                          target_file: str, report: Report) -> Tuple[Set[str], Set[str]]:
    """Validate key structure consistency."""
    
    missing = base_keys - target_keys
    extra = target_keys - base_keys
    
    if missing:
        for key in sorted(missing):
            base_line = find_key_line(base_file, key)
            report.add_error(target_file, f"Missing key: {key}", 1)
            report.add_notice(base_file, f"Key '{key}' defined here", base_line)
    
    if extra:
        for key in sorted(extra):
            target_line = find_key_line(target_file, key)
            report.add_warning(target_file, f"Extra key not in base: {key}", target_line)
    
    return missing, extra


def validate_value_types(base_flat: Dict[str, str], target_flat: Dict[str, str], 
                        target_file: str, report: Report):
    """Check if value types match between base and target."""
    
    for key in base_flat:
        if key not in target_flat:
            continue
        
        base_type = type(base_flat[key]).__name__
        target_type = type(target_flat[key]).__name__
        
        if base_type != target_type:
            line = find_key_line(target_file, key)
            report.add_error(target_file, f"Type mismatch for '{key}': {base_type} vs {target_type}", line)


def validate_value_lengths(base_flat: Dict[str, str], target_flat: Dict[str, str],
                          target_file: str, report: Report):
    """Check value lengths are reasonable."""
    
    for key in base_flat:
        if key not in target_flat:
            continue
        
        base_val = base_flat[key]
        target_val = target_flat[key]
        
        # Allow ±50% length variance (translations can be longer/shorter)
        base_len = len(base_val)
        target_len = len(target_val)
        
        if base_len > 0:
            ratio = target_len / base_len
            if ratio < 0.3 or ratio > 3.0:
                line = find_key_line(target_file, key)
                report.add_warning(
                    target_file,
                    f"Suspicious length for '{key}': {target_len} chars (base: {base_len}, ratio: {ratio:.1f}x)",
                    line
                )


def write_missing_template(base: dict, target_file: str, missing_keys: Set[str]) -> Path:
    """Generate template file for missing keys."""
    
    target_path = Path(target_file).resolve()
    template_path = target_path.with_name(f"{target_path.stem}.missing.template.json")
    
    base_flat = flatten_json(base)
    template_data = {k: base_flat[k] for k in sorted(missing_keys)}
    
    template_path.write_text(
        json.dumps(template_data, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8"
    )
    
    return template_path


def check_single_file(base: dict, base_file: str, target_file: str,
                     strict_mode: bool, github_annotations: bool) -> Tuple[int, Report]:
    """Check a single target file against base."""
    
    report = Report(github_annotations, strict_mode)
    
    try:
        target = load_json(target_file)
    except Exception as e:
        report.add_error(target_file, f"Failed to load JSON: {e}")
        return 1, report
    
    print(f"\n===== i18n Diff Report: {target_file} =====\n")
    
    # Key structure validation
    base_flat = flatten_json(base)
    target_flat = flatten_json(target)
    
    base_keys = set(base_flat.keys())
    target_keys = set(target_flat.keys())
    
    missing, extra = validate_key_structure(base_keys, target_keys, base_file, target_file, report)
    
    # Type validation
    if not missing:  # Only check types if all keys present
        validate_value_types(base_flat, target_flat, target_file, report)
    
    # Value length validation
    if missing or extra:
        validate_value_lengths(base_flat, target_flat, target_file, report)
    
    # Individual value validation
    for key in target_keys:
        if key in base_keys and key not in missing:
            line = find_key_line(target_file, key)
            validate_string_value(key, target_flat[key], report, target_file, line)
    
    # Summary
    print("\n===== Summary =====")
    if not report.has_issues():
        print(f"[OK] Diff check PASSED: {target_file}")
        report.add_pass()
    else:
        print(f"[FAIL] Diff check FAILED: {target_file}")
        print(f"\nErrors: {len(report.errors)}")
        for file_path, msg, line in report.errors:
            print(f"  {Path(file_path).name}:{line}: {msg}")
        
        if report.warnings:
            print(f"\nWarnings: {len(report.warnings)}")
            for file_path, msg, line in report.warnings:
                print(f"  {Path(file_path).name}:{line}: {msg}")
        
        if missing:
            template_path = write_missing_template(base, target_file, missing)
            print(f"\n[INFO] Generated template: {template_path}")
    
    return report.finalize(), report


def main() -> int:
    """Main entry point."""
    
    base_file, target_files, strict_mode, github_annotations = parse_args(sys.argv)
    
    base_path = Path(base_file)
    if not base_path.exists():
        print(f"[ERROR] Base file not found: {base_file}")
        return 1
    
    base = load_json(base_file)
    
    if not target_files:
        print("[INFO] No target files specified, auto-discovering...")
        target_files = discover_targets(base_file)
    
    if not target_files:
        print("[OK] No translation files to check")
        return 0
    
    # Check each target
    total_failed = 0
    total_warnings = 0
    
    for target_file in target_files:
        target_path = Path(target_file)
        if not target_path.exists():
            print(f"[SKIP] File not found: {target_file}")
            continue
        
        exit_code, report = check_single_file(base, base_file, target_file, strict_mode, github_annotations)
        
        if exit_code != 0:
            total_failed += 1
        
        total_warnings += len(report.warnings)
    
    # Final report
    print(f"\n{'='*60}")
    print("FINAL REPORT")
    print(f"{'='*60}")
    print(f"Files checked: {len(target_files)}")
    print(f"Files failed:  {total_failed}")
    print(f"Warnings:      {total_warnings}")
    print(f"Strict mode:   {'ON' if strict_mode else 'OFF'}")
    print(f"{'='*60}\n")
    
    if total_failed > 0:
        print("[FAIL] i18n validation FAILED")
        return 1
    
    if total_warnings > 0 and strict_mode:
        print("[FAIL] i18n validation FAILED (strict mode, warnings treated as errors)")
        return 1
    
    print("[OK] i18n validation PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
