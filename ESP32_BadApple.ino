// Bad Apple for ESP32 with OLED SSD1306 | 2018 by Hackerspace-FFM.de | MIT-License.
#include "FS.h"
#include "SPIFFS.h"
#include "SSD1306.h"
#include "heatshrink_decoder.h"

// MP3 Audio function
#define ENABLE_MP3

#ifdef ENABLE_MP3
// Bad Apple for ESP32 with MP3 Audio and Optimize Draw OLED | 2023 by FREE WING | MIT-License.
// http://www.neko.ne.jp/~freewing/
// Movie 6573 frame, 6573 / 30fps = 219.1 sec
// Original Wave Audio duration 3:39.127, MP3 duration 3:39.220, Delayed 93ms
// ba.mp3 (Layer-3 Lame 22050HZ 40kbps Mono, duration 3:39.220, 1096379 bytes)
// Flash Size = 4MB, Partition Scheme = No OTA(1MB APP/ 3MB SPIFFS)
// mkspiffs.exe -c .\data -s 0x2E0000 SPIFFS.bin
// esptool.exe write_flash 0x110000 SPIFFS.bin

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"

#define ENABLE_I2S_DAC
#define I2S_BCLK  12 /* Bit Clock */
#define I2S_LRC   2 /* Left/Right */

#ifdef ENABLE_I2S_DAC
  #include "AudioOutputI2S.h"
  AudioOutputI2S* pOutput;
  #define I2S_DOUT  15 /* Audio Data */

  // Normary Use Maxim MAX98357 I2S DAC it no need MCLK
  // #define USE_CS4344_DAC
  #ifdef USE_CS4344_DAC
    // Cirrus Logic CS4344 I2S DAC need MCLK
    #define I2S_MCLK 0 /* Master Clock */
  #endif

  // ! Caution Loud Sound !
  #define AUDIO_OUTPUT_GAIN 0.300 /* for MAX98357A or PCM5102A, ES7148 etc. */
  // #define AUDIO_OUTPUT_GAIN 1.999 /* for CS4344 */

  // Adjust this Loop value Yourself !!
  // Syncronize Audio and Video, Pre loop 1.000sec
  #define ADJUST_MP3_PRE_LOOP 700/1000

#else
  #include "AudioOutputI2SNoDAC.h"
  AudioOutputI2SNoDAC* pOutput;
  // GPIO 15 Connect Speaker
  #define I2S_DOUT  15

  // Max 4.0 but Become Silent So It would be Maximum 3.999
  #define AUDIO_OUTPUT_GAIN 1.999

  // Adjust this Loop value Yourself !!
  // Syncronize Audio and Video, Pre loop 0.700sec
  // Tested ESP32-D0WDQ6 (revision 1) WeMos clone
  #define ADJUST_MP3_PRE_LOOP 700/1000

#endif

AudioGeneratorMP3* pMp3;
AudioFileSourceSPIFFS* pFile;

#define MP3_LOOP pMp3->loop(); // Play MP3 Audio

#else
#define MP3_LOOP ; // NOP

#endif

// Board Boot SW GPIO 0
#define BOOT_SW 0

// Frame counter
#define ENABLE_FRAME_COUNTER

// Disable heatshrink error checking
#define DISABLE_HS_ERROR

// Rotate 90 for Direct Write OLED Buffer
#define ENABLE_ROTATE_90

// Enable Dump Bitmap log for Retrieve Original Bitmap data
// #define ENABLE_LOG

// Hints:
// * Adjust the display pins below
// * After uploading to ESP32, also do "ESP32 Sketch Data Upload" from Arduino

// SSD1306 display I2C bus
// For Heltec
// #define I2C_SCL 15
// #define I2C_SDA 4
// #define RESET_OLED 16

// For Wemos Lolin32 ESP32
#define I2C_SCL 13
#define I2C_SDA 14

#define OLED_BRIGHTNESS 196

// MAX freq for SCL is 4 MHz, However, Actual measured value is 892 kHz . (ESP32-D0WDQ6 (revision 1))
// see Inter-Integrated Circuit (I2C)
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
#define I2C_SCLK_FREQ 4000000
#ifdef I2C_SCLK_FREQ
SSD1306 display (0x3c, I2C_SDA, I2C_SCL, GEOMETRY_128_64, I2C_ONE, I2C_SCLK_FREQ);
#else
SSD1306 display (0x3c, I2C_SDA, I2C_SCL);
#endif

// Enable I2C Clock up 892kHz to 1.31MHz (It Actual measured value with ESP32-D0WDQ6 (revision 1))
// #define ENABLE_EXTRA_I2C_CLOCK_UP

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif

static heatshrink_decoder hsd;

