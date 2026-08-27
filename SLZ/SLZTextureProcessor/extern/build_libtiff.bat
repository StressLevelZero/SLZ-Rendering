cd libtiff
CALL "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
CALL cmake -G "Visual Studio 17 2022" -B build -T ClangCl -Dtiff-static=ON -Dtiff-tests=OFF -Dtiff-contrib=OFF -Dtiff-docs=OFF -Dtiff-install=OFF
PAUSE