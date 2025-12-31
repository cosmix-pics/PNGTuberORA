@echo off
cl.exe nob.c /Fe:nob.exe /nologo
if %errorlevel% neq 0 exit /b %errorlevel%
nob.exe
