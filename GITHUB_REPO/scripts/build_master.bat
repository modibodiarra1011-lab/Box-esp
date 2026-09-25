@echo off
setlocal
call "C:\esp\v6.1\esp-idf\export.bat" || exit /b 1
cd /d "%~dp0..\firmware\master" || exit /b 1
idf.py set-target esp32s3 || exit /b 1
idf.py reconfigure || exit /b 1
idf.py build || exit /b 1
