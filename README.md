# ESP32_BadApple_MP3

* ESP32で東方の Bad Apple!!の動画を 128 x 64 dotの OLED SSD1306で再生する！  
http://www.neko.ne.jp/~freewing/hardware/espressif_esp32_bad_apple_demo/  

* Caution ! Not committing MP3 files  
I have not committed the MP3 file due to copyright infringement .  
Place it in the data directory with the file name of "ba.mp3" by yourself .  
"ba.mp3" MP3 Encoding Layer-3 Lame 22050HZ 40kbps Mono, duration 3:39.220, 1096379 bytes  

[<img src="https://img.youtube.com/vi/B9TrOxXer4E/maxresdefault.jpg" alt="Touhou Bad Apple!! Demo ESP32 with MP3 Audio and SSD1306 OLED (128x64 dot)" title="Touhou Bad Apple!! Demo ESP32 with MP3 Audio and SSD1306 OLED (128x64 dot)" width="320" height="180"> YouTube https://youtu.be/B9TrOxXer4E](https://youtu.be/B9TrOxXer4E)  

# ESP32 I2S Audio Schematics
No DAC  
![No DAC](https://raw.githubusercontent.com/FREEWING-JP/ESP32_BadApple_MP3/feature/add_mp3_function/esp32_mp3_audio_no_dac_schematics.png)  
MAX98357  
![MAX98357](https://raw.githubusercontent.com/FREEWING-JP/ESP32_BadApple_MP3/feature/add_mp3_function/esp32_mp3_audio_i2s_dac_max98357_schematics.png)  
CS4344  
![CS4344](https://raw.githubusercontent.com/FREEWING-JP/ESP32_BadApple_MP3/feature/add_mp3_function/esp32_mp3_audio_i2s_dac_cs4344_schematics.png)  

# Upload SPIFFS via command line

See this commit [ba59205](https://github.com/FREEWING-JP/ESP32_BadApple_MP3/commit/ba5920535c691e5fde63bcc52ccd41607e39a3ea)  
00_Write_SPIFFS.bat  

---
# ESP32_BadApple
Bad Apple video by Touhou on ESP32 with SSD1306 OLED, uses the Heatshrink compression library to decompress the RLE encoded video data.
First version, no sound yet, video only.

![Bad Apple on ESP32](ESP32_BadApple.jpg)

## Hardware Requirements
Runs on ESP32 modules with OLED displays based on SSD1306. Typical modules are available from Heltec or TTGO or others.

## Software Requirements
* Arduino 1.8.x
* ESP32 Arduino core from https://github.com/espressif/arduino-esp32
* OLED display driver from https://github.com/ThingPulse/esp8266-oled-ssd1306
* ESP32 SPIFFS Upload Plugin from https://github.com/me-no-dev/arduino-esp32fs-plugin

# Usage
* Adapt display pins in main sketch if necessary
* Upload sketch
* Upload sketch data via "Tools" -> "ESP32 Sketch Data Upload"

Enjoy video. Pressing PRG button (GPIO0) for max display speed (mainly limited by I2C transfer), otherwise limited to 30 fps.

# How does it work
Video have been separated into >6500 single pictures, resized to 128x64 pixels using VLC. 
Python skript used to run-length encode the 8-bit-packed data using 0x55 and 0xAA as escape marker and putting all into one file.
RLE file has been further compressed using heatshrink compression for easy storage into SPIFFS (which can hold only 1MB by default). 
Heatshrink for Arduino uses ZIP-like algorithm and is available also as a library under https://github.com/p-v-o-s/Arduino-HScompression and 
original documentation is here: https://spin.atomicobject.com/2013/03/14/heatshrink-embedded-data-compression/

# Known issues
SPIFFS is broken currently (Feb 2018) in ESP32 Arduino core - hopefully will be fixed there soon. Search through issue list there for solutions - 
usually an earlier commit can be used to get it working. 


