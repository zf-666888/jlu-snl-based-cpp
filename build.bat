@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl.exe /EHsc /std:c++17 /Fe:snl_compiler.exe src\main.cpp src\scan.cpp src\tree.cpp src\parse.cpp src\analyze.cpp /I src
