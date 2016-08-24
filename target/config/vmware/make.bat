@echo off
rem Command line build environments
set WIND_HOST_TYPE=x86-win32
set WIND_BASE=C:\Tornado2.2.1_x86
set PATH=%WIND_BASE%\host\%WIND_HOST_TYPE%\bin;%PATH%
objcopypentium -O binary --gap-fill=0 bootrom bootrom.sys
