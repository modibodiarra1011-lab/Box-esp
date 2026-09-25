@echo off
setlocal
call C:\esp\v6.1\export.bat
cd /d %~dp0..\firmware\master
idf.py set-target esp32s3
idf.py build
idf.py -p %ESPPORT% flash monitor
