@echo off
echo Change COM7 value to Your Environment
echo Flash Size = 4MB, Partition Scheme = No OTA(1MB APP/ 3MB SPIFFS)
echo ESP32 Board manager Arduino ESP32 Version 2.0.7

if not exist %LocalAppData%\Arduino15\packages\esp32\ echo === Error === & exit /b

if exist SPIFFS.bin del SPIFFS.bin

rem %LocalAppData% = C:\Users\{User Name}\AppData\Local
rem %LocalAppData%\Arduino15\packages\esp32\tools\mkspiffs\0.2.3\mkspiffs.exe
setlocal
pushd %LocalAppData%\Arduino15\packages\esp32\tools\mkspiffs\
for /f "usebackq delims=" %%A in (`dir /S /A-D /B mkspiffs.exe`) do set mkspiffs=%%A
popd
echo %mkspiffs%
%mkspiffs% ^
  -c .\data ^
  -s 0x2e0000 ^
  SPIFFS.bin

if not exist SPIFFS.bin echo === Error === & exit /b

dir SPIFFS.bin

rem %LocalAppData%\Arduino15\packages\esp32\tools\esptool_py\4.5\esptool.exe
pushd %LocalAppData%\Arduino15\packages\esp32\tools\esptool_py\
for /f "usebackq delims=" %%A in (`dir /S /A-D /B esptool.exe`) do set esptool=%%A
popd
echo %esptool%
%esptool% ^
  -p COM7 ^
  -b 921600 ^
  write_flash ^
  -ff 80m ^
  -fm dio ^
  0x00110000 ^
  SPIFFS.bin

