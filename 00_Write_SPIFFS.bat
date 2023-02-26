@echo off
echo Change COM7 value to Your Environment
echo Flash Size = 4MB, Partition Scheme = No OTA(1MB APP/ 3MB SPIFFS)
rem %LocalAppData% = C:\Users\{User Name}\AppData\Local
%LocalAppData%\Arduino15\packages\esp32\tools\mkspiffs\0.2.3\mkspiffs.exe ^
  -c .\data ^
  -s 0x2e0000 ^
  SPIFFS.bin

%LocalAppData%\Arduino15\packages\esp32\tools\esptool_py\4.2.1\esptool.exe ^
  -p COM7 ^
  -b 921600 ^
  write_flash ^
  -ff 80m ^
  -fm dio ^
  0x00110000 ^
  SPIFFS.bin
