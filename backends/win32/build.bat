call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cl /Zi /FC /I../.. win32_stretchdib_main.c /Fe:stretchDIB_win32.exe user32.lib gdi32.lib
cl /Zi /FC /I../.. win32_bitblt_main.c /I../.. /Fe:bitblt_win32.exe user32.lib gdi32.lib
pause
