@echo off
echo Derleme basladi...

gcc *.c -o main.exe

if %errorlevel% neq 0 (
echo Derleme hatasi!
pause
exit /b
)

echo Program calisiyor...
main.exe