// global storage for putPixels
int32_t curr_xy = 0;

// global storage for decodeRLE
int32_t runlength = -1;
int32_t c_to_dup = -1;

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

volatile unsigned long lastRefresh;
// 30 fps target rate = 33.333us
#define FRAME_DERAY_US 33333UL
#ifdef ENABLE_FRAME_COUNTER
int32_t frame = 0;
#endif
#ifdef ENABLE_ROTATE_90
#define CURR_XY_X 0x7f
uint8_t* pImage;
#else
#define CURR_XY_X 0x0f
uint32_t* pImage;
uint32_t b = 0x01;
#endif

#ifdef ENABLE_MP3
#ifndef ENABLE_ROTATE_90
uint32_t black_skip_count = 0;
#endif
const bool isButtonPressing = false;

#else
volatile bool isButtonPressing = false;

void ARDUINO_ISR_ATTR isr() {
    lastRefresh = micros();
    isButtonPressing = (digitalRead(BOOT_SW) == LOW);
}
#endif

void putPixels(uint32_t c, int32_t len) {
#ifndef ENABLE_ROTATE_90
  uint32_t d1;
  uint32_t d2;
#endif

  while(len--) {
    MP3_LOOP; // Play MP3 Audio

    // Direct Draw OLED buffer
#ifdef ENABLE_ROTATE_90
    *pImage++ = c;
#else
    // OLED Buffer Image Rotate 90 Convert X-Y and Byte structure
    {
      // 4 dot(Direct Access 4 byte, 32 bit)
      d1 = 0;
      d2 = 0;
      if (c == 0xff) {
        d1  = b; d1 <<= 8;
        d1 |= b; d1 <<= 8;
        d1 |= b; d1 <<= 8;
        d1 |= b;

        d2 = d1;
      } else if (c != 0x00) {
        // if ((c & 0xF0) != 0x00) {
          if (c & 0x10) d1  = b<<24;
          if (c & 0x20) d1 |= b<<16;
          if (c & 0x40) d1 |= b<< 8;
          if (c & 0x80) d1 |= b;
        // }

        // if ((c & 0x0F) != 0x00) {
          if (c & 0x01) d2  = b<<24;
          if (c & 0x02) d2 |= b<<16;
          if (c & 0x04) d2 |= b<< 8;
          if (c & 0x08) d2 |= b;
        // }
      }

      if (b == 0x01) {
        *pImage++ = d1;
        *pImage   = d2;
      } else if (c != 0x00) {
        *pImage++ |= d1;
        *pImage   |= d2;
      } else {
        pImage++;
#ifdef ENABLE_MP3
        // Adjust Audio Timing for Skip Black every 4
        black_skip_count++;
        if ((black_skip_count << 30) == 0) {
          MP3_LOOP; // Play MP3 Audio
        }
#endif
      }
      pImage++;
    }
#endif

#ifdef ENABLE_LOG
    Serial.print(" 0x");
    Serial.print(c, HEX);
    Serial.print(",");
#endif

    // oyy_ybbb_xxxx X=0-15(4 bit), Bit=0-7(3 bit),Y=0-7(3 bit)
    curr_xy++;
    if((curr_xy & CURR_XY_X) == 0) {
#ifndef ENABLE_ROTATE_90
      pImage -= 128/4;
#endif

#ifdef ENABLE_LOG
      Serial.println("");
#endif

      MP3_LOOP; // Play MP3 Audio

#ifndef ENABLE_ROTATE_90
      b <<= 1;
      if(b == 0x100) {
        // Next Page
        pImage += 128/4;
        b = 0x01;
#endif

        // oyy_ybbb_xxxx X=0-15(4 bit), Bit=0-7(3 bit),Y=0-7(3 bit)
        // Check Overflow bit, It equivalent if((curr_xy & 0x400) != 0)
        if((curr_xy & 0x3ff) == 0) {
#ifdef ENABLE_ROTATE_90
          pImage = display.buffer;
#else
          pImage = (uint32_t*)display.buffer;
#endif

#ifdef ENABLE_LOG_OLED_BUFF
          uint8_t* ppImage = display.buffer;
          for (uint32_t ii = 0; ii < 128*64/8; ++ii) {
            if ((ii % 16) == 0) Serial.println("");

            Serial.print(" 0x");
            Serial.print(*ppImage++, HEX);
            Serial.print(",");
          }
#endif

          MP3_LOOP; // Play MP3 Audio

          // Update Display frame
          display.display();
          //display.clear();

          MP3_LOOP; // Play MP3 Audio

          if(!isButtonPressing) {
            // 30 fps target rate = 33.333us
            lastRefresh += FRAME_DERAY_US;
#ifdef ENABLE_FRAME_COUNTER
            // Adjust 33.334us every 3 frame
            if ((++frame % 3) == 0) lastRefresh++;
#endif
            while(micros() < lastRefresh) ;

#ifdef ENABLE_LOG
            Serial.print("  // ");
            Serial.println(frame);
            Serial.println("");
#endif

            MP3_LOOP; // Play MP3 Audio
          }
        }

#ifndef ENABLE_ROTATE_90
      }
#endif
    }
  }
}

