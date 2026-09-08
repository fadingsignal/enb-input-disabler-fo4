"""Read-only ControlMap audit. Requires pefile and capstone; use an unpacked EXE.

python tools/audit-runtime.py Fallout4.exe version-1-11-240-0.bin
"""
import argparse
import hashlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "build/python-deps"))
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64


def audit(executable, database):
    pe = pefile.PE(str(executable), fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY[name] for name in
        ["IMAGE_DIRECTORY_ENTRY_RESOURCE", "IMAGE_DIRECTORY_ENTRY_IMPORT", "IMAGE_DIRECTORY_ENTRY_EXCEPTION"]])
    vi = pe.VS_FIXEDFILEINFO[0]
    version = (vi.FileVersionMS >> 16, vi.FileVersionMS & 65535,
               vi.FileVersionLS >> 16, vi.FileVersionLS & 65535)
    profiles = {
        (1, 10, 163, 0): (325206, 425955, 0x1B29B20),
        (1, 11, 221, 0): (4799307, 2268327, 0x1670807),
        (1, 11, 240, 0): (4799307, 2268327, 0x1670B27),
    }
    if version not in profiles:
        raise ValueError(f"Unsupported executable {version}")
    expected = "version-" + "-".join(map(str, version)) + ".bin"
    if database.name != expected:
        raise ValueError(f"Expected matching {expected}")
    raw = database.read_bytes()
    if len(raw) < 8 or len(raw) != 8 + struct.unpack_from("<Q", raw)[0] * 16:
        raise ValueError("Invalid F4SE v0 Address Library size")
    entries = list(struct.iter_unpack("<QQ", raw[8:]))
    if any(a[0] >= b[0] for a, b in zip(entries, entries[1:])):
        raise ValueError("Unsorted or duplicate Address Library IDs")
    ids = dict(entries)
    singleton_id, ctor_id, gate_rva = profiles[version]
    singleton, ctor = ids[singleton_id], ids[ctor_id]
    base = pe.OPTIONAL_HEADER.ImageBase
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True
    instructions = list(md.disasm(pe.get_data(ctor, 0x120), base + ctor))
    # Constructor stores RCX (this) into the Address-Library-selected singleton.
    stores = [i for i in instructions if i.mnemonic == "mov" and
              i.op_str.startswith("qword ptr [rip +") and i.op_str.endswith(", rcx")]
    if not any(i.address + i.size + i.operands[0].mem.disp == base + singleton for i in stores):
        raise ValueError("Constructor does not reference the resolved singleton (packed EXE?)")
    if not any(i.mnemonic == "mov" and i.op_str == "word ptr [r14 + 0x140], 0" for i in instructions):
        raise ValueError("Constructor no longer initializes the expected flag layout")
    section = pe.get_section_by_rva(singleton)
    if section is None or not section.Characteristics & 0x80000000:
        raise ValueError("Singleton is not in a writable PE section")
    gate = list(md.disasm(pe.get_data(gate_rva, 24), base + gate_rva))
    expected_reg = "rbx" if version[1] == 10 else "rdi"
    device_reg = "edi" if version[1] == 10 else "esi"
    expected_ops = [("cmp", f"byte ptr [{expected_reg} + 0x141], 0"),
                    ("je", None), ("cmp", f"{device_reg}, 1"),
                    ("ja", None), ("cmp", "edx, -1"), ("sete", "al")]
    if len(gate) < len(expected_ops) or any(
        i.mnemonic != mnemonic or (operand is not None and i.op_str != operand)
        for i, (mnemonic, operand) in zip(gate, expected_ops)
    ):
        raise ValueError("Keyboard/mouse suppression predicate changed")
    clip_imports = [i.address for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports
                    if i.name == b"ClipCursor" and d.dll.lower() == b"user32.dll"]
    if len(clip_imports) != 1:
        raise ValueError("Missing or ambiguous game ClipCursor import")
    loop_id = 847266 if version[1] == 10 else 2228915
    loop_rva = ids[loop_id]
    loop_end = next(e.struct.EndAddress for e in pe.DIRECTORY_ENTRY_EXCEPTION
                    if e.struct.BeginAddress == loop_rva)
    loop = list(md.disasm(pe.get_data(loop_rva, loop_end-loop_rva), base+loop_rva))
    clip_calls = [i for i in loop if i.mnemonic == "call" and i.op_str.startswith("qword ptr [rip +")
                  and i.address + i.size + i.operands[0].mem.disp == clip_imports[0]]
    if len(clip_calls) != 2:
        raise ValueError("Expected temporary ClipCursor + restoration in message loop")
    factors = [struct.unpack("<f", pe.get_data(i.address + i.size + i.operands[1].mem.disp - base, 4))[0]
               for i in loop if i.mnemonic == "mulss" and "[rip +" in i.op_str]
    if 0.25 not in factors or -0.25 not in factors:
        raise ValueError("Centered gameplay cursor bounds changed")
    print(f"PASS cursor path: message-loop ID {loop_id} RVA {loop_rva:#x}; "
          f"ClipCursor IAT RVA {clip_imports[0]-base:#x}; temporary center clamp and restoration")
    print(f"PASS {'.'.join(map(str, version))}: singleton ID {singleton_id} RVA {singleton:#x}; "
          f"constructor ID {ctor_id} RVA {ctor:#x}; flag +0x141; keyboard/mouse gate {gate_rva:#x}")
    print("EXE SHA256", hashlib.sha256(executable.read_bytes()).hexdigest())
    print("Address Library SHA256", hashlib.sha256(raw).hexdigest())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("database", type=Path)
    args = parser.parse_args()
    audit(args.executable, args.database)
