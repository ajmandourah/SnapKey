@echo off

mkdir build > nul
cd build > nul
echo [+] Preparing files...
cmake .. > nul
echo [+] Compiling...
cmake --build . 
cd .. > nul
move "build\Debug\SnapKey.exe" "." > nul
echo [+] Done!
rmdir /s /q build > nul
echo [+] Press a key to exit...
pause > nul