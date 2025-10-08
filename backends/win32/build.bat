call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cl main.c -Fe:a.exe user32.lib gdi32.lib
pause
