"""Exercise only F4SEPlugin_Query; never invoke Load, ENB, or engine addresses."""
import ctypes
from pathlib import Path

class QueryInterface(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint32) for name in
                ('f4seVersion', 'runtimeVersion', 'editorVersion', 'isEditor')]

class PluginInfo(ctypes.Structure):
    _fields_ = [('infoVersion', ctypes.c_uint32), ('name', ctypes.c_char_p),
                ('version', ctypes.c_uint32)]

root = Path(__file__).resolve().parents[1]
plugin = ctypes.CDLL(str(root / 'release/ENBInputDisablerFO4.dll'))
query = plugin.F4SEPlugin_Query
query.argtypes = [ctypes.POINTER(QueryInterface), ctypes.POINTER(PluginInfo)]
query.restype = ctypes.c_bool
expected_version = (ctypes.c_uint32 * 2).in_dll(plugin, 'F4SEPlugin_Version')[1]

for minor, patch, expected in [(10, 163, True), (11, 221, True), (11, 240, True),
                                (10, 168, False), (10, 984, False), (11, 241, False)]:
    interface = QueryInterface(0, (1 << 24) | (minor << 16) | (patch << 4), 0, 0)
    info = PluginInfo()
    assert query(ctypes.byref(interface), ctypes.byref(info)) == expected
    assert info.infoVersion == 1 and info.name == b'ENBInputDisablerFO4'
    assert info.version == expected_version
    interface.isEditor = 1
    assert not query(ctypes.byref(interface), ctypes.byref(info))

assert not query(None, ctypes.byref(PluginInfo()))
assert not query(ctypes.byref(QueryInterface()), None)
print('PASS: DLL Query accepts the three runtimes, rejects unsupported/editor/null requests, and reports its exported version')
