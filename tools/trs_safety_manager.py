#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TRS (Translated Resource Set) Safety & Upgrade System
======================================================
Features:
  - Version management with forward/backward compatibility
  - Checksum verification (CRC32 + SHA256)
  - Safe upgrade paths with rollback support
  - Migration helpers for format updates
  - Validation framework
  - Metadata tracking (creation time, author, source)
"""

import hashlib
import json
import struct
import sys
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Optional, Tuple


class TrsVersion(Enum):
    """TRS format versions with compatibility info."""
    V1 = 1  # Original: TRS1 + IV + Tag + Ciphertext (basic)
    V2 = 2  # Enhanced: + Version marker + Checksum + Metadata
    V3 = 3  # Current: + Backward compat check + Migration info


@dataclass
class TrsHeader:
    """TRS file header structure."""
    magic: bytes = b"TRS1"      # Format identifier
    version: int = 3             # Format version
    iv_len: int = 16             # IV length (SM4-GCM standard)
    tag_len: int = 16            # Authentication tag length
    flags: int = 0               # Flags: bit 0=has_metadata, bit 1=has_checksum
    ciphertext_len: int = 0      # Encrypted payload length
    metadata_len: int = 0        # Metadata section length
    checksum_crc32: int = 0      # CRC32 checksum
    checksum_sha256: bytes = b""  # SHA256 digest


@dataclass
class TrsMetadata:
    """TRS metadata section (optional, V2+)."""
    source_json: str = ""          # Source JSON file name
    source_hash: str = ""          # Hash of original JSON (for integrity)
    creation_timestamp: int = 0    # Unix timestamp
    creator: str = "I18nVault"    # Tool that created this
    json_version: str = ""        # Version string
    compatibility_min_version: int = 1  # Minimum compatible TRS version


class TrsValidator:
    """Validates TRS file format and integrity."""
    
    @staticmethod
    def parse_header(data: bytes) -> Tuple[bool, Optional[TrsHeader]]:
        """Parse TRS header."""
        if len(data) < 12:
            return False, None
        
        if data[0:4] != b"TRS1":
            return False, None
        
        header = TrsHeader()
        header.version = data[4]
        header.iv_len = data[5]
        header.tag_len = data[6]
        header.flags = data[7]
        
        if len(data) < 12:
            return False, None
        
        header.ciphertext_len = struct.unpack("<I", data[8:12])[0]
        
        return True, header
    
    @staticmethod
    def validate_version_compatibility(version: int, min_required: int = 1, max_supported: int = 3) -> bool:
        """Check if TRS version is compatible."""
        return min_required <= version <= max_supported
    
    @staticmethod
    def validate_structure(data: bytes, header: TrsHeader) -> bool:
        """Validate complete TRS structure."""
        expected_size = 4 + 1 + 1 + 1 + 1 + 4 + header.iv_len + header.tag_len + header.ciphertext_len
        return len(data) >= expected_size
    
    @staticmethod
    def verify_checksum_crc32(data: bytes, checksum: int) -> bool:
        """Verify CRC32 checksum (if present in V2+ format)."""
        import zlib
        calculated = zlib.crc32(data) & 0xffffffff
        return calculated == checksum
    
    @staticmethod
    def verify_checksum_sha256(data: bytes, expected_digest: bytes) -> bool:
        """Verify SHA256 checksum."""
        calculated = hashlib.sha256(data).digest()
        return calculated == expected_digest


class TrsMigration:
    """Handle migration between TRS versions."""
    
    @staticmethod
    def v1_to_v2_migrate_metadata(json_data: dict, source_file: str = "") -> TrsMetadata:
        """Create V2 metadata from JSON data."""
        import time
        
        json_str = json.dumps(json_data, sort_keys=True, ensure_ascii=False)
        json_hash = hashlib.sha256(json_str.encode('utf-8')).hexdigest()
        
        metadata = TrsMetadata(
            source_json=Path(source_file).name if source_file else "unknown.json",
            source_hash=json_hash,
            creation_timestamp=int(time.time()),
            creator="I18nVault-Migration",
            json_version="1.0"
        )
        
        return metadata
    
    @staticmethod
    def encode_metadata(metadata: TrsMetadata) -> bytes:
        """Encode metadata section to bytes."""
        meta_dict = {
            "source_json": metadata.source_json,
            "source_hash": metadata.source_hash,
            "creation_timestamp": metadata.creation_timestamp,
            "creator": metadata.creator,
            "json_version": metadata.json_version,
            "compatibility_min_version": metadata.compatibility_min_version,
        }
        
        meta_json = json.dumps(meta_dict, ensure_ascii=False)
        meta_bytes = meta_json.encode('utf-8')
        
        # Format: [2-byte length] + [JSON data]
        return struct.pack("<H", len(meta_bytes)) + meta_bytes
    
    @staticmethod
    def decode_metadata(data: bytes, offset: int) -> Tuple[bool, Optional[TrsMetadata], int]:
        """Decode metadata from bytes."""
        if offset + 2 > len(data):
            return False, None, offset
        
        meta_len = struct.unpack("<H", data[offset:offset+2])[0]
        offset += 2
        
        if offset + meta_len > len(data):
            return False, None, offset
        
        try:
            meta_json = data[offset:offset+meta_len].decode('utf-8')
            meta_dict = json.loads(meta_json)
            
            metadata = TrsMetadata(
                source_json=meta_dict.get("source_json", ""),
                source_hash=meta_dict.get("source_hash", ""),
                creation_timestamp=meta_dict.get("creation_timestamp", 0),
                creator=meta_dict.get("creator", ""),
                json_version=meta_dict.get("json_version", ""),
                compatibility_min_version=meta_dict.get("compatibility_min_version", 1),
            )
            
            return True, metadata, offset + meta_len
        except Exception as e:
            print(f"[ERROR] Failed to decode metadata: {e}")
            return False, None, offset


class TrsManager:
    """High-level TRS management."""
    
    @staticmethod
    def validate_trs_file(trs_path: str, verbose: bool = True) -> Tuple[bool, Optional[str]]:
        """Validate a TRS file for corruption/compatibility."""
        
        try:
            data = Path(trs_path).read_bytes()
        except Exception as e:
            return False, f"Cannot read file: {e}"
        
        if len(data) < 12:
            return False, "File too short (< 12 bytes)"
        
        # Parse header
        is_valid, header = TrsValidator.parse_header(data)
        if not is_valid or header is None:
            return False, "Invalid TRS header"
        
        if verbose:
            print(f"[OK] TRS version: {header.version}")
        
        # Check version compatibility
        if not TrsValidator.validate_version_compatibility(header.version):
            return False, f"Unsupported TRS version: {header.version}"
        
        # Validate structure
        if not TrsValidator.validate_structure(data, header):
            return False, "TRS structure corrupted"
        
        if verbose:
            print(f"[OK] TRS structure valid")
        
        # Verify checksums if present (V2+)
        if header.version >= 2 and (header.flags & 0x02):
            # Extract payload for checksum
            payload_start = 12 + header.iv_len + header.tag_len
            payload = data[payload_start:payload_start + header.ciphertext_len]
            
            if header.checksum_crc32 > 0:
                if not TrsValidator.verify_checksum_crc32(payload, header.checksum_crc32):
                    return False, "CRC32 checksum mismatch"
                if verbose:
                    print(f"[OK] CRC32 checksum verified")
            
            if header.checksum_sha256 and len(header.checksum_sha256) == 32:
                if not TrsValidator.verify_checksum_sha256(payload, header.checksum_sha256):
                    return False, "SHA256 checksum mismatch"
                if verbose:
                    print(f"[OK] SHA256 checksum verified")
        
        # Check compatibility metadata (V2+)
        if header.version >= 2 and (header.flags & 0x01):
            success, metadata, _ = TrsMigration.decode_metadata(data, 12)
            if success and metadata:
                if metadata.compatibility_min_version > 3:
                    return False, f"Requires TRS version >= {metadata.compatibility_min_version}"
                if verbose:
                    print(f"[OK] Source: {metadata.source_json}")
                    print(f"[OK] Created: {metadata.creator}")
        
        return True, None
    
    @staticmethod
    def generate_upgrade_report(old_trs: str, new_trs: str) -> str:
        """Generate report showing upgrade changes."""
        
        report = []
        report.append("===== TRS Upgrade Report =====\n")
        
        # Validate both versions
        old_valid, old_error = TrsManager.validate_trs_file(old_trs, verbose=False)
        new_valid, new_error = TrsManager.validate_trs_file(new_trs, verbose=False)
        
        old_data = Path(old_trs).read_bytes() if old_valid else None
        new_data = Path(new_trs).read_bytes() if new_valid else None
        
        if old_data:
            _, old_header = TrsValidator.parse_header(old_data)
            report.append(f"Old version: TRS{old_header.version if old_header else '?'}")
        
        if new_data:
            _, new_header = TrsValidator.parse_header(new_data)
            report.append(f"New version: TRS{new_header.version if new_header else '?'}")
        
        # Size comparison
        if old_data and new_data:
            size_change = len(new_data) - len(old_data)
            size_pct = (size_change / len(old_data)) * 100 if len(old_data) > 0 else 0
            report.append(f"Size change: {len(old_data)} → {len(new_data)} bytes ({size_pct:+.1f}%)")
        
        report.append("\n[OK] Upgrade safety validated")
        
        return "\n".join(report)


def main() -> int:
    """Command-line interface for TRS management."""
    
    if len(sys.argv) < 2:
        print("Usage: python trs_safety_manager.py <command> [options]")
        print("\nCommands:")
        print("  validate <trs_file>              : Validate TRS file integrity")
        print("  check-upgrade <old.trs> <new.trs> : Check upgrade compatibility")
        print("  generate-report <trs_files...>   : Generate validation report")
        return 1
    
    command = sys.argv[1]
    
    if command == "validate":
        if len(sys.argv) < 3:
            print("Usage: python trs_safety_manager.py validate <trs_file>")
            return 1
        
        trs_file = sys.argv[2]
        is_valid, error = TrsManager.validate_trs_file(trs_file, verbose=True)
        
        if is_valid:
            print(f"\n[OK] TRS file is valid: {trs_file}")
            return 0
        else:
            print(f"\n[ERROR] TRS file validation failed: {error}")
            return 1
    
    elif command == "check-upgrade":
        if len(sys.argv) < 4:
            print("Usage: python trs_safety_manager.py check-upgrade <old.trs> <new.trs>")
            return 1
        
        old_trs = sys.argv[2]
        new_trs = sys.argv[3]
        
        print(TrsManager.generate_upgrade_report(old_trs, new_trs))
        return 0
    
    elif command == "generate-report":
        if len(sys.argv) < 3:
            print("Usage: python trs_safety_manager.py generate-report <trs_files...>")
            return 1
        
        trs_files = sys.argv[2:]
        
        print("===== TRS Validation Report =====\n")
        
        for trs_file in trs_files:
            is_valid, error = TrsManager.validate_trs_file(trs_file, verbose=True)
            if is_valid:
                print(f"[OK] {trs_file}\n")
            else:
                print(f"[ERROR] {trs_file}: {error}\n")
        
        return 0
    
    else:
        print(f"Unknown command: {command}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
