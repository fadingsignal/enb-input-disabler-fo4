"""Verify the built release without loading the DLL or starting Fallout 4."""
import hashlib
from pathlib import Path
import struct
import re
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "build/python-deps"))
import pefile

version = re.search(r'set_version\("(\d+)\.(\d+)\.(\d+)"\)', (root / 'xmake.lua').read_text()).groups()
version_string = '.'.join(version)
major, minor, patch = map(int, version)

dll = (root / "dist/F4SE/Plugins/ENBInputDisablerFO4.dll").read_bytes()
pe = pefile.PE(data=dll)
assert pe.FILE_HEADER.Machine == 0x8664
exports = {s.name.decode(): s.address for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
assert set(exports) == {"F4SEPlugin_Load", "F4SEPlugin_Query", "F4SEPlugin_Version"}, exports
metadata = pe.get_data(exports["F4SEPlugin_Version"], 0x45C)
assert struct.unpack_from("<II", metadata) == (1, (major << 24) | (minor << 16) | (patch << 4))
assert metadata[8:264].split(b"\0")[0] == b"ENBInputDisablerFO4"
assert struct.unpack_from("<II", metadata, 0x208) == (4, 4)
versions = struct.unpack_from("<16I", metadata, 0x210)
assert versions == ((1 << 24) | (10 << 16) | (163 << 4),
                    (1 << 24) | (11 << 16) | (221 << 4),
                    (1 << 24) | (11 << 16) | (240 << 4), *([0] * 13)), versions
with zipfile.ZipFile(root / f"build/ENBInputDisablerFO4-{version_string}.zip") as archive:
    names = {n.replace("\\", "/"): n for n in archive.namelist()}
    assert archive.read(names["F4SE/Plugins/ENBInputDisablerFO4.dll"]) == dll
    assert archive.read(names["F4SE/Plugins/ENBInputDisablerFO4.ini"]) == (root / "res/ENBInputDisablerFO4.ini").read_bytes()
    assert "README.md" in names and "LICENSE" in names and "EXCEPTIONS.md" in names
    assert not any(n.lower().endswith((".exe", ".bin")) for n in names)
    expected_files = {"F4SE/Plugins/ENBInputDisablerFO4." + ext for ext in ("dll", "pdb", "ini")}
    assert {n for n in names if not n.endswith('/')} == expected_files | {"README.md", "LICENSE", "EXCEPTIONS.md"}
    for ext in ('dll', 'pdb', 'ini'):
        filename = f'ENBInputDisablerFO4.{ext}'
        assert archive.read(names[f'F4SE/Plugins/{filename}']) == (root / 'release' / filename).read_bytes()
print("PASS: x64 DLL, F4SE exports, exact runtime list, Address Library metadata, ZIP contents")
print("DLL SHA256", hashlib.sha256(dll).hexdigest())
