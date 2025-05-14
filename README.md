Pracuję na kontrolerze ESP WROOM 32D i wyświetlaczu LCD ST7735S

Konwert pliku na rozmiar:
https://www.img2go.com/resize-image
128 x 160

Zmiana formatu koloru:
http://www.rinkydinkelectronics.com/_t_doimageconverter565.php

Fragemnt kodu jaki trzeba podmienić w obrazku.h:

#ifndef OBRAZ_H

#define OBRAZ_H

#include <Arduino.h>  // Dla ESP32


const uint16_t obraz[] PROGMEM = {

0xFA18, 0xFA18, 0xFA18, 0xFA18, 0xFA19, 0xFA19, 0xFA39, 0xFA39, 0xFA39, 0xFA39, 0xFA39, 0xFA19, 0xFA19, 0xFA19, 0xFA19, 0xFA19,   // 0x0010 (16) pixels


};
#endif

Bibloteki:

Adafruit ST7735 and ST7789 Library

Adafruit GFX Library

https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

ESP32 by Espressif Systems
