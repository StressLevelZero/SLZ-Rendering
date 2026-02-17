CALL "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
CALL cmake -G "Visual Studio 17 2022" -B build-windows -T ClangCl
PAUSE