void decodeRLE(uint32_t c) {
    MP3_LOOP; // Play MP3 Audio

    if(c_to_dup == -1) {
      if((c == 0x55) || (c == 0xaa)) {
        c_to_dup = c;
      } else {
        putPixels(c, 1);
      }
    } else {
      if(runlength == -1) {
        if(c == 0) {
          putPixels(c_to_dup & 0xff, 1);
          c_to_dup = -1;
        } else if((c & 0x80) == 0) {
          if(c_to_dup == 0x55) {
            putPixels(0, c);
          } else {
            putPixels(255, c);
          }
          c_to_dup = -1;
        } else {
          runlength = c & 0x7f;
        }
      } else {
        runlength = runlength | (c << 7);
          if(c_to_dup == 0x55) {
            putPixels(0, runlength);
          } else {
            putPixels(255, runlength);
          }
          c_to_dup = -1;
          runlength = -1;
      }
    }
}

#define RLEBUFSIZE 4096
#define READBUFSIZE 2048
void readFile(fs::FS &fs, const char * path){
    static uint8_t rle_buf[RLEBUFSIZE];
    uint8_t* p_rle_buf;
    size_t rle_size;

    size_t filelen;
    size_t filesize;
    static uint8_t compbuf[READBUFSIZE];

    Serial.printf("Reading file: %s\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("Failed to open file for reading");
        display.drawStringMaxWidth(0, 10, 128, "File open error. Upload video.hs using ESP32 Sketch Upload."); display.display();
        return;
    }
    filelen = file.size();
    filesize = filelen;
    Serial.printf("File size: %d\n", filelen);

    // init display, putPixels and decodeRLE
    display.resetDisplay();
    // curr_xy = 0;
#ifdef ENABLE_ROTATE_90
    pImage = display.buffer;
#else
    pImage = (uint32_t*)display.buffer;
#endif
    // runlength = -1;
    // c_to_dup = -1;

    // init decoder
    heatshrink_decoder_reset(&hsd);
    size_t   count  = 0;
    uint32_t sunk   = 0;
    size_t toSink = 0;
    uint32_t sinkHead = 0;

    Serial.println("Start.");

#ifdef ENABLE_LOG
    uint32_t unhs = 0;
    Serial.println("====");
#endif

#ifdef ENABLE_MP3
    // Syncronize Audio and Video, Pre loop
    for(uint32_t i=0; i<44100UL * ADJUST_MP3_PRE_LOOP; ++i) {
      pMp3->loop();
    }
#endif

    lastRefresh = micros();

    // Go through file...
    while(filelen) {
      if(toSink == 0) {
        toSink = file.read(compbuf, READBUFSIZE);
        filelen -= toSink;
        sinkHead = 0;
      }

      // uncompress buffer
      heatshrink_decoder_sink(&hsd, &compbuf[sinkHead], toSink, &count);
      //Serial.print("^^ sinked ");
      //Serial.println(count);
      toSink -= count;
      sinkHead = count;
      sunk += count;
      if (sunk == filesize) {
        heatshrink_decoder_finish(&hsd);
      }

      HSD_poll_res pres;
      do {
          // rle_size = 0;
          pres = heatshrink_decoder_poll(&hsd, rle_buf, RLEBUFSIZE, &rle_size);
#ifdef ENABLE_LOG
          unhs += rle_size;
#endif
          //Serial.print("^^ polled ");
          //Serial.println(rle_size);
#ifndef DISABLE_HS_ERROR
          if(pres < 0) {
            Serial.print("POLL ERR! ");
            Serial.println(pres);
            return;
          }
#endif

          p_rle_buf = rle_buf;
          while(rle_size) {
            rle_size--;
#ifndef DISABLE_HS_ERROR
            if(rle_bufhead >= RLEBUFSIZE) {
              Serial.println("RLE_SIZE ERR!");
              return;
            }
#endif
            decodeRLE(*(p_rle_buf++));
          }
      } while (pres == HSDR_POLL_MORE);
    }
    file.close();

#ifdef ENABLE_LOG
    Serial.println("====");
    Serial.print("heatshrink decode size: ");
    Serial.println(unhs);
#endif

#ifdef ENABLE_FRAME_COUNTER
    Serial.print("Done. ");
    Serial.println(frame);
#else
    Serial.println("Done.");
#endif
#ifdef ENABLE_MP3
    while (pMp3->loop()) ;
    pMp3->stop();
    Serial.println("MP3 end");
#endif

    // 10 sec
    delay(10000);
    display.resetDisplay();
#ifdef ENABLE_MP3
    display.drawStringMaxWidth(0, 0, 128, "Add MP3 Audio version. modded By FREE WING 2023/02/18");
#else
    display.drawStringMaxWidth(0, 0, 128, "Optimize OLED Draw Performance version. modded By FREE WING");
#endif
    display.drawStringMaxWidth(0, 40, 128, "http://www.neko.ne.jp/~freewing/"); display.display();
    delay(10000);
    // Reset to Infinite Loop Demo !
    ESP.restart();
}



