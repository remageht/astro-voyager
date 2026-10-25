import ctypes, os
from ctypes import wintypes

class STARTUPINFOW(ctypes.Structure):
    _fields_ = [
        ('cb', wintypes.DWORD), ('lpReserved', wintypes.LPWSTR), ('lpDesktop', wintypes.LPWSTR),
        ('lpTitle', wintypes.LPWSTR), ('dwX', wintypes.DWORD), ('dwY', wintypes.DWORD),
        ('dwXSize', wintypes.DWORD), ('dwYSize', wintypes.DWORD), ('dwXCountChars', wintypes.DWORD),
        ('dwYCountChars', wintypes.DWORD), ('dwFillAttribute', wintypes.DWORD), ('dwFlags', wintypes.DWORD),
        ('wShowWindow', wintypes.WORD), ('cbReserved2', wintypes.WORD), ('lpReserved2', ctypes.c_void_p),
        ('hStdInput', wintypes.HANDLE), ('hStdOutput', wintypes.HANDLE), ('hStdError', wintypes.HANDLE),
    ]

class PROCESS_INFORMATION(ctypes.Structure):
    _fields_ = [
        ('hProcess', wintypes.HANDLE), ('hThread', wintypes.HANDLE),
        ('dwProcessId', wintypes.DWORD), ('dwThreadId', wintypes.DWORD),
    ]

si = STARTUPINFOW()
si.cb = ctypes.sizeof(STARTUPINFOW)
si.lpDesktop = 'winsta0\\default'
pi = PROCESS_INFORMATION()

target_file = os.path.abspath('blender/out/ingame_station_check.png')
if os.path.exists(target_file):
    os.remove(target_file)

cwd = os.path.abspath('.')
cmd = f'cmd.exe /c "{os.path.abspath("build-gl/Release/astro-voyager.exe")}" --capture-station blender/out/ingame_station_check.png > capture.log 2>&1'
print("Spawning:", cmd)
res = ctypes.windll.kernel32.CreateProcessW(None, cmd, None, None, False, 0, None, cwd, ctypes.byref(si), ctypes.byref(pi))
if res:
    ctypes.windll.kernel32.WaitForSingleObject(pi.hProcess, 15000)
    exit_code = wintypes.DWORD()
    ctypes.windll.kernel32.GetExitCodeProcess(pi.hProcess, ctypes.byref(exit_code))
    print('Exit code:', hex(exit_code.value))
    ctypes.windll.kernel32.CloseHandle(pi.hProcess)
    ctypes.windll.kernel32.CloseHandle(pi.hThread)
    if os.path.exists(target_file):
        print(f"Success: {target_file} created ({os.path.getsize(target_file)} bytes)")
    else:
        print(f"Error: {target_file} was not created!")
else:
    print('CreateProcess failed:', ctypes.get_last_error())
