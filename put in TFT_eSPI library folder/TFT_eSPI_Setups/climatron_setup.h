/*
  Project:        Climatron
  Description:    Your personal air quality monitoring robot

  See README.md for target information
*/

#pragma once

// Comment out the #defines below with // to stop that font being loaded
// #define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
// #define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
// #define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
// #define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
// #define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
// #define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
// #define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Climatron uses the SPIFFS file system and TFT_eSPI smooth font code
#define SMOOTH_FONT

// Climatron display hardware configuration
#define JC2432W328_2P8_240x320_ST7789

// alternate Climatron boards not in usee
//#define FNK0086A_2P8_240x320_ST7789 
//#define FNK0102A_1P14_135x240_ST7789 
//#define FNK0103B_2P8_240x320_ST7789
//#define FNK0103F_2P8_240x320_ILI9341
//#define FNK0103L_3P2_240x320_ST7789  
//#define FNK0103N_3P5_320x480_ST7796  
//#define FNK0103S_4P0_320x480_ST7796

#ifdef FNK0086A_2P8_240x320_ST7789
     #include "FNK0086A_2.8_240x320_ST7789.h"
#elif defined FNK0102A_1P14_135x240_ST7789
     #include "FNK0102A_1.14_135x240_ST7789.h"
#elif defined FNK0103B_2P8_240x320_ST7789
     #include "FNK0103B_2.8_240x320_ST7789.h"
#elif defined JC2432W328_2P8_240x320_ST7789
     #include "JC2432W328_2.8_240x320_ST7789.h"
#elif defined FNK0103F_2P8_240x320_ILI9341
     #include "FNK0103F_2.8_240x320_ILI9341.h"
#elif defined FNK0103L_3P2_240x320_ST7789
     #include "FNK0103L_3.2_240x320_ST7789.h"
#elif defined FNK0103N_3P5_320x480_ST7796
     #include "FNK0103N_3.5_320x480_ST7796.h"
#elif defined FNK0103S_4P0_320x480_ST7796
     #include "FNK0103S_4.0_320x480_ST7796.h"
#endif

/////////////////////////////////////////////////////////////////////////////////////
// TFT_eSPI internal driver selection logic
// Keep this section synchronized with TFT_eSPI User_Setup_Select.h
/////////////////////////////////////////////////////////////////////////////////////


// Identical looking TFT displays may have a different colour ordering in the 16 bit colour
#define TFT_BGR 0   // Colour order Blue-Green-Red
#define TFT_RGB 1   // Colour order Red-Green-Blue

// Legacy setup support, RPI_DISPLAY_TYPE replaces RPI_DRIVER
#if defined (RPI_DRIVER)
  #if !defined (RPI_DISPLAY_TYPE)
    #define RPI_DISPLAY_TYPE
  #endif
#endif

// Legacy setup support, RPI_ILI9486_DRIVER form is deprecated
// Instead define RPI_DISPLAY_TYPE and also define driver (e.g. ILI9486_DRIVER) 
#if defined (RPI_ILI9486_DRIVER)
  #if !defined (ILI9486_DRIVER)
    #define ILI9486_DRIVER
  #endif
  #if !defined (RPI_DISPLAY_TYPE)
    #define RPI_DISPLAY_TYPE
  #endif
#endif

// Invoke 18 bit colour for selected displays
#if !defined (RPI_DISPLAY_TYPE) && !defined (TFT_PARALLEL_8_BIT) && !defined (TFT_PARALLEL_16_BIT) && !defined (ESP32_PARALLEL)
  #if defined (ILI9481_DRIVER) || defined (ILI9486_DRIVER) || defined (ILI9488_DRIVER)
    #define SPI_18BIT_DRIVER
  #endif
#endif

// Load the right driver definition - do not tinker here !
#if   defined (ILI9341_DRIVER) || defined(ILI9341_2_DRIVER) || defined (ILI9342_DRIVER)
     #include <TFT_Drivers/ILI9341_Defines.h>
     #define  TFT_DRIVER 0x9341
#elif defined (ST7735_DRIVER)
     #include <TFT_Drivers/ST7735_Defines.h>
     #define  TFT_DRIVER 0x7735
#elif defined (ILI9163_DRIVER)
     #include <TFT_Drivers/ILI9163_Defines.h>
     #define  TFT_DRIVER 0x9163
#elif defined (S6D02A1_DRIVER)
     #include <TFT_Drivers/S6D02A1_Defines.h>
     #define  TFT_DRIVER 0x6D02
#elif defined (ST7796_DRIVER)
      #include "TFT_Drivers/ST7796_Defines.h"
      #define  TFT_DRIVER 0x7796
#elif defined (ILI9486_DRIVER)
     #include <TFT_Drivers/ILI9486_Defines.h>
     #define  TFT_DRIVER 0x9486
#elif defined (ILI9481_DRIVER)
     #include <TFT_Drivers/ILI9481_Defines.h>
     #define  TFT_DRIVER 0x9481
#elif defined (ILI9488_DRIVER)
     #include <TFT_Drivers/ILI9488_Defines.h>
     #define  TFT_DRIVER 0x9488
#elif defined (HX8357D_DRIVER)
     #include "TFT_Drivers/HX8357D_Defines.h"
     #define  TFT_DRIVER 0x8357
#elif defined (EPD_DRIVER)
     #include "TFT_Drivers/EPD_Defines.h"
     #define  TFT_DRIVER 0xE9D
#elif defined (ST7789_DRIVER)
     #include "TFT_Drivers/ST7789_Defines.h"
     #define  TFT_DRIVER 0x7789
#elif defined (R61581_DRIVER)
     #include "TFT_Drivers/R61581_Defines.h"
     #define  TFT_DRIVER 0x6158
#elif defined (ST7789_2_DRIVER)
     #include "TFT_Drivers/ST7789_2_Defines.h"
     #define  TFT_DRIVER 0x778B
#elif defined (RM68140_DRIVER)
     #include "TFT_Drivers/RM68140_Defines.h"
     #define  TFT_DRIVER 0x6814
#elif defined (SSD1351_DRIVER)
     #include "TFT_Drivers/SSD1351_Defines.h"
     #define  TFT_DRIVER 0x1351
#elif defined (SSD1963_480_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800ALT_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (SSD1963_800BD_DRIVER)
     #include "TFT_Drivers/SSD1963_Defines.h"
     #define  TFT_DRIVER 0x1963
#elif defined (GC9A01_DRIVER)
     #include "TFT_Drivers/GC9A01_Defines.h"
     #define  TFT_DRIVER 0x9A01
#elif defined (ILI9225_DRIVER)
     #include "TFT_Drivers/ILI9225_Defines.h"
     #define  TFT_DRIVER 0x9225
#elif defined (RM68120_DRIVER)
     #include "TFT_Drivers/RM68120_Defines.h"
     #define  TFT_DRIVER 0x6812
#elif defined (HX8357B_DRIVER)
     #include "TFT_Drivers/HX8357B_Defines.h"
     #define  TFT_DRIVER 0x835B
#elif defined (HX8357C_DRIVER)
     #include "TFT_Drivers/HX8357C_Defines.h"
     #define  TFT_DRIVER 0x835C

                              // <<<<<<<<<<<<<<<<<<<<<<<< ADD NEW DRIVER HERE
                              // XYZZY_init.h and XYZZY_rotation.h must also be added in TFT_eSPI.cpp
#elif defined (XYZZY_DRIVER)
     #include "TFT_Drivers/XYZZY_Defines.h"
     #define  TFT_DRIVER 0x0000
#else
     #define  TFT_DRIVER 0x0000
#endif