void setup(){
#ifdef ENABLE_LOG
    Serial.begin(921600);
#else
    Serial.begin(115200);
#endif
#ifdef RESET_OLED
    // Reset for some displays
    pinMode(RESET_OLED, OUTPUT); digitalWrite(RESET_OLED, LOW); delay(50); digitalWrite(RESET_OLED, HIGH);
#endif
    display.init();
#ifdef OLED_BRIGHTNESS
    display.setBrightness(OLED_BRIGHTNESS);
#endif
    display.flipScreenVertically ();
    display.clear();
    display.setTextAlignment (TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(WHITE);
    display.drawString(0, 0, "Mounting SPIFFS...     ");
    display.display();
    if(!SPIFFS.begin()){
        Serial.println("SPIFFS mount failed");
        display.drawStringMaxWidth(0, 10, 128, "SPIFFS mount failed. Upload video.hs using ESP32 Sketch Upload."); display.display();
        return;
    }

#ifndef ENABLE_MP3
    pinMode(BOOT_SW, INPUT_PULLUP);
    attachInterrupt(BOOT_SW, isr, CHANGE);
#endif
    Serial.print("totalBytes(): ");
    Serial.println(SPIFFS.totalBytes());
    Serial.print("usedBytes(): ");
    Serial.println(SPIFFS.usedBytes());
    listDir(SPIFFS, "/", 0);

#ifdef ENABLE_MP3
    pFile = new AudioFileSourceSPIFFS("/ba.mp3" );
    if (pFile->getSize() == 0) {
        Serial.println("Failed to open file for MP3");
        display.drawStringMaxWidth(0, 10, 128, "MP3 File open error. Upload ba.mp3 using ESP32 Sketch Upload."); display.display();
        while(true) ;
    }
#ifdef ENABLE_I2S_DAC
    pOutput = new AudioOutputI2S();
    pOutput->SetGain(AUDIO_OUTPUT_GAIN);
#else
    pOutput = new AudioOutputI2SNoDAC();
    pOutput->SetGain(AUDIO_OUTPUT_GAIN);
    // pOutput->SetRate(44100 / 2); // 22050Hz enough
    // pOutput->SetBitsPerSample(8); // 8 bit enough
    // pOutput->SetChannels(1); // Mono
#endif

#ifdef USE_CS4344_DAC
    // Add support for I2S MCLK. #594 on Jan 5, 2023
    // https://github.com/earlephilhower/ESP8266Audio/pull/594
    // Release 1.9.7 Jun 10, 2022 not yet Merged
    pOutput->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, I2S_MCLK);
#else
    pOutput->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    pOutput->SetOutputModeMono(true); // MAX98357 module is Mono
#endif

    pMp3 = new AudioGeneratorMP3();
    pMp3->begin(pFile, pOutput);
#endif

#ifdef ENABLE_EXTRA_I2C_CLOCK_UP
    // Direct Access I2C SCL frequency setting value
    // It Tested ESP32-D0WDQ6 (revision 1)
    uint32_t* ptr;
    ptr = (uint32_t*)0x3FF53000; // I2C_SCL_LOW_PERIOD_REG
    // *ptr = 30; // Don't work
    // *ptr = 31; // Sometime Stop Frame drawing
    // *ptr = 32; // Works
    *ptr = 35; // Safety value
    ptr = (uint32_t*)0x3FF53038; // I2C_SCL_HIGH_PERIOD_REG
    // *ptr = 0; // Works
    *ptr = 2; // Safety value
#endif

    readFile(SPIFFS, "/video.hs");

    //Serial.print("Format SPIFSS? (enter y for yes): ");
    // while(!Serial.available()) ;
    //if(Serial.read() == 'y') {
    //  bool ret = SPIFFS.format();
    //  if(ret) Serial.println("Success. "); else Serial.println("FAILED! ");
    //} else {
    //  Serial.println("Aborted.");
    //}
}

void loop(){

}

